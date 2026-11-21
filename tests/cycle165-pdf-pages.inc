// Cycle 165 part 2a — pending page edits in PdfSession (rotate, remove, move,
// insert, undo and redo, the page map from the baseline, refusals, the
// changed-on-disk flag, adopting a saved state) and the notes' marks following
// them (draw-time translation; marking while edits are pending writes baseline
// page numbers). Everything is verified through the session's own QPdfDocument
// and PdfMarks; there is no user interface yet.

#include "pdfmarks.h"
#include "pdfsession.h"

namespace {
using Origin = PdfSession::PageOrigin;
using OriginList = QVector<Origin>;

// Pages labelled "PAGEA".. followed by the fixtures' ALPHA and OMEGA words.
QByteArray cycle165Marked(int count, int first = 0) {
    QList<PdfFixtures::Page> pages;
    for (int i = 0; i < count; ++i)
        pages.append(PdfFixtures::Page{"BT /F1 24 Tf 72 720 Td (PAGE" + QByteArray(1, char('A' + first + i)) + ") Tj ET\n" + PdfFixtures::wordsContent(), 0, {}, {}});
    return PdfFixtures::document(pages);
}

struct Cycle165Session {
    QTemporaryDir directory;
    PdfSession session;
    QString path;
    bool open(const QByteArray &bytes, const QString &name = QStringLiteral("doc.pdf")) {
        path = directory.filePath(name);
        return directory.isValid() && PdfFixtures::write(path, bytes) && session.load(QUrl::fromLocalFile(path))
            && session.state() == QLatin1String("ready");
    }
    // The first five characters of every page's text, in order ("PAGEA", ...).
    QStringList texts() {
        QStringList out;
        for (int i = 0; i < session.pageCount(); ++i) out << session.pageText(i).trimmed().left(5);
        return out;
    }
    QList<bool> landscape() {
        QList<bool> out;
        for (int i = 0; i < session.pageCount(); ++i) out << (session.pageSize(i).width() > session.pageSize(i).height());
        return out;
    }
    bool openOther(const QByteArray &bytes, QUrl *url, const QString &name = QStringLiteral("other.pdf")) {
        *url = QUrl::fromLocalFile(directory.filePath(name));
        return PdfFixtures::write(directory.filePath(name), bytes);
    }
};
OriginList cycle165Map(std::initializer_list<std::pair<int, int>> entries) {
    OriginList map;
    for (const auto &entry : entries) map.append({entry.first, entry.second});
    return map;
}
QString cycle165Describe(const OriginList &map) {
    QStringList parts;
    for (const Origin &o : map) parts << QStringLiteral("%1/%2").arg(o.origin).arg(o.rotation);
    return parts.join(QLatin1Char(' '));
}
#define CYCLE165_MAP(actual, ...) QCOMPARE(cycle165Describe(actual), cycle165Describe(cycle165Map(__VA_ARGS__)))

// Notes + marks over a session, without a window.
struct Cycle165Notes {
    QString text;
    PdfMarks marks;
    explicit Cycle165Notes(PdfSession *session)
        : marks(session, [this] { return text; }, [this](const PdfNotes::Edit &edit) { text = PdfNotes::apply(text, edit); return true; }) {}
    QVariantList items() { marks.refresh(); return marks.items(); }
    QVariantMap item(int i) { return items().at(i).toMap(); }
};
}

