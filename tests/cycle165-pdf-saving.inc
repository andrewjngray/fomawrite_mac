// Cycle 165 part 2b — saving page edits (Backend::savePdf): the conflict check, the verified write, the recovery
// copy, the atomic replace, the session adopting the saved file, the notes renumbered and saved with the PDF,
// undo across the save boundary, and Backend::exportPdfPages. No user interface yet.

#include "pdffiles.h"

namespace {

struct Cycle165Disk {
    int count = 0;
    QStringList texts;
    QList<bool> landscape;
};
Cycle165Disk cycle165ReadDisk(const QString &path) {
    Cycle165Disk out;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return out;
    QByteArray bytes = file.readAll();
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);
    QPdfDocument document;
    document.load(&buffer);
    out.count = document.status() == QPdfDocument::Status::Ready ? document.pageCount() : 0;
    for (int i = 0; i < out.count; ++i) {
        out.texts << document.getAllText(i).text().trimmed().left(5);
        out.landscape << (document.pagePointSize(i).width() > document.pagePointSize(i).height());
    }
    return out;
}
QByteArray cycle165Bytes(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// A Backend with a Source editor and a PDF (and optionally existing notes) in a temporary folder.
struct Cycle165Save {
    QTemporaryDir directory;
    Cycle162Draft draft;
    QString path, notes;
    Backend *backend() { return draft.backend.get(); }
    PdfSession *pdf() { return draft.backend->pdf(); }
    PdfMarks *marks() { return draft.backend->pdfMarks(); }
    QString text() { return draft.editor->property("text").toString(); }
    bool open(const QByteArray &bytes, const QString &notesText = QString(), const QString &name = QStringLiteral("doc.pdf")) {
        path = directory.filePath(name);
        notes = path + QStringLiteral(".md");
        if (!directory.isValid() || !draft.create() || !PdfFixtures::write(path, bytes)) return false;
        if (!notesText.isNull()) {
            QFile file(notes);
            if (!file.open(QIODevice::WriteOnly) || file.write(notesText.toUtf8()) < 0) return false;
        }
        return backend()->open(QUrl::fromLocalFile(path)) && pdf()->state() == QLatin1String("ready");
    }
    QStringList files() { return QDir(directory.path()).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name); }
    QStringList texts() {
        QStringList out;
        for (int i = 0; i < pdf()->pageCount(); ++i) out << pdf()->pageText(i).trimmed().left(5);
        return out;
    }
    // A highlight over ALPHA and a pin at (100, 200) on a working page.
    void markPage(int page) {
        const int alpha = pdf()->pageText(page).indexOf(QStringLiteral("ALPHA"));
        marks()->mark(QStringLiteral("highlight"), QStringLiteral("yellow"), page, alpha, 5);
        marks()->placeNote(page, 100, 200);
    }
    // What the pane draws: every item's kind, page, placement, pin position and text rectangles.
    QList<QVariantMap> drawn() {
        marks()->refresh();
        QList<QVariantMap> out;
        for (const QVariant &v : marks()->items()) {
            const QVariantMap m = v.toMap();
            QVariantMap keep;
            for (const char *key : {"kind", "page", "placed", "x", "y", "rects"}) keep.insert(QLatin1String(key), m.value(QLatin1String(key)));
            out << keep;
        }
        return out;
    }
    QStringList recoveryCopies() { return PdfFiles::recoveryCopies(backend()->pdfRecoveryFolder(), path); }
    ~Cycle165Save() {
        draft.backend->discardRecovery();
        QDir(draft.backend->pdfRecoveryFolder()).removeRecursively();
    }
};
QUrl cycle165WriteOther(Cycle165Save &s, const QByteArray &bytes, const QString &name = QStringLiteral("other.pdf")) {
    PdfFixtures::write(s.directory.filePath(name), bytes);
    return QUrl::fromLocalFile(s.directory.filePath(name));
}
}

