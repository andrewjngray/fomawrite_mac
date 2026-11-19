#include "pdfeditor.h"

#include <qpdf/Buffer.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjGen.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>

#include <algorithm>
#include <exception>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

namespace PdfEditor {
namespace {

struct Refused { QString message; };   // thrown inside this file only

QString plain(const std::exception &e) {
    return QStringLiteral("qpdf reported: %1").arg(QString::fromUtf8(e.what()));
}

std::shared_ptr<QPDF> open(const QByteArray &pdf, Refusal *refusal, QString *message) {
    *refusal = Refusal::None;
    auto q = QPDF::create();
    q->setSuppressWarnings(true);
    try {
        if (pdf.isEmpty()) throw std::runtime_error("empty input");
        q->processMemoryFile("pdf", pdf.constData(), size_t(pdf.size()));
        QPDFPageDocumentHelper helper(*q);
        if (helper.getAllPages().empty()) throw std::runtime_error("no pages");
    } catch (const QPDFExc &e) {
        const bool password = e.getErrorCode() == qpdf_e_password;
        *refusal = password ? Refusal::NeedsPassword : Refusal::Damaged;
        if (message) *message = password
            ? QStringLiteral("This PDF is password protected. Opening it for editing needs the password.")
            : QStringLiteral("This PDF is damaged and cannot be edited (%1).").arg(QString::fromUtf8(e.what()));
        return nullptr;
    } catch (const std::exception &e) {
        *refusal = Refusal::Damaged;
        if (message) *message = QStringLiteral("This PDF is damaged and cannot be edited (%1).").arg(QString::fromUtf8(e.what()));
        return nullptr;
    } catch (...) {
        *refusal = Refusal::Damaged;
        if (message) *message = QStringLiteral("This PDF is damaged and cannot be edited.");
        return nullptr;
    }
    if (q->isEncrypted() && !q->allowModifyAssembly()) {
        *refusal = Refusal::NeedsPassword;
        if (message) *message = QStringLiteral("This PDF's permissions do not allow changing its pages. Changing them needs the owner password.");
        return nullptr;
    }
    return q;
}

bool hasSignature(QPDFObjectHandle field, std::set<QPDFObjGen> &seen, int depth) {
    if (!field.isDictionary() || depth > 64) return false;
    if (field.isIndirect() && !seen.insert(field.getObjGen()).second) return false;
    if (field.getKey("/FT").isName() && field.getKey("/FT").getName() == "/Sig" && !field.getKey("/V").isNull()) return true;
    QPDFObjectHandle kids = field.getKey("/Kids");
    if (kids.isArray())
        for (int i = 0; i < kids.getArrayNItems(); ++i)
            if (hasSignature(kids.getArrayItem(i), seen, depth + 1)) return true;
    return false;
}

bool isSigned(QPDF &q) {
    QPDFObjectHandle root = q.getRoot();
    if (root.getKey("/Perms").isDictionary()) return true;
    QPDFObjectHandle form = root.getKey("/AcroForm");
    if (!form.isDictionary()) return false;
    QPDFObjectHandle flags = form.getKey("/SigFlags");
    if (flags.isInteger() && (flags.getIntValue() & 1)) return true;
    QPDFObjectHandle fields = form.getKey("/Fields");
    std::set<QPDFObjGen> seen;
    if (fields.isArray())
        for (int i = 0; i < fields.getArrayNItems(); ++i)
            if (hasSignature(fields.getArrayItem(i), seen, 0)) return true;
    return false;
}

const QString SignedMessage = QStringLiteral("This PDF is digitally signed. Changing its pages will invalidate the signature.");

Result failure(const QString &message) {
    Result r;
    r.error = message;
    return r;
}

// The input opened for an edit: refused as the contract says.
std::shared_ptr<QPDF> openForEdit(const QByteArray &pdf, bool allowSigned, Result *failed) {
    Refusal refusal;
    QString message;
    auto q = open(pdf, &refusal, &message);
    if (!q) { *failed = failure(message); return nullptr; }
    if (!allowSigned && isSigned(*q)) { *failed = failure(SignedMessage); return nullptr; }
    return q;
}

QByteArray write(QPDF &q) {
    QPDFWriter writer(q);
    writer.setOutputMemory();
    writer.write();
    std::shared_ptr<Buffer> buffer = writer.getBufferSharedPointer();
    return QByteArray(reinterpret_cast<const char *>(buffer->getBuffer()), qsizetype(buffer->getSize()));
}

// Validates a selection against pageCount; returns the sorted, de-duplicated set.
bool selection(const QVector<int> &pages, int pageCount, std::vector<int> *out, QString *error) {
    if (pages.isEmpty()) { *error = QStringLiteral("No pages are selected."); return false; }
    std::set<int> set;
    for (const int page : pages) {
        if (page < 0 || page >= pageCount) {
            *error = QStringLiteral("Page %1 does not exist; the document has %2 page(s).").arg(page + 1).arg(pageCount);
            return false;
        }
        set.insert(page);
    }
    out->assign(set.begin(), set.end());
    return true;
}

// Runs an edit, turning every exception into a failure.
template <typename F> Result guarded(F &&edit) {
    try {
        return edit();
    } catch (const std::exception &e) {
        return failure(plain(e));
    } catch (...) {
        return failure(QStringLiteral("qpdf failed for an unknown reason."));
    }
}

}

Refusal check(const QByteArray &pdf, QString *message) {
    Refusal refusal = Refusal::None;
    try {
        QString text;
        auto q = open(pdf, &refusal, &text);
        if (q && isSigned(*q)) { refusal = Refusal::Signed; text = SignedMessage; }
        if (message) *message = refusal == Refusal::None ? QString() : text;
    } catch (...) {
        if (message) *message = QStringLiteral("This PDF is damaged and cannot be edited.");
        return Refusal::Damaged;
    }
    return refusal;
}

Result rotate(const QByteArray &pdf, const QVector<int> &pages, int degreesClockwise, bool allowSigned) {
    return guarded([&]() -> Result {
        const int turn = ((degreesClockwise % 360) + 360) % 360;
        if (turn == 0 || turn % 90 != 0) return failure(QStringLiteral("A page can be turned by 90, 180 or 270 degrees, not %1.").arg(degreesClockwise));
        Result failed;
        auto q = openForEdit(pdf, allowSigned, &failed);
        if (!q) return failed;
        auto all = QPDFPageDocumentHelper(*q).getAllPages();
        std::vector<int> chosen;
        QString error;
        if (!selection(pages, int(all.size()), &chosen, &error)) return failure(error);
        Result r;
        for (int i = 0; i < int(all.size()); ++i) r.pages.append({i, 0});
        for (const int page : chosen) {
            all[size_t(page)].rotatePage(turn, true);
            r.pages[page].rotation = turn;
        }
        r.bytes = write(*q);
        r.ok = true;
        return r;
    });
}

Result remove(const QByteArray &pdf, const QVector<int> &pages, bool allowSigned) {
    return guarded([&]() -> Result {
        Result failed;
        auto q = openForEdit(pdf, allowSigned, &failed);
        if (!q) return failed;
        QPDFPageDocumentHelper helper(*q);
        auto all = helper.getAllPages();
        std::vector<int> chosen;
        QString error;
        if (!selection(pages, int(all.size()), &chosen, &error)) return failure(error);
        if (chosen.size() == all.size()) return failure(QStringLiteral("A PDF must keep at least one page, so every page cannot be removed."));
        Result r;
        for (int i = 0; i < int(all.size()); ++i)
            if (!std::binary_search(chosen.begin(), chosen.end(), i)) r.pages.append({i, 0});
        for (auto it = chosen.rbegin(); it != chosen.rend(); ++it) helper.removePage(all[size_t(*it)]);
        r.bytes = write(*q);
        r.ok = true;
        return r;
    });
}

Result move(const QByteArray &pdf, const QVector<int> &pages, int before, bool allowSigned) {
    return guarded([&]() -> Result {
        Result failed;
        auto q = openForEdit(pdf, allowSigned, &failed);
        if (!q) return failed;
        QPDFPageDocumentHelper helper(*q);
        auto all = helper.getAllPages();
        const int count = int(all.size());
        std::vector<int> chosen;
        QString error;
        if (!selection(pages, count, &chosen, &error)) return failure(error);
        if (before < 0 || before > count)
            return failure(QStringLiteral("Cannot move pages before position %1; the document has %2 page(s).").arg(before + 1).arg(count));
        std::vector<int> order;     // output order, as input indices
        for (int i = 0; i < before; ++i) if (!std::binary_search(chosen.begin(), chosen.end(), i)) order.push_back(i);
        order.insert(order.end(), chosen.begin(), chosen.end());
        for (int i = before; i < count; ++i) if (!std::binary_search(chosen.begin(), chosen.end(), i)) order.push_back(i);
        Result r;
        for (const int i : order) r.pages.append({i, 0});
        bool identity = true;
        for (int i = 0; i < count; ++i) identity = identity && order[size_t(i)] == i;
        if (identity) { r.bytes = pdf; r.ok = true; return r; }
        for (auto &page : all) helper.removePage(page);
        for (const int i : order) helper.addPage(all[size_t(i)], false);
        r.bytes = write(*q);
        r.ok = true;
        return r;
    });
}

Result insert(const QByteArray &pdf, const QByteArray &other, int before, bool allowSigned) {
    return guarded([&]() -> Result {
        Result failed;
        auto q = openForEdit(pdf, allowSigned, &failed);
        if (!q) return failed;
        Refusal refusal;
        QString message;
        auto source = open(other, &refusal, &message);
        if (!source) return failure(QStringLiteral("The PDF to insert cannot be used: %1").arg(message));
        QPDFPageDocumentHelper helper(*q);
        auto all = helper.getAllPages();
        const int count = int(all.size());
        if (before < 0 || before > count)
            return failure(QStringLiteral("Cannot insert pages before position %1; the document has %2 page(s).").arg(before + 1).arg(count));
        auto added = QPDFPageDocumentHelper(*source).getAllPages();
        Result r;
        for (int i = 0; i < before; ++i) r.pages.append({i, 0});
        for (size_t i = 0; i < added.size(); ++i) r.pages.append({-1, 0});
        for (int i = before; i < count; ++i) r.pages.append({i, 0});
        for (auto &page : added) {
            if (before == count) helper.addPage(page, false);
            else helper.addPageAt(page, true, all[size_t(before)]);
        }
        r.bytes = write(*q);   // `source` must outlive this: foreign streams are read when writing
        r.ok = true;
        return r;
    });
}

Result extract(const QByteArray &pdf, const QVector<int> &pages) {
    return guarded([&]() -> Result {
        Result failed;
        auto q = openForEdit(pdf, true, &failed);
        if (!q) return failed;
        auto all = QPDFPageDocumentHelper(*q).getAllPages();
        if (pages.isEmpty()) return failure(QStringLiteral("No pages are selected."));
        for (const int page : pages)
            if (page < 0 || page >= int(all.size()))
                return failure(QStringLiteral("Page %1 does not exist; the document has %2 page(s).").arg(page + 1).arg(all.size()));
        auto out = QPDF::create();
        out->setSuppressWarnings(true);
        out->emptyPDF();
        QPDFPageDocumentHelper target(*out);
        Result r;
        for (const int page : pages) {
            target.addPage(all[size_t(page)], false);
            r.pages.append({page, 0});
        }
        r.bytes = write(*out);
        r.ok = true;
        return r;
    });
}

}