void FomawriteTest::pdfPagesEditsApplyToTheSessionsDocument() {
    Cycle165Session s;
    QVERIFY(s.open(cycle165Marked(3)));
    QVERIFY(!s.session.pagesModified());
    QVERIFY(!s.session.canUndoPageEdit());
    const int revision = s.session.revision();
    QSignalSpy edits(&s.session, &PdfSession::pageEditsChanged);

    QVERIFY(s.session.rotatePages({1}, 90));
    QVERIFY(s.session.revision() > revision);
    QCOMPARE(s.session.pageCount(), 3);
    QCOMPARE(s.landscape(), (QList<bool>{false, true, false}));
    QVERIFY(s.session.pagesModified());
    QVERIFY(s.session.canUndoPageEdit());
    QVERIFY(!s.session.canRedoPageEdit());
    QVERIFY(edits.count() >= 1);
    QVERIFY(s.session.rotatePages({1}, -90)); // back upright
    QCOMPARE(s.landscape(), (QList<bool>{false, false, false}));

    QVERIFY(s.session.movePages({2}, 0));
    QCOMPARE(s.texts(), (QStringList{"PAGEC", "PAGEA", "PAGEB"}));
    QVERIFY(s.session.removePages({1}));
    QCOMPARE(s.texts(), (QStringList{"PAGEC", "PAGEB"}));
    QUrl other;
    QVERIFY(s.openOther(cycle165Marked(2, 3), &other)); // PAGED, PAGEE
    QVERIFY(s.session.insertPdf(other, 1));
    QCOMPARE(s.texts(), (QStringList{"PAGEC", "PAGED", "PAGEE", "PAGEB"}));
    QCOMPARE(s.session.info().value("pageCount").toInt(), 4);
    QVERIFY(s.session.pageEditError().isEmpty());

    // A change that changes nothing succeeds without an undo step.
    const QByteArray before = s.session.workingBytes();
    QVERIFY(s.session.movePages({0}, 0));
    QCOMPARE(s.session.workingBytes(), before);
    // Bad arguments are refused with a message and change nothing.
    QVERIFY(!s.session.removePages({9}));
    QVERIFY(!s.session.pageEditError().isEmpty());
    QCOMPARE(s.session.workingBytes(), before);
    // The file on disk was never touched.
    QCOMPARE(s.session.baselineBytes(), cycle165Marked(3));
}

void FomawriteTest::pdfPagesMapComposesAcrossOperations() {
    Cycle165Session s;
    QVERIFY(s.open(cycle165Marked(4)));
    CYCLE165_MAP(s.session.pageMap(), {{0, 0}, {1, 0}, {2, 0}, {3, 0}});
    QVERIFY(s.session.rotatePages({1, 2}, 90));
    CYCLE165_MAP(s.session.pageMap(), {{0, 0}, {1, 90}, {2, 90}, {3, 0}});
    QVERIFY(s.session.rotatePages({2}, 180));
    CYCLE165_MAP(s.session.pageMap(), {{0, 0}, {1, 90}, {2, 270}, {3, 0}});
    QVERIFY(s.session.rotatePages({1}, -90));
    CYCLE165_MAP(s.session.pageMap(), {{0, 0}, {1, 0}, {2, 270}, {3, 0}});
    QVERIFY(s.session.movePages({3, 2}, 1)); // C and D (in order) before B
    CYCLE165_MAP(s.session.pageMap(), {{0, 0}, {2, 270}, {3, 0}, {1, 0}});
    QVERIFY(s.session.removePages({0}));
    CYCLE165_MAP(s.session.pageMap(), {{2, 270}, {3, 0}, {1, 0}});
    QUrl other;
    QVERIFY(s.openOther(cycle165Marked(1, 5), &other));
    QVERIFY(s.session.insertPdf(other, 1));
    CYCLE165_MAP(s.session.pageMap(), {{2, 270}, {-1, 0}, {3, 0}, {1, 0}});
    QVERIFY(s.session.rotatePages({0, 1}, 90)); // a page from another file has no baseline turn
    CYCLE165_MAP(s.session.pageMap(), {{2, 0}, {-1, 0}, {3, 0}, {1, 0}});
    QCOMPARE(s.texts(), (QStringList{"PAGEC", "PAGEF", "PAGED", "PAGEB"}));
    // Translation helpers read the map.
    QCOMPARE(s.session.workingPageForBaseline(3), 2);
    QCOMPARE(s.session.workingPageForBaseline(0), -1);
    QCOMPARE(s.session.baselinePageForWorking(1), -1);
    QCOMPARE(s.session.baselinePageForWorking(3), 1);
    // Undo walks the map back.
    QVERIFY(s.session.undoPageEdit());
    CYCLE165_MAP(s.session.pageMap(), {{2, 270}, {-1, 0}, {3, 0}, {1, 0}});
    QVERIFY(s.session.undoPageEdit());
    QVERIFY(s.session.undoPageEdit());
    CYCLE165_MAP(s.session.pageMap(), {{0, 0}, {2, 270}, {3, 0}, {1, 0}});
    QVERIFY(s.session.redoPageEdit());
    CYCLE165_MAP(s.session.pageMap(), {{2, 270}, {3, 0}, {1, 0}});
}