void FomawriteTest::pdfSavingWritesEachOperationAndKeepsThePreviousBytes() {
    struct Op {
        const char *name;
        std::function<bool(Cycle165Save &)> run;
        QStringList texts;
        QList<bool> landscape;
    };
    const QList<Op> ops = {
        {"rotate", [](Cycle165Save &s) { return s.pdf()->rotatePages({1}, 90); }, {"PAGEA", "PAGEB", "PAGEC"}, {false, true, false}},
        {"remove", [](Cycle165Save &s) { return s.pdf()->removePages({1}); }, {"PAGEA", "PAGEC"}, {false, false}},
        {"move", [](Cycle165Save &s) { return s.pdf()->movePages({2}, 0); }, {"PAGEC", "PAGEA", "PAGEB"}, {false, false, false}},
        {"insert", [](Cycle165Save &s) { return s.pdf()->insertPdf(cycle165WriteOther(s, cycle165Marked(1, 5)), 1); },
         {"PAGEA", "PAGEF", "PAGEB", "PAGEC"}, {false, false, false, false}},
    };
    for (const Op &op : ops) {
        QVERIFY2(true, op.name);
        Cycle165Save s;
        const QByteArray original = cycle165Marked(3);
        QVERIFY(s.open(original));
        // Nothing pending: nothing happens.
        QVERIFY(s.backend()->savePdf());
        QCOMPARE(cycle165Bytes(s.path), original);
        QVERIFY(s.recoveryCopies().isEmpty());
        QVERIFY2(op.run(s), op.name);
        QVERIFY(s.pdf()->pagesModified());
        const QByteArray working = s.pdf()->workingBytes();
        QVERIFY2(s.backend()->savePdf(), qPrintable(s.backend()->pdfSaveError()));
        QVERIFY(s.backend()->pdfSaveError().isEmpty());
        const Cycle165Disk disk = cycle165ReadDisk(s.path);
        QCOMPARE(disk.texts, op.texts);
        QCOMPARE(disk.landscape, op.landscape);
        QCOMPARE(cycle165Bytes(s.path), working);
        QCOMPARE(s.texts(), op.texts);
        QVERIFY(!s.pdf()->pagesModified());
        QCOMPARE(s.pdf()->baselineBytes(), working);
        QCOMPARE(s.recoveryCopies().size(), 1);
        QCOMPARE(cycle165Bytes(s.recoveryCopies().first()), original);
        QStringList files = s.files();
        files.removeAll(QStringLiteral("other.pdf")); // the file the insert read
        QCOMPARE(files, QStringList{QStringLiteral("doc.pdf")}); // no temporary file left
        // Saved again with nothing pending: still nothing.
        QVERIFY(s.backend()->savePdf());
        QCOMPARE(s.recoveryCopies().size(), 1);
    }
}

void FomawriteTest::pdfSavingKeepsTheLatestRecoveryCopiesOfEachFile() {
    Cycle165Save s;
    const QByteArray original = cycle165Marked(2);
    QVERIFY(s.open(original));
    QList<QByteArray> before;
    for (int i = 0; i < 5; ++i) {
        before << cycle165Bytes(s.path);
        QVERIFY(s.pdf()->rotatePages({0}, 90));
        QVERIFY2(s.backend()->savePdf(), qPrintable(s.backend()->pdfSaveError()));
    }
    const QStringList copies = s.recoveryCopies(); // newest first
    QCOMPARE(copies.size(), 3);
    for (int i = 0; i < 3; ++i) QCOMPARE(cycle165Bytes(copies.at(i)), before.at(4 - i));
    // Another path has its own (here: no) copies.
    QVERIFY(PdfFiles::recoveryCopies(s.backend()->pdfRecoveryFolder(), s.directory.filePath("elsewhere.pdf")).isEmpty());
}

void FomawriteTest::pdfSavingFailuresLeaveEverythingAsItWas() {
    for (const QString &stage : {QStringLiteral("write"), QStringLiteral("verify"), QStringLiteral("recovery"), QStringLiteral("replace")}) {
        Cycle165Save s;
        const QByteArray original = cycle165Marked(3);
        QVERIFY(s.open(original));
        s.markPage(0);
        s.markPage(1);
        const QString notes = s.text();
        const QByteArray notesOnDisk = cycle165Bytes(s.notes);
        QVERIFY(s.pdf()->rotatePages({0}, 90));
        QVERIFY(s.pdf()->movePages({2}, 0));
        QVERIFY(s.pdf()->rotatePages({0}, 90));
        QVERIFY(s.pdf()->undoPageEdit());
        const QByteArray working = s.pdf()->workingBytes();
        const QStringList filesBefore = s.files();
        s.backend()->failNextPdfWrite(stage);
        QVERIFY2(!s.backend()->savePdf(), qPrintable(stage));
        QVERIFY2(!s.backend()->pdfSaveError().isEmpty(), qPrintable(stage));
        QCOMPARE(cycle165Bytes(s.path), original);
        QCOMPARE(s.files(), filesBefore);
        QVERIFY(s.recoveryCopies().isEmpty() || stage == QLatin1String("never"));
        QVERIFY(s.pdf()->pagesModified());
        QVERIFY(s.pdf()->canUndoPageEdit());
        QVERIFY(s.pdf()->canRedoPageEdit());
        QVERIFY(s.draft.editor->property("canUndo").toBool());
        QCOMPARE(s.pdf()->workingBytes(), working);
        QCOMPARE(s.pdf()->baselineBytes(), original);
        QCOMPARE(s.text(), notes);
        QCOMPARE(cycle165Bytes(s.notes), notesOnDisk);
        // The failure was one use: the same save now works.
        QVERIFY2(s.backend()->savePdf(), qPrintable(s.backend()->pdfSaveError()));
        QCOMPARE(cycle165Bytes(s.path), working);
        QVERIFY(s.backend()->pdfSaveError().isEmpty());
    }
#ifndef Q_OS_WIN
    // A folder that cannot be written to (real, not injected).
    Cycle165Save s;
    const QByteArray original = cycle165Marked(2);
    QVERIFY(s.open(original));
    QVERIFY(s.pdf()->rotatePages({0}, 90));
    const QFile::Permissions before = QFile::permissions(s.directory.path());
    QVERIFY(QFile::setPermissions(s.directory.path(), QFile::ReadOwner | QFile::ExeOwner));
    const bool saved = s.backend()->savePdf();
    QFile::setPermissions(s.directory.path(), before);
    QVERIFY(!saved);
    QVERIFY(!s.backend()->pdfSaveError().isEmpty());
    QCOMPARE(cycle165Bytes(s.path), original);
    QCOMPARE(s.files(), QStringList{QStringLiteral("doc.pdf")});
    QVERIFY(s.pdf()->pagesModified());
#endif
}

