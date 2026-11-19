// Cycle 165 part 1 — PdfEditor over qpdf: refusals, rotate, remove, move,
// insert and extract, each verified through Qt PDF (page count, order by each
// page's text, orientation by page size) plus the `pages` map. Fixtures are
// built here (tests/pdffixtures.h); the encrypted ones are tests/fixtures/.

#include "pdfeditor.h"
#include <QBuffer>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfLinkModel>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>

namespace {
using PdfEditor::PageRef;
using PdfEditor::Refusal;
using PdfEditor::Result;

// Pages labelled "P1".."Pn" (letters first so the text is easy to compare), with the given /Rotate values.
QByteArray cycle165Document(const QStringList &labels, const QList<int> &rotates = {}) {
    QList<PdfFixtures::Page> pages;
    for (int i = 0; i < labels.size(); ++i)
        pages.append(PdfFixtures::Page{"BT /F1 24 Tf 72 720 Td (" + labels.at(i).toLatin1() + ") Tj ET\n", rotates.value(i, 0), {}, {}});
    return PdfFixtures::document(pages);
}
QByteArray cycle165Letters(int n, int first = 0) {
    QStringList labels;
    for (int i = 0; i < n; ++i) labels << QStringLiteral("PAGE%1").arg(QChar('A' + first + i));
    return cycle165Document(labels);
}

// A document read back through Qt PDF.
struct Cycle165Loaded {
    QByteArray bytes;
    QBuffer buffer;
    QPdfDocument document;
    bool ok = false;
    explicit Cycle165Loaded(const QByteArray &data) : bytes(data) {
        buffer.setData(bytes);
        buffer.open(QIODevice::ReadOnly);
        document.load(&buffer);
        ok = document.status() == QPdfDocument::Status::Ready;
    }
    int count() const { return document.pageCount(); }
    QString text(int page) { return document.getAllText(page).text().trimmed(); }
    QStringList texts() { QStringList out; for (int i = 0; i < count(); ++i) out << text(i); return out; }
    bool landscape(int page) const { const QSizeF s = document.pagePointSize(page); return s.width() > s.height(); }
};
QStringList cycle165Texts(const QByteArray &bytes) { Cycle165Loaded l(bytes); return l.ok ? l.texts() : QStringList{QStringLiteral("<unreadable>")}; }
QList<bool> cycle165Landscape(const QByteArray &bytes) {
    Cycle165Loaded l(bytes);
    QList<bool> out;
    for (int i = 0; i < l.count(); ++i) out << l.landscape(i);
    return out;
}
// Effective /Rotate of every page (inherited values included), read with qpdf itself.
QList<int> cycle165Rotations(const QByteArray &bytes) {
    QPDF q;
    q.setSuppressWarnings(true);
    q.processMemoryFile("t", bytes.constData(), size_t(bytes.size()));
    QList<int> out;
    for (auto &page : QPDFPageDocumentHelper(q).getAllPages()) {
        QPDFObjectHandle r = page.getAttribute("/Rotate", false);
        out << (r.isInteger() ? int(((r.getIntValue() % 360) + 360) % 360) : 0);
    }
    return out;
}
QList<int> cycle165Sources(const Result &r) { QList<int> out; for (const PageRef &p : r.pages) out << p.source; return out; }
QList<int> cycle165Turns(const Result &r) { QList<int> out; for (const PageRef &p : r.pages) out << p.rotation; return out; }
QString cycle165Page(const char *letter) { return QStringLiteral("PAGE") + QLatin1String(letter); }

// A catalog extra giving the document a signature field (with or without a value) and flags.
QByteArray cycle165Signed(bool withValue, const QByteArray &flags) {
    return PdfFixtures::document({PdfFixtures::Page{"BT /F1 24 Tf 72 720 Td (PAGEA) Tj ET\n", 0, {}, {}}},
        [&](PdfFixtures::Writer &pdf, const QList<int> &) {
            const QByteArray value = withValue
                ? " /V << /Type /Sig /Filter /Adobe.PPKLite /SubFilter /adbe.pkcs7.detached /ByteRange [0 0 0 0] /Contents <00> >>" : "";
            const int field = pdf.add("<< /FT /Sig /T (Signature1) /Type /Annot /Subtype /Widget /Rect [0 0 0 0]" + value + " >>");
            return "/AcroForm << /Fields [" + QByteArray::number(field) + " 0 R] " + flags + " >>";
        });
}
QByteArray cycle165Fixture(const char *name) {
    QFile file(QFINDTESTDATA(QStringLiteral("fixtures/") + QLatin1String(name)));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
}

void FomawriteTest::pdfEditorChecksWhetherADocumentCanBeEdited() {
    QString message;
    QCOMPARE(PdfEditor::check(cycle165Letters(2), &message), Refusal::None);
    QVERIFY(message.isEmpty());

    QCOMPARE(PdfEditor::check(PdfFixtures::damagedDocument(), &message), Refusal::Damaged);
    QVERIFY(!message.isEmpty());
    QCOMPARE(PdfEditor::check(QByteArray("# not a pdf"), &message), Refusal::Damaged);
    QCOMPARE(PdfEditor::check(QByteArray(), &message), Refusal::Damaged);

    // Encrypted: a user password is needed; an empty user password that allows
    // changes is editable; one that forbids changes needs the owner password.
    QCOMPARE(PdfEditor::check(cycle165Fixture("password.pdf"), &message), Refusal::NeedsPassword);
    QVERIFY(message.contains("password"));
    QCOMPARE(PdfEditor::check(cycle165Fixture("emptypassword.pdf"), &message), Refusal::None);
    QCOMPARE(PdfEditor::check(cycle165Fixture("restricted.pdf"), &message), Refusal::NeedsPassword);

    // Signed: a signature with a value, or /SigFlags, but not an empty signature field.
    QCOMPARE(PdfEditor::check(cycle165Signed(true, "/SigFlags 3"), &message), Refusal::Signed);
    QVERIFY(message.contains("signed"));
    QCOMPARE(PdfEditor::check(cycle165Signed(true, ""), &message), Refusal::Signed);
    QCOMPARE(PdfEditor::check(cycle165Signed(false, "/SigFlags 1"), &message), Refusal::Signed);
    QCOMPARE(PdfEditor::check(cycle165Signed(false, ""), &message), Refusal::None);
}

void FomawriteTest::pdfEditorRotatesOnePageSeveralPagesAndExistingRotation() {
    const QByteArray three = cycle165Letters(3);
    // One page, clockwise: only it becomes landscape; the others and their order are untouched.
    Result r = PdfEditor::rotate(three, {1}, 90);
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(cycle165Texts(r.bytes), (QStringList{cycle165Page("A"), cycle165Page("B"), cycle165Page("C")}));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{false, true, false}));
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{0, 90, 0}));
    QCOMPARE(cycle165Sources(r), (QList<int>{0, 1, 2}));
    QCOMPARE(cycle165Turns(r), (QList<int>{0, 90, 0}));

    // Anticlockwise from 0 is 270; 180 keeps the orientation.
    r = PdfEditor::rotate(three, {0}, -90);
    QVERIFY(r.ok);
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{270, 0, 0}));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{true, false, false}));
    QCOMPARE(cycle165Turns(r), (QList<int>{270, 0, 0}));
    r = PdfEditor::rotate(three, {2}, 180);
    QVERIFY(r.ok);
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{0, 0, 180}));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{false, false, false}));

    // Several pages, selection order and repeats ignored.
    r = PdfEditor::rotate(three, {2, 0, 2}, 90);
    QVERIFY(r.ok);
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{true, false, true}));
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{90, 0, 90}));
    QCOMPARE(cycle165Turns(r), (QList<int>{90, 0, 90}));

    // Relative to an existing /Rotate: pages already at 0, 90, 270.
    const QByteArray turned = cycle165Document({QStringLiteral("PAGEA"), QStringLiteral("PAGEB"), QStringLiteral("PAGEC")}, {0, 90, 270});
    QCOMPARE(cycle165Landscape(turned), (QList<bool>{false, true, true}));
    r = PdfEditor::rotate(turned, {0, 1, 2}, 90);
    QVERIFY(r.ok);
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{90, 180, 0}));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{true, false, false}));
    r = PdfEditor::rotate(turned, {1}, -90);
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{0, 0, 270}));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{false, false, true}));

    // Turning back undoes it.
    r = PdfEditor::rotate(PdfEditor::rotate(three, {1}, 90).bytes, {1}, -90);
    QVERIFY(r.ok);
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{0, 0, 0}));
}