void FomawriteTest::pdfPagesUndoAndRedoRestoreExactBytesWithinABound() {
    Cycle165Session s;
    const QByteArray original = cycle165Marked(3);
    QVERIFY(s.open(original));
    QVERIFY(s.session.rotatePages({0}, 90));
    const QByteArray afterRotate = s.session.workingBytes();
    QVERIFY(s.session.removePages({2}));
    const QByteArray afterRemove = s.session.workingBytes();
    QVERIFY(afterRotate != original);

    QVERIFY(s.session.undoPageEdit());
    QCOMPARE(s.session.workingBytes(), afterRotate);
    QCOMPARE(s.session.pageCount(), 3);
    QVERIFY(s.session.canRedoPageEdit());
    QVERIFY(s.session.undoPageEdit());
    QCOMPARE(s.session.workingBytes(), original);
    QVERIFY(!s.session.pagesModified());
    QVERIFY(!s.session.canUndoPageEdit());
    QVERIFY(!s.session.undoPageEdit());
    QVERIFY(s.session.redoPageEdit());
    QVERIFY(s.session.redoPageEdit());
    QCOMPARE(s.session.workingBytes(), afterRemove);
    QVERIFY(!s.session.redoPageEdit());
    QVERIFY(s.session.pagesModified());
    // A new edit after an undo clears redo.
    QVERIFY(s.session.undoPageEdit());
    QVERIFY(s.session.rotatePages({1}, 180));
    QVERIFY(!s.session.canRedoPageEdit());

    // The history is bounded by count, and by bytes; the oldest goes first.
    QVERIFY(s.open(cycle165Marked(3), QStringLiteral("bound.pdf")));
    s.session.setUndoLimits(3, 1ll << 30);
    for (int i = 0; i < 6; ++i) QVERIFY(s.session.rotatePages({0}, 90));
    int steps = 0;
    while (s.session.undoPageEdit()) ++steps;
    QCOMPARE(steps, 3);
    QVERIFY(s.session.pagesModified()); // the original is no longer reachable
    QVERIFY(s.open(cycle165Marked(3), QStringLiteral("bytes.pdf")));
    s.session.setUndoLimits(20, qint64(s.session.workingBytes().size()) * 5 / 2);
    for (int i = 0; i < 6; ++i) QVERIFY(s.session.rotatePages({0}, 90));
    steps = 0;
    while (s.session.undoPageEdit()) ++steps;
    QVERIFY(steps >= 1 && steps <= 2);
    // Loading another PDF clears everything (and the default limits return with a fresh session).
    QVERIFY(s.session.rotatePages({0}, 90));
    QVERIFY(s.open(cycle165Marked(2), QStringLiteral("next.pdf")));
    QVERIFY(!s.session.canUndoPageEdit() && !s.session.canRedoPageEdit() && !s.session.pagesModified());
    s.session.close();
    QVERIFY(s.session.baselineBytes().isEmpty() && s.session.pageMap().isEmpty());
}