void FomawriteTest::pdfSavingVerifiesTheFileItWrote() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("a.pdf");
    const QByteArray original = cycle165Marked(2);
    QVERIFY(PdfFixtures::write(path, original));
    PdfFiles::Options options;
    options.path = path;
    options.bytes = cycle165Marked(3);
    options.expectedPages = 2; // the bytes have three pages
    const QString error = PdfFiles::writeVerified(options);
    QVERIFY(error.contains(QStringLiteral("could not be verified")));
    QVERIFY(error.contains(QStringLiteral("3 pages instead of 2")));
    QCOMPARE(cycle165Bytes(path), original);
    options.bytes = QByteArray("not a pdf at all");
    options.expectedPages = 0;
    QVERIFY(PdfFiles::writeVerified(options).contains(QStringLiteral("could not be verified")));
    QCOMPARE(cycle165Bytes(path), original);
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot), QStringList{QStringLiteral("a.pdf")});
    options.bytes = cycle165Marked(2, 3);
    options.expectedPages = 2;
    QVERIFY(PdfFiles::writeVerified(options).isEmpty());
    QCOMPARE(cycle165Bytes(path), options.bytes);
    // Permissions survive the replace.
#ifndef Q_OS_WIN
    QVERIFY(QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup));
    options.bytes = cycle165Marked(2, 5);
    QVERIFY(PdfFiles::writeVerified(options).isEmpty());
    QCOMPARE(QFile::permissions(path) & (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther), QFile::ReadGroup);
#endif
}

void FomawriteTest::pdfSavingRefusesAFileThatChangedOnDisk() {
    Cycle165Save s;
    const QByteArray original = cycle165Marked(3);
    QVERIFY(s.open(original));
    QVERIFY(s.pdf()->rotatePages({0}, 90));
    const QByteArray working = s.pdf()->workingBytes();
    const QByteArray other = cycle165Marked(5);
    QVERIFY(PdfFixtures::write(s.path, other));
    QVERIFY(!s.backend()->savePdf());
    QCOMPARE(s.backend()->pdfSaveError(), QStringLiteral("The PDF changed on disk. Your page changes were not saved."));
    QCOMPARE(cycle165Bytes(s.path), other);
    QCOMPARE(s.files(), QStringList{QStringLiteral("doc.pdf")});
    QVERIFY(s.recoveryCopies().isEmpty());
    QVERIFY(s.pdf()->pagesModified());
    QCOMPARE(s.pdf()->workingBytes(), working);
    QTRY_VERIFY_WITH_TIMEOUT(s.pdf()->changedOnDiskWhileEditing(), 5000);
    // The writer chooses to overwrite: the other version is kept as the recovery copy.
    QVERIFY2(s.backend()->savePdf(true), qPrintable(s.backend()->pdfSaveError()));
    QCOMPARE(cycle165Bytes(s.path), working);
    QCOMPARE(s.recoveryCopies().size(), 1);
    QCOMPARE(cycle165Bytes(s.recoveryCopies().first()), other);
    QVERIFY(!s.pdf()->changedOnDiskWhileEditing());
    QVERIFY(!s.pdf()->pagesModified());
    // A file that is gone is refused too.
    QVERIFY(s.pdf()->rotatePages({1}, 90));
    QVERIFY(QFile::remove(s.path));
    QVERIFY(!s.backend()->savePdf());
    QVERIFY(!s.backend()->pdfSaveError().isEmpty());
    QVERIFY(!QFileInfo::exists(s.path));
    QVERIFY(s.pdf()->pagesModified());
}

void FomawriteTest::pdfSavingDoesNotReloadThePdf() {
    Cycle165Save s;
    QVERIFY(s.open(cycle165Marked(3)));
    QVERIFY(s.pdf()->rotatePages({0}, 90));
    QVERIFY(s.pdf()->movePages({2}, 0));
    QVERIFY(s.backend()->savePdf());
    const int revision = s.pdf()->revision();
    QTest::qWait(1200); // well past the watcher's debounce
    QCOMPARE(s.pdf()->revision(), revision);
    QVERIFY(!s.pdf()->changedOnDiskWhileEditing());
    QVERIFY(!s.pdf()->pagesModified());
    QCOMPARE(s.pdf()->state(), QStringLiteral("ready"));
    // The file is still watched: a later outside change is noticed.
    QVERIFY(PdfFixtures::write(s.path, cycle165Marked(4)));
    QTRY_COMPARE_WITH_TIMEOUT(s.pdf()->pageCount(), 4, 5000);
}