void FomawriteTest::pdfEditorRotatesAnInheritedRotation() {
    // /Rotate 90 on the page tree node, none on the pages: both pages inherit it.
    const QByteArray inherited = PdfFixtures::document(
        {PdfFixtures::Page{"BT /F1 24 Tf 72 720 Td (PAGEA) Tj ET\n", 0, {}, {}}, PdfFixtures::Page{"BT /F1 24 Tf 72 720 Td (PAGEB) Tj ET\n", 0, {}, {}}},
        {}, {});
    // Rebuild with the rotation on the /Pages node instead of on the pages.
    QByteArray bytes = inherited;
    bytes.replace("/Rotate 0", "");
    bytes.replace("/Type /Pages", "/Type /Pages /Rotate 90");
    // Offsets shifted; qpdf rebuilds the table from the objects.
    QCOMPARE(cycle165Landscape(bytes), (QList<bool>{true, true}));
    QCOMPARE(cycle165Rotations(bytes), (QList<int>{90, 90}));
    Result r = PdfEditor::rotate(bytes, {1}, 90);
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(cycle165Rotations(r.bytes), (QList<int>{90, 180}));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{true, false}));
    QCOMPARE(cycle165Texts(r.bytes), (QStringList{cycle165Page("A"), cycle165Page("B")}));
}