void FomawriteTest::pdfPagesRefusalsLeaveTheStateUntouched() {
    Cycle165Session s;
    // Signed: refused until the owner of the window allows it.
    const QByteArray signedPdf = cycle165Signed(true, "/SigFlags 3");
    QVERIFY(s.open(signedPdf));
    QVERIFY(!s.session.signedEditsAllowed());
    QVERIFY(!s.session.rotatePages({0}, 90));
    QVERIFY(s.session.pageEditError().contains("signed"));
    QVERIFY(!s.session.pagesModified());
    QCOMPARE(s.session.workingBytes(), signedPdf);
    QSignalSpy errorSpy(&s.session, &PdfSession::pageEditErrorChanged);
    s.session.setSignedEditsAllowed(true);
    QVERIFY(s.session.signedEditsAllowed());
    QVERIFY(s.session.rotatePages({0}, 90));
    QVERIFY(s.session.pageEditError().isEmpty());
    QVERIFY(errorSpy.count() >= 1);
    QVERIFY(s.session.pagesModified());
    // Opening another PDF forgets the permission.
    QVERIFY(s.open(cycle165Marked(2), QStringLiteral("again.pdf")));
    QVERIFY(!s.session.signedEditsAllowed());

    // Needs a password: awaiting it, and unlocked (the file stays encrypted, so it is never edited).
    const QString locked = QFINDTESTDATA("fixtures/password.pdf");
    QVERIFY(!locked.isEmpty());
    QVERIFY(s.session.load(QUrl::fromLocalFile(locked)));
    QCOMPARE(s.session.state(), QStringLiteral("password"));
    QVERIFY(!s.session.rotatePages({0}, 90));
    QVERIFY(!s.session.pageEditError().isEmpty());
    QVERIFY(s.session.load(QUrl::fromLocalFile(locked), QStringLiteral("fomawrite-user")));
    QCOMPARE(s.session.state(), QStringLiteral("ready"));
    const QByteArray lockedBytes = s.session.workingBytes();
    QVERIFY(!s.session.removePages({0}));
    QVERIFY(!s.session.pageEditError().isEmpty());
    QVERIFY(!s.session.pagesModified());
    QCOMPARE(s.session.workingBytes(), lockedBytes);

    // Damaged or unreadable file to insert; nothing open.
    QVERIFY(s.open(cycle165Marked(2), QStringLiteral("target.pdf")));
    const QByteArray before = s.session.workingBytes();
    QUrl bad;
    QVERIFY(s.openOther(PdfFixtures::damagedDocument(), &bad, QStringLiteral("damaged.pdf")));
    QVERIFY(!s.session.insertPdf(bad, 0));
    QVERIFY(s.session.pageEditError().contains("cannot be used"));
    QVERIFY(!s.session.insertPdf(QUrl::fromLocalFile(s.directory.filePath("missing.pdf")), 0));
    QVERIFY(!s.session.insertPdf(QUrl(QStringLiteral("https://example.org/a.pdf")), 0));
    QCOMPARE(s.session.workingBytes(), before);
    QVERIFY(!s.session.pagesModified() && !s.session.canUndoPageEdit());
    s.session.close();
    QVERIFY(!s.session.rotatePages({0}, 90));
    QVERIFY(!s.session.pageEditError().isEmpty());
}

void FomawriteTest::pdfPagesFileChangedOnDiskWhileEditingIsFlagged() {
    Cycle165Session s;
    QVERIFY(s.open(cycle165Marked(3)));
    QVERIFY(s.session.rotatePages({0}, 90));
    const QByteArray working = s.session.workingBytes();
    QSignalSpy flagged(&s.session, &PdfSession::changedOnDiskChanged);
    QVERIFY(PdfFixtures::write(s.path, cycle165Marked(5)));
    QVERIFY(QTest::qWaitFor([&] { return s.session.changedOnDiskWhileEditing(); }, 5000));
    QVERIFY(flagged.count() >= 1);
    QCOMPARE(s.session.workingBytes(), working);
    QCOMPARE(s.session.pageCount(), 3);
    QVERIFY(s.session.pagesModified());
    QVERIFY(s.session.undoPageEdit()); // the history is intact
    QCOMPARE(s.session.pageCount(), 3);

    // Without pending edits a change reloads as before, and the flag is clear.
    Cycle165Session t;
    QVERIFY(t.open(cycle165Marked(3)));
    // Windows registers a new watch on its watcher thread; a write straight after
    // opening can fall before the watch's first snapshot and go unnoticed.
    QTest::qWait(300);
    QVERIFY(PdfFixtures::write(t.path, cycle165Marked(5)));
    QVERIFY(QTest::qWaitFor([&] { return t.session.pageCount() == 5; }, 5000));
    QVERIFY(!t.session.changedOnDiskWhileEditing());
    QCOMPARE(t.session.baselineBytes(), cycle165Marked(5));
}