void FomawriteTest::pdfSavingResetsPageHistoryAndKeepsUnchangedNotesHistory() {
    Cycle165Save s;
    const QString notes = QStringLiteral("Plain notes.\n");
    QVERIFY(s.open(cycle165Marked(3), notes));
    const auto type = [&](const QString &text) {
        return QMetaObject::invokeMethod(s.draft.editor.data(), "insert",
                                         Q_ARG(int, 0), Q_ARG(QString, text));
    };
    QVERIFY(type(QStringLiteral("First. ")));
    QVERIFY(type(QStringLiteral("Second. ")));
    QVERIFY(QMetaObject::invokeMethod(s.draft.editor.data(), "undo"));
    const QString beforeSave = s.text();
    QVERIFY(beforeSave != notes);
    QVERIFY(s.draft.editor->property("canUndo").toBool());
    QVERIFY(s.draft.editor->property("canRedo").toBool());

    QVERIFY(s.pdf()->removePages({1}));
    const QByteArray saved = s.pdf()->workingBytes();
    QVERIFY(s.pdf()->rotatePages({0}, 90));
    QVERIFY(s.pdf()->undoPageEdit());
    QVERIFY(s.pdf()->canUndoPageEdit());
    QVERIFY(s.pdf()->canRedoPageEdit());
    QVERIFY2(s.backend()->savePdf(), qPrintable(s.backend()->pdfSaveError()));
    QCOMPARE(cycle165Bytes(s.path), saved);
    QVERIFY(!s.pdf()->canUndoPageEdit());
    QVERIFY(!s.pdf()->canRedoPageEdit());
    QCOMPARE(s.text(), beforeSave);
    QVERIFY(s.draft.editor->property("canUndo").toBool());
    QVERIFY(s.draft.editor->property("canRedo").toBool());
    QVERIFY(QMetaObject::invokeMethod(s.draft.editor.data(), "undo"));
    QCOMPARE(s.text(), notes);
    QVERIFY(QMetaObject::invokeMethod(s.draft.editor.data(), "redo"));
    QCOMPARE(s.text(), beforeSave);

    QVERIFY(s.pdf()->rotatePages({0}, 90));
    QVERIFY(s.pdf()->undoPageEdit());
    QCOMPARE(s.pdf()->workingBytes(), saved);
    QVERIFY(!s.pdf()->pagesModified());
    // A no-op Save did not write a new baseline or clear the pending Redo.
    QVERIFY(s.backend()->savePdf());
    QVERIFY(s.pdf()->redoPageEdit());
}

void FomawriteTest::pdfSavingRenumbersTheNotesAndMarksStayOnTheirContent() {
    struct Case {
        const char *name;
        std::function<bool(Cycle165Save &)> edit;
        QList<int> pages; // each item's page in the saved notes, in document order (0: removed)
    };
    const QList<Case> cases = {
        {"rotate 90", [](Cycle165Save &s) { return s.pdf()->rotatePages({0}, 90); }, {1, 1, 2, 2, 3, 3}},
        {"rotate 180", [](Cycle165Save &s) { return s.pdf()->rotatePages({0}, 180); }, {1, 1, 2, 2, 3, 3}},
        {"rotate 270", [](Cycle165Save &s) { return s.pdf()->rotatePages({0}, 270); }, {1, 1, 2, 2, 3, 3}},
        {"move", [](Cycle165Save &s) { return s.pdf()->movePages({0}, 3); }, {3, 3, 1, 1, 2, 2}},
        {"remove", [](Cycle165Save &s) { return s.pdf()->removePages({1}); }, {1, 1, 0, 0, 2, 2}},
        {"insert before", [](Cycle165Save &s) { return s.pdf()->insertPdf(cycle165WriteOther(s, cycle165Marked(2, 5)), 0); }, {3, 3, 4, 4, 5, 5}},
        {"combined", [](Cycle165Save &s) {
             return s.pdf()->movePages({2}, 0) && s.pdf()->rotatePages({1}, 90) && s.pdf()->removePages({2})
                 && s.pdf()->insertPdf(cycle165WriteOther(s, cycle165Marked(1, 5)), 1);
         }, {3, 3, 0, 0, 1, 1}},
    };
    for (const Case &c : cases) {
        Cycle165Save s;
        QVERIFY(s.open(cycle165Marked(3)));
        for (int page = 0; page < 3; ++page) s.markPage(page);
        const QString before = s.text();
        QVERIFY2(c.edit(s), c.name);
        const QList<QVariantMap> drawnBefore = s.drawn();
        QVERIFY(s.backend()->savePdf());
        QVERIFY2(s.backend()->pdfSaveError().isEmpty(), c.name);
        const PdfNotes::Document parsed = PdfNotes::parse(s.text());
        QList<int> pages;
        for (const PdfNotes::Item &item : parsed.items) pages << item.page;
        QCOMPARE(pages, c.pages);
        QVERIFY2(s.text() != before, c.name);
        const QList<QVariantMap> after = s.drawn();
        QCOMPARE(after.size(), drawnBefore.size());
        for (int i = 0; i < after.size(); ++i) {
            if (c.pages.at(i) == 0) { // a removed page's items are unplaced, listed without a page
                QVERIFY2(!after.at(i).value("placed").toBool(), c.name);
                QCOMPARE(after.at(i).value("page").toInt(), -1);
                continue;
            }
            QVERIFY2(after.at(i) == drawnBefore.at(i), qPrintable(QStringLiteral("%1, item %2").arg(c.name).arg(i)));
            QVERIFY2(after.at(i).value("placed").toBool(), c.name);
            if (after.at(i).value("kind").toString() == QLatin1String("highlight")) { // still over ALPHA
                const QPointF centre = after.at(i).value("rects").toList().first().toRectF().center();
                QCOMPARE(s.pdf()->wordAt(after.at(i).value("page").toInt(), centre).value("text").toString(), QStringLiteral("ALPHA"));
            }
        }
    }
    // The pin of a turned page, as text.
    for (const auto &e : {std::pair<int, const char *>{90, "at=592,100"}, {180, "at=512,592"}, {270, "at=200,512"}}) {
        Cycle165Save s;
        QVERIFY(s.open(cycle165Marked(2)));
        s.markPage(0);
        QVERIFY(s.pdf()->rotatePages({0}, e.first));
        QVERIFY(s.backend()->savePdf());
        QVERIFY2(s.text().contains(QLatin1String(e.second)), e.second);
        QVERIFY(s.text().contains(QStringLiteral("page=1 at=")));
        QVERIFY(s.text().contains(QStringLiteral("text="))); // the highlight's range is unchanged
    }
}