void FomawriteTest::pdfEditorRemovesPages() {
    const QByteArray four = cycle165Letters(4);
    Result r = PdfEditor::remove(four, {1});
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(cycle165Texts(r.bytes), (QStringList{cycle165Page("A"), cycle165Page("C"), cycle165Page("D")}));
    QCOMPARE(cycle165Sources(r), (QList<int>{0, 2, 3}));
    QCOMPARE(cycle165Turns(r), (QList<int>{0, 0, 0}));

    r = PdfEditor::remove(four, {3, 0, 3});
    QVERIFY(r.ok);
    QCOMPARE(cycle165Texts(r.bytes), (QStringList{cycle165Page("B"), cycle165Page("C")}));
    QCOMPARE(cycle165Sources(r), (QList<int>{1, 2}));

    r = PdfEditor::remove(four, {0, 1, 2});
    QVERIFY(r.ok);
    QCOMPARE(cycle165Texts(r.bytes), (QStringList{cycle165Page("D")}));

    // Every page: refused, nothing returned.
    r = PdfEditor::remove(four, {0, 1, 2, 3});
    QVERIFY(!r.ok);
    QVERIFY(r.error.contains("at least one page"));
    QVERIFY(r.bytes.isEmpty());
    QVERIFY(r.pages.isEmpty());
    QVERIFY(!PdfEditor::remove(cycle165Letters(1), {0}).ok);
}

void FomawriteTest::pdfEditorMovesPages() {
    const QByteArray five = cycle165Letters(5);   // A B C D E
    const auto order = [](const Result &r) { return cycle165Texts(r.bytes).join(' ').remove("PAGE"); };
    // Forwards: A to before D -> B C A D E.
    Result r = PdfEditor::move(five, {0}, 3);
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(order(r), QStringLiteral("B C A D E"));
    QCOMPARE(cycle165Sources(r), (QList<int>{1, 2, 0, 3, 4}));
    // To the end.
    r = PdfEditor::move(five, {1}, 5);
    QCOMPARE(order(r), QStringLiteral("A C D E B"));
    // Backwards: E to the start.
    r = PdfEditor::move(five, {4}, 0);
    QCOMPARE(order(r), QStringLiteral("E A B C D"));
    QCOMPARE(cycle165Sources(r), (QList<int>{4, 0, 1, 2, 3}));
    // Several pages keep their relative order, however the selection is given.
    r = PdfEditor::move(five, {3, 1}, 0);
    QCOMPARE(order(r), QStringLiteral("B D A C E"));
    r = PdfEditor::move(five, {0, 1}, 5);
    QCOMPARE(order(r), QStringLiteral("C D E A B"));
    r = PdfEditor::move(five, {0, 4}, 2);
    QCOMPARE(order(r), QStringLiteral("B A E C D"));
    // `before` inside the selection's own span.
    r = PdfEditor::move(five, {1, 3}, 2);
    QCOMPARE(order(r), QStringLiteral("A B D C E"));
    QCOMPARE(cycle165Turns(r), (QList<int>{0, 0, 0, 0, 0}));

    // A move that changes nothing returns the input untouched.
    for (const auto &[pages, before] : {std::pair<QVector<int>, int>{{2}, 2}, {{2}, 3}, {{1, 2}, 1}, {{0, 1, 2, 3, 4}, 0}, {{4}, 5}}) {
        r = PdfEditor::move(five, pages, before);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.bytes, five);
        QCOMPARE(cycle165Sources(r), (QList<int>{0, 1, 2, 3, 4}));
    }
}