void FomawriteTest::pdfPagesAdoptingTheWorkingStateClearsTheHistory() {
    Cycle165Session s;
    QVERIFY(s.open(cycle165Marked(3)));
    QVERIFY(s.session.rotatePages({1}, 90));
    QVERIFY(s.session.removePages({0}));
    const QByteArray adopted = s.session.workingBytes();
    QVERIFY(s.session.rotatePages({1}, 180));
    QVERIFY(s.session.undoPageEdit());
    CYCLE165_MAP(s.session.pageMap(), {{1, 90}, {2, 0}});
    QVERIFY(s.session.canUndoPageEdit());
    QVERIFY(s.session.canRedoPageEdit());

    QVERIFY(s.session.adoptWorkingAsBaseline());
    QCOMPARE(s.session.baselineBytes(), adopted);
    QCOMPARE(s.session.workingBytes(), adopted);
    CYCLE165_MAP(s.session.pageMap(), {{0, 0}, {1, 0}});
    QCOMPARE(s.session.baselinePageCount(), 2);
    QVERIFY(!s.session.pagesModified());
    QVERIFY(!s.session.undoPageEdit());
    QVERIFY(!s.session.redoPageEdit());
}

void FomawriteTest::pdfPagesTextIndicesDoNotChangeWithRotation() {
    Cycle165Session s;
    QVERIFY(s.open(cycle165Marked(1)));
    const QString text = s.session.pageText(0);
    const int alpha = text.indexOf(QStringLiteral("ALPHA"));
    QVERIFY(alpha >= 0);
    for (const int degrees : {90, 180, 270}) {
        QVERIFY(s.session.rotatePages({0}, 90));
        QCOMPARE(s.session.pageText(0), text);
        const QVariantMap selection = s.session.selectionAt(0, alpha, 5);
        QCOMPARE(selection.value("text").toString(), QStringLiteral("ALPHA"));
        QVERIFY(!selection.value("rects").toList().isEmpty());
        QCOMPARE(s.session.rotationOfWorkingPage(0), degrees);
    }
}

