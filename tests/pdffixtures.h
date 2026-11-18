#pragma once

// Hand-built PDF fixtures for the PDF tests (Cycle 160), ported from the
// Phase 0 spike's make_fixtures.py (research/cycle-159). Text sits at known
// PDF user-space positions so geometry and rendering can be checked exactly:
// ALPHA at (100, 600) and OMEGA at (300, 200), Helvetica 24.

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QRectF>
#include <QString>
#include <functional>

namespace PdfFixtures {

constexpr double FontSize = 24;
// Helvetica advance widths (1/1000 em) for the capitals used here.
inline double textWidth(const QByteArray &word) {
    double total = 0;
    for (const char c : word) {
        switch (c) {
        case 'A': case 'P': case 'E': total += 667; break;
        case 'L': total += 556; break;
        case 'H': total += 722; break;
        case 'O': case 'G': total += 778; break;
        case 'M': total += 833; break;
        default: total += 600; break;
        }
    }
    return total * FontSize / 1000;
}

struct Word { QByteArray text; double x, y; };
inline const Word Alpha{"ALPHA", 100, 600};
inline const Word Omega{"OMEGA", 300, 200};

// The word's box in PDF user space, from the baseline's descender to cap height
// as the spike measured it: (x, y-5.52) .. (x+width, y+18.48).
inline QRectF userBox(const Word &word) {
    return QRectF(QPointF(word.x, word.y - 5.52), QPointF(word.x + textWidth(word.text), word.y + 18.48));
}

// The spike's mapping from user space to page view points (top-left origin,
// after /Rotate, inside the CropBox). crop: left, bottom, right, top.
inline QRectF userToView(const QRectF &user, int rotate, const QRectF &crop) {
    const double x0 = crop.left(), y0 = crop.top(), x1 = crop.right(), y1 = crop.bottom();
    const auto map = [&](QPointF p) -> QPointF {
        switch (rotate) {
        case 90: return {p.y() - y0, p.x() - x0};
        case 180: return {x1 - p.x(), p.y() - y0};
        case 270: return {y1 - p.y(), x1 - p.x()};
        default: return {p.x() - x0, y1 - p.y()};
        }
    };
    return QRectF(map(user.topLeft()), map(user.bottomRight())).normalized();
}

class Writer {
public:
    int reserve() { m_objects.append(QByteArray()); return int(m_objects.size()); }
    int add(const QByteArray &body) { m_objects.append(body); return int(m_objects.size()); }
    void set(int number, const QByteArray &body) { m_objects[number - 1] = body; }
    int stream(const QByteArray &data, const QByteArray &extra = {}) {
        return add("<< /Length " + QByteArray::number(data.size()) + " " + extra + " >>\nstream\n" + data + "\nendstream");
    }
    QByteArray write(int root, int info = 0) const {
        QByteArray out("%PDF-1.7\n%\xe2\xe3\xcf\xd3\n");
        QList<qsizetype> offsets;
        for (int i = 0; i < m_objects.size(); ++i) {
            offsets.append(out.size());
            out += QByteArray::number(i + 1) + " 0 obj\n" + m_objects.at(i) + "\nendobj\n";
        }
        const qsizetype xref = out.size();
        out += "xref\n0 " + QByteArray::number(m_objects.size() + 1) + "\n0000000000 65535 f \n";
        for (const qsizetype offset : offsets) out += QByteArray::number(offset).rightJustified(10, '0') + " 00000 n \n";
        out += "trailer\n<< /Size " + QByteArray::number(m_objects.size() + 1) + " /Root " + QByteArray::number(root) + " 0 R"
            + (info ? " /Info " + QByteArray::number(info) + " 0 R" : QByteArray()) + " >>\nstartxref\n"
            + QByteArray::number(xref) + "\n%%EOF\n";
        return out;
    }
private:
    QList<QByteArray> m_objects;
};

inline QByteArray num(double value) { return QByteArray::number(value, 'f', 2); }
inline QByteArray wordOps(const Word &word) {
    return "BT /F1 " + num(FontSize) + " Tf " + num(word.x) + " " + num(word.y) + " Td (" + word.text + ") Tj ET\n";
}

struct Page {
    QByteArray content;
    int rotate = 0;
    QRectF crop;        // left, bottom, right, top in user space; empty for none
    // Builds the page's annotation objects; receives the writer and every page's object number.
    std::function<QList<int>(Writer &, const QList<int> &)> annotations;
};

// One document of the given pages; `catalog` and `info` may add to the catalog
// (given the page object numbers) and set the /Info dictionary.
inline QByteArray document(const QList<Page> &pages,
                           const std::function<QByteArray(Writer &, const QList<int> &)> &catalog = {},
                           const QByteArray &info = {}) {
    Writer pdf;
    const int font = pdf.add("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>");
    const int pagesId = pdf.reserve();
    QList<int> pageIds;
    for (int i = 0; i < pages.size(); ++i) pageIds.append(pdf.reserve());
    for (int i = 0; i < pages.size(); ++i) {
        const Page &page = pages.at(i);
        const int content = pdf.stream(page.content);
        QByteArray annots;
        if (page.annotations) {
            QByteArray refs;
            for (const int id : page.annotations(pdf, pageIds)) refs += QByteArray::number(id) + " 0 R ";
            annots = "/Annots [" + refs + "]";
        }
        QByteArray box;
        if (!page.crop.isEmpty())
            box = "/CropBox [" + num(page.crop.left()) + " " + num(page.crop.top()) + " " + num(page.crop.right()) + " " + num(page.crop.bottom()) + "]";
        pdf.set(pageIds.at(i), "<< /Type /Page /Parent " + QByteArray::number(pagesId) + " 0 R /MediaBox [0 0 612 792] /Rotate "
            + QByteArray::number(page.rotate) + " " + box + " /Resources << /Font << /F1 " + QByteArray::number(font)
            + " 0 R >> >> /Contents " + QByteArray::number(content) + " 0 R " + annots + " >>");
    }
    QByteArray kids;
    for (const int id : pageIds) kids += QByteArray::number(id) + " 0 R ";
    pdf.set(pagesId, "<< /Type /Pages /Kids [" + kids + "] /Count " + QByteArray::number(pageIds.size()) + " >>");
    const QByteArray extra = catalog ? catalog(pdf, pageIds) : QByteArray();
    const int root = pdf.add("<< /Type /Catalog /Pages " + QByteArray::number(pagesId) + " 0 R " + extra + " >>");
    const int infoId = info.isEmpty() ? 0 : pdf.add(info);
    return pdf.write(root, infoId);
}

inline QByteArray wordsContent() { return wordOps(Alpha) + wordOps(Omega); }

// Five pages with ALPHA and OMEGA: /Rotate 0, 90, 180, 270, then 0 with CropBox [50 100 562 742].
inline QList<Page> geometryPages() {
    QList<Page> pages;
    for (const int rotate : {0, 90, 180, 270}) pages.append(Page{wordsContent(), rotate, {}, {}});
    pages.append(Page{wordsContent(), 0, QRectF(QPointF(50, 100), QPointF(562, 742)), {}});
    return pages;
}
inline QRectF geometryCrop(int page) {
    return page == 4 ? QRectF(QPointF(50, 100), QPointF(562, 742)) : QRectF(QPointF(0, 0), QPointF(612, 792));
}
inline const int geometryRotation[] = {0, 90, 180, 270, 0};

// QuadPoints from descender to cap height, as the spike wrote them.
inline QList<double> quad(const Word &word) {
    const double w = textWidth(word.text), top = word.y + 0.718 * FontSize, bottom = word.y - 0.207 * FontSize;
    return {word.x, top, word.x + w, top, word.x, bottom, word.x + w, bottom};
}
inline QByteArray markup(const char *kind, const char *colour, const Word &word) {
    const auto q = quad(word);
    QByteArray points;
    for (const double v : q) points += num(v) + " ";
    return QByteArray("<< /Type /Annot /Subtype /") + kind + " /Rect [" + num(q[0]) + " " + num(q[5]) + " " + num(q[2]) + " "
        + num(q[1]) + "] /C [" + colour + "] /F 4 /QuadPoints [" + points + "] >>";
}
// Page 1 of the geometry document with a yellow Highlight over ALPHA, an
// Underline and a StrikeOut over OMEGA and a Text note, none with /AP.
inline QByteArray annotatedDocument() {
    Page page{wordsContent(), 0, {}, [](Writer &pdf, const QList<int> &) {
        return QList<int>{pdf.add(markup("Highlight", "1 0.85 0", Alpha)), pdf.add(markup("Underline", "0 0.4 1", Omega)),
                          pdf.add(markup("StrikeOut", "1 0 0", Omega)),
                          pdf.add("<< /Type /Annot /Subtype /Text /Rect [450 650 470 670] /C [1 0.9 0.3] /F 4 "
                                  "/Contents (Fixture note) /Name /Note /Open false >>")};
    }};
    return document({page});
}
// The highlight's rectangle in view points on that (unrotated) page.
inline QRectF highlightView() { return userToView(QRectF(QPointF(100, 594.48), QPointF(178.7, 618.48)), 0, geometryCrop(0)); }

inline QByteArray manyPagesDocument(int count) {
    QList<Page> pages;
    for (int n = 1; n <= count; ++n)
        pages.append(Page{"BT /F1 24 Tf 72 720 Td (PAGE) Tj ET\nBT /F1 24 Tf 160 720 Td (" + QByteArray::number(n) + ") Tj ET\n", 0, {}, {}});
    return document(pages);
}

// Two pages with metadata, an outline (one entry per page), a link from page 1
// to page 2 and a web address in page 1's text.
inline QByteArray navigationDocument() {
    // Qt PDF reports web links it finds in the page text (not /URI actions).
    QList<Page> pages{Page{wordsContent() + "BT /F1 12 Tf 100 400 Td (https://example.com/fomawrite) Tj ET\n", 0, {},
                           [](Writer &pdf, const QList<int> &ids) {
        return QList<int>{pdf.add("<< /Type /Annot /Subtype /Link /Rect [100 594 179 618] /Border [0 0 0] /Dest ["
                                  + QByteArray::number(ids.at(1)) + " 0 R /XYZ 0 792 0] >>"),
                          pdf.add("<< /Type /Annot /Subtype /Link /Rect [300 194 389 218] /Border [0 0 0] "
                                  "/A << /Type /Action /S /URI /URI (https://example.org/omega) >> >>")};
    }}, Page{wordOps(Omega), 0, {}, {}}};
    const auto outline = [](Writer &pdf, const QList<int> &ids) {
        const int root = pdf.reserve(), first = pdf.reserve(), second = pdf.reserve();
        pdf.set(root, "<< /Type /Outlines /First " + QByteArray::number(first) + " 0 R /Last " + QByteArray::number(second) + " 0 R /Count 2 >>");
        pdf.set(first, "<< /Title (Opening) /Parent " + QByteArray::number(root) + " 0 R /Next " + QByteArray::number(second)
                + " 0 R /Dest [" + QByteArray::number(ids.at(0)) + " 0 R /XYZ 0 792 0] >>");
        pdf.set(second, "<< /Title (Closing) /Parent " + QByteArray::number(root) + " 0 R /Prev " + QByteArray::number(first)
                + " 0 R /Dest [" + QByteArray::number(ids.at(1)) + " 0 R /XYZ 0 792 0] >>");
        return "/Outlines " + QByteArray::number(root) + " 0 R";
    };
    return document(pages, outline,
        "<< /Title (Fixture Title) /Author (Fixture Author) /Subject (Fixture Subject) /Keywords (alpha omega) "
        "/Producer (Fomawrite tests) /Creator (pdffixtures.h) /CreationDate (D:20261119120000Z) >>");
}

// A PDF whose header and cross-reference table are present but whose objects are cut short.
inline QByteArray damagedDocument() {
    QByteArray bytes = document(geometryPages());
    return bytes.left(120) + QByteArray(64, '\0') + "garbage";
}

inline bool write(const QString &path, const QByteArray &bytes) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size();
}

}