void FomawriteTest::pdfEditorInsertsPagesAtStartMiddleAndEnd() {
    const QByteArray base = cycle165Letters(3);                 // A B C
    const QByteArray other = cycle165Document({QStringLiteral("NEWONE"), QStringLiteral("NEWTWO")}, {0, 90});
    const auto order = [](const Result &r) { return cycle165Texts(r.bytes).join(' ').remove("PAGE"); };
    Result r = PdfEditor::insert(base, other, 0);
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(order(r), QStringLiteral("NEWONE NEWTWO A B C"));
    QCOMPARE(cycle165Sources(r), (QList<int>{-1, -1, 0, 1, 2}));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{false, true, false, false, false}));
    r = PdfEditor::insert(base, other, 1);
    QCOMPARE(order(r), QStringLiteral("A NEWONE NEWTWO B C"));
    QCOMPARE(cycle165Sources(r), (QList<int>{0, -1, -1, 1, 2}));
    r = PdfEditor::insert(base, other, 3);
    QCOMPARE(order(r), QStringLiteral("A B C NEWONE NEWTWO"));
    QCOMPARE(cycle165Sources(r), (QList<int>{0, 1, 2, -1, -1}));
    QCOMPARE(cycle165Turns(r), (QList<int>{0, 0, 0, 0, 0}));
    // The same document into itself.
    r = PdfEditor::insert(base, base, 1);
    QVERIFY(r.ok);
    QCOMPARE(order(r), QStringLiteral("A A B C B C"));

    // The inserted pages are usable: a later edit of the result works.
    r = PdfEditor::remove(PdfEditor::insert(base, other, 1).bytes, {1});
    QCOMPARE(order(r), QStringLiteral("A NEWTWO B C"));

    // Bad `before`, and an unusable or protected `other`.
    QVERIFY(!PdfEditor::insert(base, other, -1).ok);
    QVERIFY(!PdfEditor::insert(base, other, 4).ok);
    r = PdfEditor::insert(base, PdfFixtures::damagedDocument(), 0);
    QVERIFY(!r.ok);
    QVERIFY(r.error.contains("cannot be used"));
    QVERIFY(!PdfEditor::insert(base, cycle165Fixture("password.pdf"), 0).ok);
}

void FomawriteTest::pdfEditorExtractsPagesIntoANewDocument() {
    const QByteArray five = cycle165Letters(5);
    const auto order = [](const Result &r) { return cycle165Texts(r.bytes).join(' ').remove("PAGE"); };
    Result r = PdfEditor::extract(five, {3, 1});
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(order(r), QStringLiteral("D B"));          // in the order given
    QCOMPARE(cycle165Sources(r), (QList<int>{3, 1}));
    r = PdfEditor::extract(five, {2});
    QCOMPARE(order(r), QStringLiteral("C"));
    r = PdfEditor::extract(five, {0, 1, 2, 3, 4});
    QCOMPARE(order(r), QStringLiteral("A B C D E"));
    // A rotated source page stays rotated; the source is not touched.
    const QByteArray turned = cycle165Document({QStringLiteral("PAGEA"), QStringLiteral("PAGEB")}, {0, 90});
    r = PdfEditor::extract(turned, {1});
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{true}));
    QVERIFY(!PdfEditor::extract(five, {}).ok);
    QVERIFY(!PdfEditor::extract(five, {5}).ok);
    QVERIFY(!PdfEditor::extract(five, {-1}).ok);
    // A signed source can be extracted from without the flag; the copy carries no signature.
    r = PdfEditor::extract(cycle165Signed(true, "/SigFlags 3"), {0});
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(PdfEditor::check(r.bytes), Refusal::None);
}