void FomawriteTest::pdfPagesMarksAndPinsStayOnTheirContent() {
    // Notes written against the unedited PDF: a highlight on ALPHA and a pin, both on page 1; a pin on page 2.
    const auto build = [](Cycle165Session &s, Cycle165Notes &n, int *start, const QString &word = QStringLiteral("ALPHA")) {
        const QString text = s.session.pageText(0);
        *start = text.indexOf(word);
        n.marks.mark(QStringLiteral("highlight"), QStringLiteral("yellow"), 0, *start, 5);
        n.marks.placeNote(0, 100, 200);
        n.marks.placeNote(1, 300, 400);
        return n.items().size() == 3;
    };
    const auto find = [](Cycle165Notes &n, const QString &kind, int index = 0) {
        for (const QVariant &v : n.items())
            if (v.toMap().value("kind").toString() == kind && index-- == 0) return v.toMap();
        return QVariantMap();
    };
    // Rotated pages: the pin lands where its page spot went; the highlight is still over ALPHA.
    struct Expect { int degrees; double x, y; };
    for (const Expect &e : {Expect{90, 592, 100}, Expect{180, 512, 592}, Expect{270, 200, 512}}) {
        Cycle165Session s;
        Cycle165Notes n(&s.session);
        int start = 0;
        QVERIFY(s.open(cycle165Marked(2)));
        QVERIFY(build(s, n, &start));
        const QString written = n.text;
        QVERIFY(s.session.rotatePages({0}, e.degrees));
        const QVariantMap pin = find(n, "note", 0);
        QVERIFY(pin.value("placed").toBool());
        QCOMPARE(pin.value("page").toInt(), 0);
        QCOMPARE(pin.value("x").toInt(), int(e.x));
        QCOMPARE(pin.value("y").toInt(), int(e.y));
        const QVariantMap mark = find(n, "highlight");
        QVERIFY(mark.value("placed").toBool());
        QCOMPARE(mark.value("page").toInt(), 0);
        const QVariantList rects = mark.value("rects").toList();
        QVERIFY(!rects.isEmpty());
        const QPointF centre = rects.first().toRectF().center();
        QCOMPARE(s.session.wordAt(0, centre).value("text").toString(), QStringLiteral("ALPHA"));
        QCOMPARE(n.text, written); // drawing never touches the notes
        // The second page's pin is untouched.
        QCOMPARE(find(n, "note", 1).value("x").toInt(), 300);
        // Undo puts everything back.
        QVERIFY(s.session.undoPageEdit());
        QCOMPARE(find(n, "note", 0).value("x").toInt(), 100);
        QCOMPARE(find(n, "note", 0).value("y").toInt(), 200);
    }
    // Move and insert-before: the items follow their page.
    Cycle165Session s;
    Cycle165Notes n(&s.session);
    int start = 0;
    QVERIFY(s.open(cycle165Marked(3)));
    QVERIFY(build(s, n, &start, QStringLiteral("PAGEA"))); // text only page 1 has (the same word nearby would re-anchor)
    QVERIFY(s.session.movePages({0}, 3)); // page 1 to the end
    QCOMPARE(find(n, "highlight").value("page").toInt(), 2);
    QCOMPARE(find(n, "note", 0).value("page").toInt(), 2);
    QCOMPARE(find(n, "note", 1).value("page").toInt(), 0); // baseline page 2 is now first
    QUrl other;
    QVERIFY(s.openOther(cycle165Marked(1, 5), &other));
    QVERIFY(s.session.insertPdf(other, 0));
    QCOMPARE(find(n, "highlight").value("page").toInt(), 3);
    QCOMPARE(find(n, "note", 1).value("page").toInt(), 1);
    QVERIFY(find(n, "highlight").value("placed").toBool());
    // A removed page: its items are not drawn and are listed as unplaced.
    QVERIFY(s.session.removePages({3}));
    const QVariantMap gone = find(n, "highlight");
    QVERIFY(!gone.value("placed").toBool());
    QVERIFY(gone.value("rects").toList().isEmpty());
    QVERIFY(!find(n, "note", 0).value("placed").toBool());
    QVERIFY(find(n, "note", 1).value("placed").toBool());
    int unplaced = 0;
    for (const QVariant &v : n.items()) unplaced += !v.toMap().value("placed").toBool();
    QCOMPARE(unplaced, 2);
    QVERIFY(s.session.undoPageEdit());
    QVERIFY(find(n, "highlight").value("placed").toBool());
}