void FomawriteTest::pdfSavingRewritesHeadingsAndRemovedPagesAndKeepsEverythingElse() {
    const auto remap = [](std::initializer_list<std::tuple<int, int>> entries) {
        QList<PdfNotes::PageRemap> pages;
        for (const auto &e : entries) pages.append({std::get<0>(e), std::get<1>(e), QSizeF(612, 792)});
        return pages;
    };
    const auto none = [](int) { return QString(); };
    const QString text = QStringLiteral(
        "<!-- fomawrite:pdf-notes 1 -->\n# Notes on doc.pdf\n\n"
        "## [Page 1](doc.pdf#page=1)\n\n<!-- fomawrite:pdf note page=1 at=100,200 -->\nPin. See [Page 2](doc.pdf#page=2) and [elsewhere](other.pdf#page=2).\n\n"
        "## [Page ii](doc.pdf#page=2)\n\n<!-- fomawrite:pdf highlight page=2 colour=yellow text=3+4 -->\n> abcd\n\n"
        "<!-- fomawrite:pdf note page=2 at=100,200 -->\nTurned.\n<!-- fomawrite:pdf rect page=2 rect=10,20,110,70 -->\n\n"
        "## [My budget](doc.pdf#page=3)\n\n<!-- fomawrite:pdf note page=3 at=5,6 -->\nKept text.\n\n"
        "## [Page 4](doc.pdf#page=4)\n\n<!-- fomawrite:pdf note page=4 at=7,8 -->\n\n"
        "```\n<!-- fomawrite:pdf note page=1 at=100,200 -->\n[Page 1](doc.pdf#page=1)\n```\n");
    QString expected = text;
    const auto change = [&](const QString &from, const QString &to) { QVERIFY2(expected.contains(from), qPrintable(from)); expected.replace(from, to); };
    // old 1 -> new 2; old 2 -> new 1 turned 90 degrees; old 3 and old 4 removed.
    const auto pages = remap({{2, 0}, {1, 90}, {0, 0}, {0, 0}});
    const auto oldLabel = [](int page) { return page == 2 ? QStringLiteral("ii") : QString(); };
    const auto newLabel = [](int page) { return page == 1 ? QStringLiteral("i") : QString(); };
    const PdfNotes::Edit edit = PdfNotes::renumber(text, QStringLiteral("doc.pdf"), pages, oldLabel, newLabel);
    // Written in the order the document has them (the code fence at the end is not touched).
    change(QStringLiteral("## [Page 1](doc.pdf#page=1)\n\n<!-- fomawrite:pdf note page=1 at=100,200 -->\nPin. See [Page 2](doc.pdf#page=2)"),
           QStringLiteral("## [Page 2](doc.pdf#page=2)\n\n<!-- fomawrite:pdf note page=2 at=100,200 -->\nPin. See [Page i](doc.pdf#page=1)"));
    change(QStringLiteral("## [Page ii](doc.pdf#page=2)"), QStringLiteral("## [Page i](doc.pdf#page=1)"));
    change(QStringLiteral("highlight page=2 colour"), QStringLiteral("highlight page=1 colour"));
    change(QStringLiteral("note page=2 at=100,200 -->\nTurned"), QStringLiteral("note page=1 at=592,100 -->\nTurned"));
    change(QStringLiteral("rect page=2 rect=10,20,110,70"), QStringLiteral("rect page=1 rect=722,10,772,110"));
    change(QStringLiteral("## [My budget](doc.pdf#page=3)"), QStringLiteral("## [My budget](doc.pdf#page=0)"));
    change(QStringLiteral("note page=3 at=5,6"), QStringLiteral("note page=0 at=5,6"));
    change(QStringLiteral("## [Page 4](doc.pdf#page=4)"), QStringLiteral("## Deleted page 4"));
    change(QStringLiteral("note page=4 at=7,8"), QStringLiteral("note page=0 at=7,8"));
    QCOMPARE(PdfNotes::apply(text, edit), expected);
    QVERIFY(PdfNotes::apply(text, edit).contains(QStringLiteral("Kept text.")));
    // Windows line breaks are kept.
    QString crlf = text;
    crlf.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
    QString crlfExpected = expected;
    crlfExpected.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
    QCOMPARE(PdfNotes::apply(crlf, PdfNotes::renumber(crlf, QStringLiteral("doc.pdf"), pages, oldLabel, newLabel)), crlfExpected);
    // The parser sees page 0 as never placed.
    const PdfNotes::Document parsed = PdfNotes::parse(PdfNotes::apply(text, edit));
    int zero = 0;
    for (const PdfNotes::Item &item : parsed.items) {
        if (item.page != 0) continue;
        ++zero;
        QVERIFY(!PdfNotes::place(item, [](int) { return QStringLiteral("abcdefgh"); }).placed);
    }
    QCOMPARE(zero, 2);
    // Identity map: nothing is edited. A rotation of the same page with the same number only moves its points.
    QVERIFY(PdfNotes::renumber(text, QStringLiteral("doc.pdf"), remap({{1, 0}, {2, 0}, {3, 0}, {4, 0}}), oldLabel, newLabel).isEmpty());
    // The file name in the links must be this PDF's (percent-decoded).
    const QString spaced = QStringLiteral("## [Page 1](My%20Report.pdf#page=1)\n\n<!-- fomawrite:pdf note page=1 at=1,2 -->\n");
    QCOMPARE(PdfNotes::apply(spaced, PdfNotes::renumber(spaced, QStringLiteral("My Report.pdf"), remap({{3, 0}}), none, none)),
             QStringLiteral("## [Page 3](My%20Report.pdf#page=3)\n\n<!-- fomawrite:pdf note page=3 at=1,2 -->\n"));
}