void FomawriteTest::pdfEditorRefusesWhatItCannotEditAndRejectsBadArguments() {
    const QByteArray good = cycle165Letters(3);
    const QByteArray other = cycle165Letters(1);
    const QByteArray signedPdf = cycle165Signed(true, "/SigFlags 3");
    const QList<QByteArray> unusable{PdfFixtures::damagedDocument(), cycle165Fixture("password.pdf"), cycle165Fixture("restricted.pdf"),
                                     QByteArray("garbage"), QByteArray()};
    for (const QByteArray &bad : unusable) {
        for (const Result &r : {PdfEditor::rotate(bad, {0}, 90), PdfEditor::remove(bad, {0}), PdfEditor::move(bad, {0}, 0),
                                PdfEditor::insert(bad, other, 0), PdfEditor::extract(bad, {0})}) {
            QVERIFY(!r.ok);
            QVERIFY(!r.error.isEmpty());
            QVERIFY(r.bytes.isEmpty());
            QVERIFY(r.pages.isEmpty());
        }
    }
    // Signed: refused unless allowed; allowed edits still produce a readable document.
    for (const Result &r : {PdfEditor::rotate(signedPdf, {0}, 90), PdfEditor::move(signedPdf, {0}, 0), PdfEditor::insert(signedPdf, other, 0)}) {
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains("signed"));
    }
    QVERIFY(!PdfEditor::remove(cycle165Signed(true, "/SigFlags 3"), {0}).ok);
    Result r = PdfEditor::rotate(signedPdf, {0}, 90, true);
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{true}));
    r = PdfEditor::insert(signedPdf, other, 1, true);
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(cycle165Texts(r.bytes).size(), 2);

    // An encrypted PDF with an empty user password is editable and stays encrypted.
    const QByteArray empty = cycle165Fixture("emptypassword.pdf");
    r = PdfEditor::rotate(empty, {0}, 90);
    QVERIFY2(r.ok, qPrintable(r.error));
    QVERIFY(r.bytes.contains("/Encrypt"));
    QCOMPARE(cycle165Landscape(r.bytes), (QList<bool>{true}));

    // Bad arguments: errors, not silent no-ops.
    for (const int degrees : {0, 45, 360, 91}) QVERIFY2(!PdfEditor::rotate(good, {0}, degrees).ok, qPrintable(QString::number(degrees)));
    QVERIFY(PdfEditor::rotate(good, {0}, 270).ok);
    QVERIFY(PdfEditor::rotate(good, {0}, -270).ok);
    QVERIFY(!PdfEditor::rotate(good, {}, 90).ok);
    QVERIFY(!PdfEditor::rotate(good, {3}, 90).ok);
    QVERIFY(!PdfEditor::rotate(good, {-1, 0}, 90).ok);
    QVERIFY(!PdfEditor::remove(good, {}).ok);
    QVERIFY(!PdfEditor::remove(good, {7}).ok);
    QVERIFY(!PdfEditor::move(good, {}, 1).ok);
    QVERIFY(!PdfEditor::move(good, {3}, 0).ok);
    QVERIFY(!PdfEditor::move(good, {0}, -1).ok);
    QVERIFY(!PdfEditor::move(good, {0}, 4).ok);
    QVERIFY(PdfEditor::rotate(good, {3}, 90).error.contains("Page 4"));
}

void FomawriteTest::pdfEditorKeepsMetadataOutlineAndLinks() {
    const QByteArray navigation = PdfFixtures::navigationDocument();   // 2 pages, info, 2 outline entries, links on page 1
    const auto check = [&](const Result &r, int pages) {
        QVERIFY2(r.ok, qPrintable(r.error));
        Cycle165Loaded l(r.bytes);
        QVERIFY(l.ok);
        QCOMPARE(l.count(), pages);
        QCOMPARE(l.document.metaData(QPdfDocument::MetaDataField::Title).toString(), QStringLiteral("Fixture Title"));
        QCOMPARE(l.document.metaData(QPdfDocument::MetaDataField::Author).toString(), QStringLiteral("Fixture Author"));
        QPdfBookmarkModel outline;
        outline.setDocument(&l.document);
        QCOMPARE(outline.rowCount({}), 2);
        QPdfLinkModel links;
        links.setDocument(&l.document);
        links.setPage(r.pages.first().source == 0 ? 0 : 1);   // the page that was page 1
        QVERIFY(links.rowCount({}) >= 2);
    };
    check(PdfEditor::rotate(navigation, {0}, 90), 2);
    check(PdfEditor::move(navigation, {1}, 0), 2);
    check(PdfEditor::insert(navigation, cycle165Letters(1), 2), 3);
    // Page boxes untouched by a rotation.
    QList<PdfFixtures::Page> pages{PdfFixtures::Page{PdfFixtures::wordsContent(), 0, QRectF(QPointF(50, 100), QPointF(562, 742)), {}}};
    const Result r = PdfEditor::rotate(PdfFixtures::document(pages), {0}, 90);
    QVERIFY(r.ok);
    QVERIFY(r.bytes.contains("/CropBox"));
    Cycle165Loaded l(r.bytes);
    QCOMPARE(l.document.pagePointSize(0), QSizeF(642, 512));   // the 512 x 642 crop, turned
}