void FomawriteTest::pdfPagesMarkingWhilePendingWritesBaselineNumbers() {
    Cycle165Session s;
    Cycle165Notes n(&s.session);
    QVERIFY(s.open(cycle165Marked(3)));
    const int alpha = s.session.pageText(0).indexOf(QStringLiteral("ALPHA"));
    // Working order C, A, B with C turned 90 degrees.
    QVERIFY(s.session.movePages({2}, 0));
    QVERIFY(s.session.rotatePages({0}, 90));
    QCOMPARE(n.marks.mark(QStringLiteral("highlight"), QStringLiteral("green"), 0, alpha, 5), QStringLiteral("added"));
    QVERIFY(n.text.contains(QStringLiteral("page=3 ")));
    QVERIFY(!n.text.contains(QStringLiteral("page=1 ")));
    QVERIFY(n.text.contains(QStringLiteral("## [Page 3](doc.pdf#page=3)")));
    // The same mark again removes it (found by its baseline page).
    QCOMPARE(n.marks.mark(QStringLiteral("highlight"), QStringLiteral("green"), 0, alpha, 5), QStringLiteral("removed"));
    QVERIFY(PdfNotes::parse(n.text).items.isEmpty());
    // A pin placed at working (592, 100) on the turned page is stored at baseline (100, 200).
    QVERIFY(n.marks.placeNote(0, 592, 100) >= 0);
    QVERIFY(n.text.contains(QStringLiteral("note page=3 at=100,200")));
    // Moving it by its working position writes baseline coordinates again.
    QVERIFY(n.marks.moveNote(n.item(0).value("index").toInt(), 552, 300));
    QVERIFY(n.text.contains(QStringLiteral("note page=3 at=300,240")));
    QCOMPARE(n.item(0).value("x").toInt(), 552);
    QCOMPARE(n.item(0).value("y").toInt(), 300);
    // A mark on the plainly moved page A is page 1.
    QCOMPARE(n.marks.mark(QStringLiteral("underline"), QString(), 1, alpha, 5), QStringLiteral("added"));
    QVERIFY(n.text.contains(QStringLiteral("underline page=1 ")));

    // A page inserted from another file cannot be marked until the PDF is saved.
    QUrl other;
    QVERIFY(s.openOther(cycle165Marked(1, 5), &other));
    QVERIFY(s.session.insertPdf(other, 0));
    const QString before = n.text;
    QVERIFY(n.marks.mark(QStringLiteral("highlight"), QString(), 0, alpha, 5).isEmpty());
    QCOMPARE(s.session.pageEditError(), QStringLiteral("Save the PDF before marking inserted pages."));
    QCOMPARE(n.marks.placeNote(0, 10, 10), -1);
    QCOMPARE(n.text, before);
    // Other pages still mark, with baseline numbers (working 2 is baseline page 1).
    QVERIFY(s.session.pageEditError().size() > 0);
    QCOMPARE(n.marks.mark(QStringLiteral("strike"), QString(), 2, alpha, 5), QStringLiteral("added"));
    QVERIFY(n.text.contains(QStringLiteral("strike page=1 ")));
}

void FomawriteTest::pdfPagesLinksScrollToTheTranslatedPage() {
    Cycle165Session s;
    QVERIFY(s.open(cycle165Marked(4)));
    // No pending edits: as before (the page, clamped).
    s.session.requestBaselinePage(2);
    QCOMPARE(s.session.requestedPage(), 2);
    s.session.requestBaselinePage(99);
    QCOMPARE(s.session.requestedPage(), 3);
    QVERIFY(s.session.movePages({3}, 0));    // D A B C
    QVERIFY(s.session.removePages({2}));     // D A C
    s.session.requestBaselinePage(2);        // baseline page 3 (C) is now last
    QCOMPARE(s.session.requestedPage(), 2);
    s.session.requestBaselinePage(0);
    QCOMPARE(s.session.requestedPage(), 1);
    s.session.requestBaselinePage(3);
    QCOMPARE(s.session.requestedPage(), 0);
    s.session.setRequestedPage(1);
    s.session.requestBaselinePage(1);        // baseline page 2 (B) was removed: nothing happens
    QCOMPARE(s.session.requestedPage(), 1);
    // A link and a quote made now name the baseline page; an untouched session still names the page itself.
    QCOMPARE(s.session.pageLinkAddress(0), QStringLiteral("doc.pdf#page=4"));
    QCOMPARE(s.session.pageLinkAddress(2), QStringLiteral("doc.pdf#page=3"));
    QVERIFY(s.session.markdownQuoteFor(0, QStringLiteral("text")).contains(QStringLiteral("[Page 4](doc.pdf#page=4)")));
    QVERIFY(s.session.undoPageEdit() && s.session.undoPageEdit());
    QCOMPARE(s.session.pageLinkAddress(3), QStringLiteral("doc.pdf#page=4"));
    QCOMPARE(s.session.pageLinkAddress(0), QStringLiteral("doc.pdf#page=1"));
}