void FomawriteTest::pdfSavingRenumbersAndClearsSourceHistoryAndKeepsWindowsLineBreaks() {
    Cycle165Save s;
    const QString notes = QStringLiteral(
        "<!-- fomawrite:pdf-notes 1 -->\r\n# Notes on doc.pdf\r\n\r\n## [Page 1](doc.pdf#page=1)\r\n\r\n"
        "<!-- fomawrite:pdf note page=1 at=100,200 -->\r\nFirst.\r\n\r\n## [Page 2](doc.pdf#page=2)\r\n\r\n"
        "<!-- fomawrite:pdf note page=2 at=10,20 -->\r\nSecond.\r\n");
    QVERIFY(s.open(cycle165Marked(2), notes));
    QCOMPARE(cycle165Bytes(s.notes), notes.toUtf8());
    QVERIFY(QMetaObject::invokeMethod(s.draft.editor.data(), "insert",
                                     Q_ARG(int, s.text().size()), Q_ARG(QString, QStringLiteral("Later writing."))));
    QVERIFY(QMetaObject::invokeMethod(s.draft.editor.data(), "undo"));
    QVERIFY(s.draft.editor->property("canRedo").toBool());
    const QString before = s.text();
    QVERIFY(s.pdf()->movePages({0}, 2));
    QVERIFY2(s.backend()->savePdf(), qPrintable(s.backend()->pdfSaveError()));
    const QString after = s.text();
    QVERIFY(after != before);
    // Notes are saved with the PDF, in the file's own line breaks, and nothing is left unsaved.
    QVERIFY(!s.backend()->modified());
    const QString expected = QStringLiteral(
        "<!-- fomawrite:pdf-notes 1 -->\r\n# Notes on doc.pdf\r\n\r\n## [Page 2](doc.pdf#page=2)\r\n\r\n"
        "<!-- fomawrite:pdf note page=2 at=100,200 -->\r\nFirst.\r\n\r\n## [Page 1](doc.pdf#page=1)\r\n\r\n"
        "<!-- fomawrite:pdf note page=1 at=10,20 -->\r\nSecond.\r\n");
    QCOMPARE(cycle165Bytes(s.notes), expected.toUtf8());
    QVERIFY(!cycle165Bytes(s.notes).contains("\n\n"));
    QVERIFY(!s.draft.editor->property("canUndo").toBool());
    QVERIFY(!s.draft.editor->property("canRedo").toBool());
    s.backend()->undoSource(0);
    s.backend()->redoSource(0);
    QCOMPARE(s.text(), after);
    QVERIFY(QMetaObject::invokeMethod(s.draft.editor.data(), "insert",
                                     Q_ARG(int, 0), Q_ARG(QString, QStringLiteral("New writing. "))));
    QCOMPARE(s.text(), QStringLiteral("New writing. ") + after);
    s.backend()->undoSource(0);
    QCOMPARE(s.text(), after);
}

void FomawriteTest::pdfSavingResetsLiveNotesHistoryOnlyWhenReferencesChange() {
    for (const bool renumber : {false, true}) {
        Cycle103SettingsSnapshot settings;
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        Cycle134Page page;
        QVERIFY2(page.create(), qPrintable(page.error));
        QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
        const QString pdfPath = temporary.filePath(QStringLiteral("doc.pdf"));
        QVERIFY(PdfFixtures::write(pdfPath, cycle165Marked(2)));
        const QString original = renumber
            ? QStringLiteral("Notes.\n\n<!-- fomawrite:pdf note page=1 at=100,200 -->\nPin text.\n")
            : QStringLiteral("Notes.\n");
        QFile file(pdfPath + QStringLiteral(".md"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(original.toUtf8()), original.toUtf8().size());
        file.close();
        QVERIFY(page.backend.open(QUrl::fromLocalFile(pdfPath)));
        page.backend.syncLiveEditor();
        QCOMPARE(page.pageText(), original);

        const QString insert = QStringLiteral("[{\"from\":0,\"to\":0,\"insert\":\"Draft. \"}]");
        page.bridge()->injectUserChanges(insert);
        const QString beforeSave = QStringLiteral("Draft. ") + original;
        QTRY_COMPARE(page.canonical(), beforeSave);
        QVERIFY(page.backend.pdf()->movePages({0}, 2));
        page.backend.failNextPdfWrite(QStringLiteral("replace"));
        QVERIFY(!page.backend.savePdf());
        page.bridge()->requestUndo();
        QCOMPARE(page.pageText(), original);
        page.bridge()->requestRedo();
        QCOMPARE(page.pageText(), beforeSave);
        QTRY_COMPARE(page.canonical(), beforeSave);

        QSignalSpy resets(page.bridge(), &EditorBridge::setDocument);
        QVERIFY2(page.backend.savePdf(), qPrintable(page.backend.pdfSaveError()));
        QString saved = beforeSave;
        if (renumber) saved.replace(QStringLiteral("page=1"), QStringLiteral("page=2"));
        QCOMPARE(page.canonical(), saved);
        QCOMPARE(page.pageText(), saved);
        QCOMPARE(resets.count(), renumber ? 1 : 0);

        page.bridge()->requestUndo();
        QCOMPARE(page.pageText(), renumber ? saved : original);
        page.bridge()->requestRedo();
        QCOMPARE(page.pageText(), saved);
        page.bridge()->injectUserChanges(insert);
        QCOMPARE(page.pageText(), QStringLiteral("Draft. ") + saved);
        page.bridge()->requestUndo();
        QCOMPARE(page.pageText(), saved);
        page.backend.discardRecovery();
        QDir(page.backend.pdfRecoveryFolder()).removeRecursively();
    }
}

void FomawriteTest::pdfSavingSavesTheNotesDocumentWhenItHasATarget() {
    // Untitled notes with marks: written to doc.pdf.md.
    {
        Cycle165Save s;
        QVERIFY(s.open(cycle165Marked(3)));
        s.markPage(0);
        QVERIFY(!QFileInfo::exists(s.notes));
        QVERIFY(s.pdf()->movePages({0}, 3));
        QVERIFY(s.backend()->savePdf());
        QVERIFY(QFileInfo::exists(s.notes));
        QCOMPARE(cycle165Bytes(s.notes), s.text().toUtf8());
        QVERIFY(s.text().contains(QStringLiteral("page=3 ")));
        QVERIFY(!s.backend()->modified());
    }
    // Nothing in the notes: no notes file appears.
    {
        Cycle165Save s;
        QVERIFY(s.open(cycle165Marked(3)));
        QVERIFY(s.pdf()->movePages({0}, 3));
        QVERIFY(s.backend()->savePdf());
        QVERIFY(!QFileInfo::exists(s.notes));
        QVERIFY(s.text().isEmpty());
    }
    // An edit that renumbers nothing leaves a saved notes file alone.
    {
        Cycle165Save s;
        QVERIFY(s.open(cycle165Marked(3), QStringLiteral("Just words.\n")));
        const QByteArray bytes = cycle165Bytes(s.notes);
        QVERIFY(s.pdf()->rotatePages({1}, 90));
        QVERIFY(s.backend()->savePdf());
        QCOMPARE(cycle165Bytes(s.notes), bytes);
        QCOMPARE(s.text(), QStringLiteral("Just words.\n"));
    }
}

void FomawriteTest::pdfSavingKeepsMarkingIntoTheRightSectionAfterAReorder() {
    Cycle165Save s;
    QVERIFY(s.open(cycle165Marked(4)));
    for (int page = 0; page < 3; ++page) s.markPage(page);
    QVERIFY(s.pdf()->movePages({0}, 4)); // B C D A: the file's sections are now 4, 1, 2
    QVERIFY(s.backend()->savePdf());
    const PdfNotes::Document order = PdfNotes::parse(s.text());
    QList<int> headings;
    for (const PdfNotes::Section &section : order.sections) headings << section.page;
    QCOMPARE(headings, (QList<int>{4, 1, 2})); // nothing moved
    // A new mark on page 1 goes into page 1's existing section, after its items.
    const int alpha = s.pdf()->pageText(0).indexOf(QStringLiteral("OMEGA"));
    QCOMPARE(s.marks()->mark(QStringLiteral("highlight"), QStringLiteral("blue"), 0, alpha, 5), QStringLiteral("added"));
    PdfNotes::Document now = PdfNotes::parse(s.text());
    QCOMPARE(now.sections.size(), 3);
    const PdfNotes::Section &first = now.sections.at(1);
    QCOMPARE(first.page, 1);
    int inside = 0;
    for (const PdfNotes::Item &item : now.items)
        if (item.page == 1 && item.markerStart >= first.headingLineEnd && item.markerStart < first.end) ++inside;
    QCOMPARE(inside, 3); // the highlight, the pin and the new mark
    // A pin on a page without a section gets one, in a sensible place and never inside another section.
    QVERIFY(s.marks()->placeNote(2, 50, 60) >= 0);
    now = PdfNotes::parse(s.text());
    QCOMPARE(now.sections.size(), 4);
    for (const PdfNotes::Section &a : now.sections)
        for (const PdfNotes::Section &b : now.sections)
            QVERIFY(&a == &b || a.end <= b.headingStart || b.end <= a.headingStart);
    for (const PdfNotes::Item &item : now.items) {
        bool inSection = false;
        for (const PdfNotes::Section &section : now.sections)
            if (item.markerStart >= section.headingLineEnd && item.markerStart < section.end) inSection = section.page == item.page;
        QVERIFY2(inSection, qPrintable(QString::number(item.page)));
    }
}

void FomawriteTest::pdfSavingExportsPagesWithoutChangingTheSession() {
    Cycle165Save s;
    const QByteArray original = cycle165Marked(3);
    QVERIFY(s.open(original));
    s.markPage(0);
    QVERIFY(s.pdf()->rotatePages({2}, 90)); // pending, and exported as shown
    const QString notes = s.text();
    const QByteArray working = s.pdf()->workingBytes();
    const QString out = s.directory.filePath("part.pdf");
    QVERIFY2(s.backend()->exportPdfPages(QUrl::fromLocalFile(out), {2, 0}), qPrintable(s.backend()->pdfSaveError()));
    const Cycle165Disk part = cycle165ReadDisk(out);
    QCOMPARE(part.texts, (QStringList{"PAGEC", "PAGEA"}));
    QCOMPARE(part.landscape, (QList<bool>{true, false}));
    // Everything else is as it was.
    QCOMPARE(cycle165Bytes(s.path), original);
    QCOMPARE(s.pdf()->workingBytes(), working);
    QCOMPARE(s.pdf()->baselineBytes(), original);
    QVERIFY(s.pdf()->pagesModified() && s.pdf()->canUndoPageEdit());
    QCOMPARE(s.text(), notes);
    QVERIFY(!QFileInfo::exists(s.notes));
    QVERIFY(!QFileInfo::exists(out + QStringLiteral(".md")));
    QVERIFY(s.recoveryCopies().isEmpty());
    // An empty list is every working page, and writing over an existing file keeps its old bytes.
    const QByteArray partBytes = cycle165Bytes(out);
    QVERIFY(s.backend()->exportPdfPages(QUrl::fromLocalFile(out), {}));
    QCOMPARE(cycle165ReadDisk(out).texts, (QStringList{"PAGEA", "PAGEB", "PAGEC"}));
    const QStringList kept = PdfFiles::recoveryCopies(s.backend()->pdfRecoveryFolder(), out);
    QCOMPARE(kept.size(), 1);
    QCOMPARE(cycle165Bytes(kept.first()), partBytes);
    // Refusals and failures: the open PDF is never the target; a bad page or a failed write changes nothing.
    QVERIFY(!s.backend()->exportPdfPages(QUrl::fromLocalFile(s.path), {0}));
    QVERIFY(s.backend()->pdfSaveError().contains(QStringLiteral("different file")));
    QCOMPARE(cycle165Bytes(s.path), original);
    const QByteArray all = cycle165Bytes(out);
    QVERIFY(!s.backend()->exportPdfPages(QUrl::fromLocalFile(out), {7}));
    QVERIFY(!s.backend()->pdfSaveError().isEmpty());
    s.backend()->failNextPdfWrite(QStringLiteral("replace"));
    QVERIFY(!s.backend()->exportPdfPages(QUrl::fromLocalFile(out), {0}));
    QCOMPARE(cycle165Bytes(out), all);
    QCOMPARE(s.files(), (QStringList{"doc.pdf", "part.pdf"}));
}
