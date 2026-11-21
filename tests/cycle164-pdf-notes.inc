// Cycle 164 part 1 — note pins on the page and navigation between the page
// and the notes: the `note` marker (parse, insert, move, remove) on strings;
// placing a note by clicking the page (one undo step, caret and focus on the
// note's text line); pins on every fixture rotation; the popover, move and
// remove; a click on a mark or pin showing its item in the notes (Source and
// Live); the caret moving into another item showing it on the page; dragging
// a selection from a highlight still selecting; the Note tool, menu item and
// shortcut. Uses the cycle 160-163 fixtures and helpers.

namespace {
QString cycle164Marker(int page, int x, int y) {
    return QStringLiteral("<!-- fomawrite:pdf note page=%1 at=%2,%3 -->").arg(page).arg(x).arg(y);
}
QString cycle164Crlf(QString text) { return text.replace(QLatin1Char('\n'), QStringLiteral("\r\n")); }

// The note items of the marks list.
QVariantList cycle164Notes(Cycle161Pane &p) {
    QVariantList notes;
    for (const QVariant &item : cycle163Items(p))
        if (item.toMap().value("kind").toString() == QLatin1String("note")) notes << item;
    return notes;
}
QList<QQuickItem *> cycle164Pins(Cycle161Pane &p) { return cycle103VisualItems(p.list, QStringLiteral("pdfNotePin")); }
void cycle164Click(Cycle161Pane &p, int page, QPointF points) {
    // Wait for the real condition: the page is laid out, and the point stays put
    // inside the viewport across two looks (a slow machine finishes a layout
    // change late; a click at the old place would land on blank page).
    QPointF last(-1, -1);
    QTest::qWaitFor([&] {
        p.reveal(page, points);
        const QPointF now = p.scene(page, points);
        const bool stable = now == last && p.viewport().contains(now);
        last = now;
        return stable;
    }, 5000);
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(page, points).toPoint());
}
// The caret line of the Source editor is inside the notes' viewport.
bool cycle164CaretVisible(QQuickItem *editor) {
    const QRectF caret = editor->mapRectToScene(editor->property("cursorRectangle").toRectF());
    QQuickItem *flick = editor->parentItem();
    while (flick && !flick->property("contentY").isValid()) flick = flick->parentItem();
    return flick && cycle103SceneBounds(flick).contains(caret.center());
}
qreal cycle164ContentY(Cycle161Pane &p) { return p.list->property("contentY").toReal(); }
QPointF cycle164PinCentre(QQuickItem *pin) { return pin->mapToScene(QPointF(pin->width() / 2, pin->height() / 2)); }
// Opens geometry.pdf with the given notes text, Split layout, Source.
bool cycle164Open(Cycle161Pane &p, Cycle160Fixtures &f, const QString &notes, int layoutMode = 1) {
    if (!p.open(f.url("geometry.pdf"))) return false;
    if (!notes.isEmpty()) p.w.editor->setProperty("text", notes);
    p.w.layout->setProperty("layoutMode", layoutMode);
    return QTest::qWaitFor([&] { return p.w.layout->property("effectiveLayoutMode").toInt() == layoutMode && p.page(0); }, 5000);
}
}

void FomawriteTest::pdfNotesNoteMarkerRoundTrip() {
    const QString text = cycle163Canonical;
    PdfNotes::Document document = PdfNotes::parse(text);
    QCOMPARE(document.items.size(), 3);
    const PdfNotes::Item &note = document.items.at(1);
    QCOMPARE(note.kind, QStringLiteral("note"));
    QCOMPARE(note.page, 1);
    QVERIFY(note.hasAt);
    QCOMPARE(note.atX, 420);
    QCOMPARE(note.atY, 515);
    QVERIFY(!note.hasQuote);
    QVERIFY(note.hasBody);
    QCOMPARE(text.mid(note.bodyStart, note.contentEnd - note.bodyStart), QStringLiteral("Ask about the return date."));
    // Bad or missing at= is not a position; unknown keys are kept; negative values parse.
    const auto odd = PdfNotes::parse(QStringLiteral("<!-- fomawrite:pdf note page=2 at=4,x mood=calm -->\ntext\n\n<!-- fomawrite:pdf note page=2 at=-3,7 -->\n"));
    QVERIFY(!odd.items.at(0).hasAt);
    QCOMPARE(odd.items.at(0).value("mood"), QStringLiteral("calm"));
    QVERIFY(odd.items.at(1).hasAt);
    QCOMPARE(odd.items.at(1).atX, -3);
    QVERIFY(!odd.items.at(1).hasBody);

    // Insert: after the page's existing items, in one minimal change, the marker then an empty text line.
    const PdfNotes::Note pin{1, 100, 200};
    const PdfNotes::NoteInsert insert = PdfNotes::insertNote(text, pin, QStringLiteral("Report.pdf"), QStringLiteral("1"));
    QVERIFY(cycle163Minimal(text, insert.edit));
    QCOMPARE(insert.edit.size(), 1);
    QString expected = text;
    expected.replace(QStringLiteral("## [Page 3]"), cycle164Marker(1, 100, 200) + QStringLiteral("\n\n\n## [Page 3]"));
    const QString inserted = PdfNotes::apply(text, insert.edit);
    QCOMPARE(inserted, expected);
    QCOMPARE(inserted.mid(insert.caret - 1, 2), QStringLiteral("\n\n")); // the caret is on the empty line after the marker
    QVERIFY(inserted.left(insert.caret).endsWith(cycle164Marker(1, 100, 200) + QLatin1Char('\n')));
    // A page without a section gets its heading, in page order.
    const PdfNotes::NoteInsert second = PdfNotes::insertNote(text, PdfNotes::Note{2, 7, 8}, QStringLiteral("Report.pdf"), QStringLiteral("2"));
    const QString withSecond = PdfNotes::apply(text, second.edit);
    QVERIFY(withSecond.indexOf("## [Page 2](Report.pdf#page=2)") > withSecond.indexOf("## [Page 1]"));
    QVERIFY(withSecond.indexOf("## [Page 2](Report.pdf#page=2)") < withSecond.indexOf("## [Page 3]"));
    QCOMPARE(withSecond.mid(second.caret - 1, 2), QStringLiteral("\n\n"));
    // Empty notes: header, title, heading and the note.
    const PdfNotes::NoteInsert first = PdfNotes::insertNote(QString(), pin, QStringLiteral("Report.pdf"), QStringLiteral("1"));
    QCOMPARE(PdfNotes::apply(QString(), first.edit), PdfNotes::headerLine() + QStringLiteral("\n# Notes on Report.pdf\n\n## [Page 1](Report.pdf#page=1)\n\n")
             + cycle164Marker(1, 100, 200) + QStringLiteral("\n\n"));
    // Windows line breaks stay Windows line breaks.
    const QString crlf = cycle164Crlf(text);
    const PdfNotes::NoteInsert insertedCrlf = PdfNotes::insertNote(crlf, pin, QStringLiteral("Report.pdf"), QStringLiteral("1"));
    const QString crlfText = PdfNotes::apply(crlf, insertedCrlf.edit);
    QVERIFY(!QString(crlfText).replace(QStringLiteral("\r\n"), QString()).contains(QLatin1Char('\n')));
    QCOMPARE(crlfText.mid(insertedCrlf.caret - 2, 2), QStringLiteral("\r\n"));
    QCOMPARE(crlfText, cycle164Crlf(expected));

    // Move rewrites only the at= value; a note without it gains one.
    const PdfNotes::Edit move = PdfNotes::moveNote(text, note, 10, 20);
    QCOMPARE(move.size(), 1);
    QCOMPARE(move.first().removed, int(QStringLiteral("420,515").size()));
    QCOMPARE(PdfNotes::apply(text, move), QString(text).replace(QStringLiteral("at=420,515"), QStringLiteral("at=10,20")));
    QVERIFY(PdfNotes::moveNote(text, note, 420, 515).isEmpty());
    const QString bare = QStringLiteral("<!-- fomawrite:pdf note page=1 -->\nhi\n");
    QCOMPARE(PdfNotes::apply(bare, PdfNotes::moveNote(bare, PdfNotes::parse(bare).items.at(0), 3, 4)),
             QStringLiteral("<!-- fomawrite:pdf note page=1 at=3,4 -->\nhi\n"));

    // Remove: the marker line goes, the note's text stays as plain Markdown.
    const PdfNotes::Edit removed = PdfNotes::removeMark(text, note, QString());
    QVERIFY(cycle163Minimal(text, removed));
    QCOMPARE(PdfNotes::apply(text, removed), QString(text).remove(cycle164Marker(1, 420, 515) + QLatin1Char('\n')));
    const QString spaced = QStringLiteral("Before.\n\n<!-- fomawrite:pdf note page=1 at=1,2 -->\n\nKept text.\n");
    QCOMPARE(PdfNotes::apply(spaced, PdfNotes::removeMark(spaced, PdfNotes::parse(spaced).items.at(0), QString())),
             QStringLiteral("Before.\n\nKept text.\n"));
    // An empty note goes whole: inserting then removing restores the bytes.
    for (const QString &original : {QString(), QStringLiteral("My notes\n"), text, crlf, QStringLiteral("# Mine\n\nTop.\n\n## Page 2\n\nText\n")}) {
        for (const int page : {1, 2, 3, 5}) {
            const PdfNotes::NoteInsert added = PdfNotes::insertNote(original, PdfNotes::Note{page, 30, 40}, QStringLiteral("Report.pdf"), QString::number(page));
            const QString after = PdfNotes::apply(original, added.edit);
            const PdfNotes::Document parsed = PdfNotes::parse(after);
            int index = -1;
            for (int i = 0; i < parsed.items.size(); ++i)
                if (parsed.items.at(i).kind == QLatin1String("note") && parsed.items.at(i).atX == 30 && parsed.items.at(i).atY == 40) index = i;
            QVERIFY2(index >= 0, qPrintable(after));
            QVERIFY(!parsed.items.at(index).hasBody);
            QCOMPARE(PdfNotes::apply(after, PdfNotes::removeMark(after, parsed.items.at(index), QString())), original);
        }
    }
}

// Armed with the toolbar button, a click on a page in PDF-only layout switches
// to Split and writes the note in one edit; the caret is on its text line, the
// editor has the keyboard, the tool is off again and Undo takes the pin away.
void FomawriteTest::pdfNotePlacedByClickIsOneUndoStepInSource() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QVERIFY(p.reader->setProperty("inPageNoteEditing", false));
    QTRY_VERIFY(p.page(0));
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    auto *button = p.item("pdfNote"); QVERIFY(button);
    QVERIFY(button->property("enabled").toBool());
    QVERIFY(cycle102Click(p.w.quick, button));
    QVERIFY(p.reader->property("noteTool").toBool());
    p.call("focusPages");
    p.key(Qt::Key_Escape); // Escape disarms
    QVERIFY(!p.reader->property("noteTool").toBool());
    QVERIFY(cycle102Click(p.w.quick, button));
    QVERIFY(p.reader->property("noteTool").toBool());
    cycle164Click(p, 0, QPointF(300, 400));
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1); // PDF only became Split
    QTRY_COMPARE(cycle164Notes(p).size(), 1);
    const QVariantMap note = cycle164Notes(p).first().toMap();
    QVERIFY(qAbs(note.value("x").toInt() - 300) <= 1 && qAbs(note.value("y").toInt() - 400) <= 1);
    const QString marker = cycle164Marker(1, note.value("x").toInt(), note.value("y").toInt());
    const QString placed = PdfNotes::headerLine() + QStringLiteral("\n# Notes on geometry.pdf\n\n## [Page 1](geometry.pdf#page=1)\n\n") + marker + QStringLiteral("\n\n");
    QCOMPARE(p.w.editor->property("text").toString(), placed);
    const int caret = int(placed.indexOf(marker)) + int(marker.size()) + 1;
    QTRY_COMPARE(p.w.editor->property("cursorPosition").toInt(), caret);
    QTRY_VERIFY(p.w.editor->hasActiveFocus());
    QVERIFY(!p.reader->property("noteTool").toBool()); // one placement per arming
    QCOMPARE(cycle164Pins(p).size(), 1);
    QVERIFY(p.w.backend.modified());
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "undo"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), QString()); // the whole placement in one step
    QTRY_COMPARE(cycle164Pins(p).size(), 0);
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "redo"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), placed);
    QTRY_COMPARE(cycle164Pins(p).size(), 1);
    // The writer can type the note's text at once, and the pin's popover has it.
    QTRY_VERIFY(p.w.editor->hasActiveFocus());
    p.w.editor->setProperty("cursorPosition", caret);
    for (const char letter : {'A', 's', 'k'}) QTest::keyClick(p.w.quick, letter);
    QTRY_VERIFY(p.w.editor->property("text").toString().contains(marker + "\nAsk\n"));
    QTRY_COMPARE(cycle164Notes(p).first().toMap().value("text").toString(), QStringLiteral("Ask"));
}

// The same in Live: the page gets the placement as one change set (one undo
// step of its own), and its caret is on the note's text line.
void FomawriteTest::pdfNotePlacedByClickIsOneUndoStepInLive() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QVERIFY(p.reader->setProperty("inPageNoteEditing", false));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(p.page(0));
    const QString before = QStringLiteral("My notes\n");
    p.w.editor->setProperty("text", before);
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    QVERIFY(p.w.layout->property("liveEditEnabled").toBool());
    auto *bridge = p.w.backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 30000);
    auto *view = p.w.window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), before, 10000);
    p.call("armNote");
    QVERIFY(p.reader->property("noteTool").toBool());
    cycle164Click(p, 0, QPointF(250, 300));
    QTRY_COMPARE(cycle164Notes(p).size(), 1);
    const QVariantMap note = cycle164Notes(p).first().toMap();
    const QString marker = cycle164Marker(1, note.value("x").toInt(), note.value("y").toInt());
    const QString placed = PdfNotes::headerLine() + QStringLiteral("\n\nMy notes\n\n## [Page 1](geometry.pdf#page=1)\n\n") + marker + QStringLiteral("\n\n");
    QCOMPARE(p.w.editor->property("text").toString(), placed);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), placed, 10000);
    const int caret = int(placed.indexOf(marker)) + int(marker.size()) + 1;
    QTRY_COMPARE_WITH_TIMEOUT(cycle163LiveSelection(bridge), qMakePair(caret, caret), 10000);
    QTRY_COMPARE(cycle164Pins(p).size(), 1);
    bridge->requestUndo(); // one step in the page's own history
    QTRY_COMPARE_WITH_TIMEOUT(p.w.editor->property("text").toString(), before, 10000);
    QCOMPARE(cycle144PageText(bridge), before);
    QTRY_COMPARE(cycle164Pins(p).size(), 0);
    p.w.layout->setProperty("liveEditEnabled", false);
}

// A pin is drawn where it was clicked on pages rotated 0, 90, 180 and 270 and
// on the CropBox page, at a fixed size at any zoom, with its position scaling.
void FomawriteTest::pdfNotePinLandsWhereClickedOnEveryRotation() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(cycle164Open(p, f, QString(), 2));
    QVERIFY(p.reader->setProperty("inPageNoteEditing", false));
    for (int page = 0; page < 5; ++page) {
        QVERIFY(p.reader->setProperty("noteTool", true));
        const QPointF points(180, 260);
        cycle164Click(p, page, points);
        QTRY_COMPARE(cycle164Notes(p).size(), page + 1);
        QVERIFY(!p.reader->property("noteTool").toBool());
        const QVariantMap note = cycle164Notes(p).value(page).toMap();
        QCOMPARE(note.value("page").toInt(), page);
        QVERIFY2(qAbs(note.value("x").toInt() - points.x()) <= 1 && qAbs(note.value("y").toInt() - points.y()) <= 1,
                 qPrintable(QString("page %1 at %2,%3").arg(page).arg(note.value("x").toInt()).arg(note.value("y").toInt())));
        QVERIFY(p.w.editor->property("text").toString().contains(cycle164Marker(page + 1, note.value("x").toInt(), note.value("y").toInt())));
        // The pin's centre is on the clicked point of that page.
        p.reveal(page, points);
        QQuickItem *pin = nullptr;
        for (auto *candidate : cycle164Pins(p))
            if (qAbs(cycle164PinCentre(candidate).x() - p.scene(page, points).x()) < 2 && qAbs(cycle164PinCentre(candidate).y() - p.scene(page, points).y()) < 2) pin = candidate;
        QVERIFY2(pin, qPrintable(QString("page %1").arg(page)));
        QCOMPARE(pin->width(), 22.0);
        QCOMPARE(pin->height(), 22.0);
    }
    // Zoom: the pin keeps its size, its position scales with the page.
    p.call("goToPage", 0);
    p.reader->setProperty("fixedZoom", 2.0);
    p.reader->setProperty("zoomMode", QStringLiteral("fixed"));
    QVERIFY(p.settled());
    QTRY_COMPARE(p.zoom(), 2.0);
    p.reveal(0, QPointF(180, 260));
    QTRY_VERIFY(!cycle164Pins(p).isEmpty());
    QQuickItem *pin = nullptr;
    for (auto *candidate : cycle164Pins(p))
        if (qAbs(cycle164PinCentre(candidate).x() - p.scene(0, QPointF(180, 260)).x()) < 2 && qAbs(cycle164PinCentre(candidate).y() - p.scene(0, QPointF(180, 260)).y()) < 2) pin = candidate;
    QVERIFY(pin);
    QCOMPARE(pin->width(), 22.0);
    QCOMPARE(p.zoom(), 2.0);
}

// Clicking a pin selects it and shows its text in a popover (read-only);
// clicking elsewhere or Escape closes it. Dragging moves only the at= value
// in one edit; Delete and the context menu remove the marker, never the text.
void FomawriteTest::pdfNotePopoverMoveAndRemove() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    const QString notes = QStringLiteral("My notes\n\n## [Page 1](geometry.pdf#page=1)\n\n") + cycle164Marker(1, 300, 400)
        + QStringLiteral("\nAsk about the return date.\n\nMore.\n");
    QVERIFY(cycle164Open(p, f, notes));
    QVERIFY(p.reader->setProperty("inPageNoteEditing", false));
    QTRY_COMPARE(cycle164Pins(p).size(), 1);
    p.reveal(0, QPointF(300, 400));
    auto popover = [&]() -> QQuickItem * {
        const auto found = cycle103VisualItems(p.list, QStringLiteral("pdfNotePopover"));
        return found.isEmpty() ? nullptr : found.first();
    };
    QVERIFY(!popover() || !popover()->isVisible());
    cycle164Click(p, 0, QPointF(300, 400));
    QTRY_VERIFY(popover() && popover()->isVisible());
    QCOMPARE(p.reader->property("selectedPin").toMap().value("marker").toString(), cycle164Marker(1, 300, 400));
    auto *label = cycle103VisualItems(p.list, QStringLiteral("pdfNoteText")).value(0);
    QVERIFY(label);
    QCOMPARE(label->property("text").toString(), QStringLiteral("Ask about the return date.\n\nMore."));
    QCOMPARE(p.w.editor->property("text").toString(), notes); // clicking changed nothing in the notes
    cycle164Click(p, 0, QPointF(100, 100)); // elsewhere
    QTRY_VERIFY(!popover()->isVisible());
    cycle164Click(p, 0, QPointF(300, 400));
    QTRY_VERIFY(popover()->isVisible());
    p.call("focusPages");
    p.key(Qt::Key_Escape);
    QTRY_VERIFY(!popover()->isVisible());

    // Drag: press on the pin, move, release.
    const QString beforeDrag = p.w.editor->property("text").toString();
    const QPoint from = p.scene(0, QPointF(300, 400)).toPoint();
    const QPoint to = p.scene(0, QPointF(200, 150)).toPoint();
    QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(p.w.quick, (from + to) / 2);
    QTest::mouseMove(p.w.quick, to);
    QVERIFY(!p.reader->property("dragNote").isNull() && p.reader->property("dragNote").isValid());
    QCOMPARE(p.w.editor->property("text").toString(), beforeDrag); // nothing is written until release
    QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, to);
    const QVariantMap moved = cycle164Notes(p).first().toMap();
    QVERIFY(qAbs(moved.value("x").toInt() - 200) <= 2 && qAbs(moved.value("y").toInt() - 150) <= 2);
    QCOMPARE(p.w.editor->property("text").toString(),
             QString(beforeDrag).replace(cycle164Marker(1, 300, 400), cycle164Marker(1, moved.value("x").toInt(), moved.value("y").toInt())));
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "undo")); // one step
    QTRY_COMPARE(p.w.editor->property("text").toString(), beforeDrag);
    QTRY_COMPARE(cycle164Notes(p).first().toMap().value("x").toInt(), 300);
    // A pin never leaves its page (the drag clamps, and so does the edit).
    const QSizeF size = p.w.backend.pdf()->pageSize(0);
    QVERIFY(p.w.backend.pdfMarks()->moveNote(cycle164Notes(p).first().toMap().value("index").toInt(), size.width() + 400, -300));
    QCOMPARE(cycle164Notes(p).first().toMap().value("x").toInt(), int(size.width()));
    QCOMPARE(cycle164Notes(p).first().toMap().value("y").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "undo"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), beforeDrag);

    // Delete removes the marker line only.
    const QString kept = QString(notes).remove(cycle164Marker(1, 300, 400) + QLatin1Char('\n'));
    cycle164Click(p, 0, QPointF(300, 400));
    QTRY_VERIFY(popover()->isVisible());
    p.key(Qt::Key_Delete);
    QTRY_COMPARE(p.w.editor->property("text").toString(), kept);
    QTRY_COMPARE(cycle164Pins(p).size(), 0);
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "undo"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), notes);
    QTRY_COMPARE(cycle164Pins(p).size(), 1);
    // The context menu says "Remove Note".
    cycle162ContextClick(p.w.quick, p.scene(0, QPointF(300, 400)).toPoint());
    auto *menu = p.object("pdfPageMenu"); QVERIFY(menu);
    QTRY_VERIFY(menu->property("opened").toBool());
    auto *remove = p.object("pdfContextRemoveMark");
    QVERIFY(remove->property("visible").toBool());
    QCOMPARE(remove->property("text").toString(), QStringLiteral("Remove Note"));
    QVERIFY(!p.object("pdfContextColour_yellow")->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(remove, "click"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), kept);
    // An empty note is removed whole.
    p.w.editor->setProperty("text", QString());
    QVERIFY(p.w.backend.pdfMarks()->placeNote(0, 50, 60) >= 0);
    QCOMPARE(cycle164Notes(p).size(), 1);
    QVERIFY(p.w.backend.pdfMarks()->removeMark(cycle164Notes(p).first().toMap().value("index").toInt()));
    QCOMPARE(p.w.editor->property("text").toString(), QString());
}

// A plain click on a mark or a pin puts the notes' caret at the start of its
// item (the quote, or the note's text), scrolling the editor to it; PDF only it
// just selects. Same in Live.
void FomawriteTest::pdfClickOnMarkOrPinShowsItsItemInTheNotes() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QString filler;
    for (int i = 0; i < 120; ++i) filler += QStringLiteral("Filler line %1 of the notes.\n\n").arg(i);
    QVERIFY(cycle164Open(p, f, filler));
    QVERIFY(p.reader->setProperty("inPageNoteEditing", false));
    PdfMarks *marks = p.w.backend.pdfMarks();
    PdfSession *pdf = p.w.backend.pdf();
    QCOMPARE(marks->mark("highlight", "yellow", 0, int(pdf->pageText(0).indexOf("ALPHA")), 5), QStringLiteral("added"));
    QVERIFY(marks->placeNote(0, 300, 500) >= 0);
    const QString text = p.w.editor->property("text").toString();
    const int quote = int(text.indexOf("> ALPHA"));
    const int noteText = int(text.indexOf(cycle164Marker(1, 300, 500))) + int(cycle164Marker(1, 300, 500).size()) + 1;
    QVERIFY(quote > 0 && noteText > quote);
    // Click the highlight.
    const QRectF alpha = cycle163WordRect(pdf, 0, "ALPHA");
    p.w.editor->setProperty("cursorPosition", 0);
    QTest::qWait(700); // the window's own caret moves are quiet for a moment
    cycle164Click(p, 0, alpha.center());
    QTRY_COMPARE(p.w.editor->property("cursorPosition").toInt(), quote);
    QTRY_VERIFY(cycle164CaretVisible(p.w.editor));
    QVERIFY(p.list->hasActiveFocus()); // the page keeps the keyboard
    QCOMPARE(p.w.editor->property("text").toString(), text);
    // Click the pin: caret at the note's text; the popover shows.
    p.w.editor->setProperty("cursorPosition", 0);
    QTest::qWait(700); // the window's own caret moves are quiet for a moment
    cycle164Click(p, 0, QPointF(300, 500));
    QTRY_COMPARE(p.w.editor->property("cursorPosition").toInt(), noteText);
    QVERIFY(!p.reader->property("selectedPin").isNull());
    // A click on blank page, a link-free click: no caret move.
    p.w.editor->setProperty("cursorPosition", 3);
    cycle164Click(p, 0, QPointF(500, 700));
    QTest::qWait(100);
    QCOMPARE(p.w.editor->property("cursorPosition").toInt(), 3);
    // PDF only: selects, does not change the layout or the caret.
    p.w.layout->setProperty("layoutMode", 2);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    QTRY_VERIFY(p.page(0));
    // The pin is drawn where the notes put it before the click is sent.
    QTRY_VERIFY(!cycle164Pins(p).isEmpty());
    cycle164Click(p, 0, QPointF(300, 500));
    QTRY_VERIFY(!p.reader->property("selectedPin").isNull());
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    QCOMPARE(p.w.editor->property("cursorPosition").toInt(), 3);
    QVERIFY(!p.reader->property("selectedPin").isNull());

    // Live.
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = p.w.backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 30000);
    auto *view = p.w.window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), text, 10000);
    bridge->placeCursor(0);
    QTRY_COMPARE_WITH_TIMEOUT(cycle163LiveSelection(bridge), qMakePair(0, 0), 10000);
    QTest::qWait(700);
    p.reveal(0, alpha.center());
    cycle164Click(p, 0, alpha.center());
    QTRY_COMPARE_WITH_TIMEOUT(cycle163LiveSelection(bridge), qMakePair(quote, quote), 10000);
    p.w.layout->setProperty("liveEditEnabled", false);
}

// The caret moving into a different item shows it on the page: the pane
// scrolls just enough and flashes it. Not when it is already in view, not
// while typing inside the same item, and not as a result of a click on the
// page or of inserting a mark.
void FomawriteTest::pdfCaretIntoAnItemShowsItOnThePage() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(cycle164Open(p, f, QString()));
    PdfMarks *marks = p.w.backend.pdfMarks();
    PdfSession *pdf = p.w.backend.pdf();
    QCOMPARE(marks->mark("highlight", "yellow", 0, int(pdf->pageText(0).indexOf("ALPHA")), 5), QStringLiteral("added"));
    QCOMPARE(marks->mark("underline", "red", 3, int(pdf->pageText(3).indexOf("OMEGA")), 5), QStringLiteral("added"));
    QVERIFY(marks->placeNote(4, 100, 150) >= 0);
    const QString text = p.w.editor->property("text").toString();
    const int first = int(text.indexOf("> ALPHA")), fourth = int(text.indexOf("> OMEGA"));
    const QString pinMarker = cycle164Marker(5, 100, 150);
    const int fifth = int(text.indexOf(pinMarker)) + int(pinMarker.size()) + 1;
    p.call("goToPage", 0);
    QVERIFY(p.settled());
    QTest::qWait(700);
    p.w.editor->setProperty("cursorPosition", 0);
    QTest::qWait(100);
    // Into the page-4 underline from the first page: the pane scrolls to it and flashes it.
    const qreal before = cycle164ContentY(p);
    p.w.editor->setProperty("cursorPosition", fourth);
    QTRY_VERIFY(cycle164ContentY(p) > before + 100);
    QTRY_COMPARE(p.reader->property("flashIndex").toInt(), 1);
    QTRY_VERIFY(!cycle103VisualItems(p.list, QStringLiteral("pdfMarkFlash")).isEmpty());
    QVERIFY(p.reader->property("currentPage").toInt() >= 2);
    const QRectF mark = cycle103VisualItems(p.list, QStringLiteral("pdfMark_underline")).value(0) ? cycle103SceneBounds(cycle103VisualItems(p.list, QStringLiteral("pdfMark_underline")).value(0)) : QRectF();
    QVERIFY(p.viewport().contains(mark));
    // Typing inside the same item: nothing scrolls, nothing new flashes.
    p.call("goToPage", 3);
    QVERIFY(p.settled());
    const qreal y = cycle164ContentY(p);
    p.reader->setProperty("flashIndex", -1);
    p.w.editor->setProperty("cursorPosition", fourth + 4);
    QTest::qWait(150);
    QCOMPARE(cycle164ContentY(p), y);
    QCOMPARE(p.reader->property("flashIndex").toInt(), -1);
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, fourth + 4), Q_ARG(QString, "x")));
    QTest::qWait(150);
    QCOMPARE(cycle164ContentY(p), y);
    QCOMPARE(p.reader->property("flashIndex").toInt(), -1);
    // Already fully visible: no scroll (the pane is on page 4 and the item is on it), but only a different item counts.
    p.w.editor->setProperty("cursorPosition", 0);
    QTest::qWait(100);
    const qreal visibleY = cycle164ContentY(p);
    p.w.editor->setProperty("cursorPosition", fourth + 1);
    QTest::qWait(150);
    QCOMPARE(cycle164ContentY(p), visibleY);
    // The pin's note text is part of its item.
    p.w.editor->setProperty("cursorPosition", first);
    QTest::qWait(100);
    p.w.editor->setProperty("cursorPosition", fifth);
    QTRY_VERIFY(cycle164ContentY(p) != visibleY);
    QTRY_VERIFY(p.reader->property("currentPage").toInt() == 4 || p.reader->property("currentPage").toInt() == 3);
    QTRY_VERIFY(!cycle103VisualItems(p.list, QStringLiteral("pdfNoteFlash")).isEmpty());
    // A click on a mark moves the notes' caret; that must not scroll or flash the page in answer.
    p.call("goToPage", 0);
    QVERIFY(p.settled());
    QTest::qWait(700);
    p.reader->setProperty("flashIndex", -1);
    const QRectF alpha = cycle163WordRect(pdf, 0, "ALPHA");
    p.w.editor->setProperty("cursorPosition", fifth);
    QTest::qWait(700);
    p.call("goToPage", 0);
    QVERIFY(p.settled());
    p.reveal(0, alpha.center());
    QTest::qWait(700);
    const qreal clickY = cycle164ContentY(p);
    p.reader->setProperty("flashIndex", -1);
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, alpha.center()).toPoint());
    QTRY_COMPARE(p.w.editor->property("cursorPosition").toInt(), first);
    QTest::qWait(200);
    QCOMPARE(p.reader->property("flashIndex").toInt(), -1);
    QVERIFY(qAbs(cycle164ContentY(p) - clickY) < 2);
    // Inserting a mark does not scroll the page to the new item.
    p.call("goToPage", 3);
    QVERIFY(p.settled());
    QTest::qWait(700);
    const qreal atFour = cycle164ContentY(p);
    p.reader->setProperty("flashIndex", -1);
    QCOMPARE(marks->mark("strike", "red", 0, int(pdf->pageText(0).indexOf("OMEGA")), 5), QStringLiteral("added"));
    QTest::qWait(250);
    QCOMPARE(cycle164ContentY(p), atFour);
    QCOMPARE(p.reader->property("flashIndex").toInt(), -1);
}

// Selecting by dragging from a highlight still selects; the notes' caret stays.
void FomawriteTest::pdfDragSelectingFromAHighlightStillSelects() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(cycle164Open(p, f, QStringLiteral("My notes\n")));
    PdfSession *pdf = p.w.backend.pdf();
    QCOMPARE(p.w.backend.pdfMarks()->mark("highlight", "yellow", 0, int(pdf->pageText(0).indexOf("ALPHA")), 5), QStringLiteral("added"));
    p.w.editor->setProperty("cursorPosition", 2);
    QTest::qWait(700);
    const QRectF alpha = cycle163WordRect(pdf, 0, "ALPHA");
    p.reveal(0, alpha.center());
    const QPoint from = p.scene(0, QPointF(alpha.left() + 0.5, alpha.center().y())).toPoint();
    const QPoint to = p.scene(0, QPointF(alpha.right() + 1, alpha.center().y())).toPoint();
    QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(p.w.quick, (from + to) / 2);
    QTest::mouseMove(p.w.quick, to);
    QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, to);
    QVERIFY2(p.reader->property("selectionText").toString().contains("PHA"), qPrintable("selection=" + p.reader->property("selectionText").toString() + " from " + QString::number(from.x()) + "," + QString::number(from.y()) + " to " + QString::number(to.x())));
    QTest::qWait(150);
    QCOMPARE(p.w.editor->property("cursorPosition").toInt(), 2); // a drag is not a click on the mark
    // A double-click on it still selects the word.
    p.call("clearSelection");
    QTest::mouseDClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, alpha.center()).toPoint());
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("ALPHA"));
}

// The Note tool: a toolbar button right after the highlighter, "Note" in the
// PDF menu with Control-Command-N (nothing else uses it), Escape disarms.
void FomawriteTest::pdfNoteToolButtonMenuItemAndShortcut() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    auto *button = p.item("pdfNote"); QVERIFY(button);
    auto *highlighter = p.item("pdfHighlighterGroup"); QVERIFY(highlighter);
    const QRectF noteBox = cycle103SceneBounds(button), penBox = cycle103SceneBounds(highlighter);
    QVERIFY(noteBox.left() >= penBox.right() - 0.5 && noteBox.left() - penBox.right() < 20);
    QVERIFY(qAbs(noteBox.center().y() - penBox.center().y()) < 2);
    QVERIFY(button->property("enabled").toBool());
    const Cycle161Menus menus{p.w.window.data()};
    auto *item = menus.item("note"); QVERIFY(item);
    QCOMPARE(item->property("text").toString(), QStringLiteral("Note"));
    if (menus.native()) {
        QCOMPARE(QKeySequence(item->property("shortcut").toString()), QKeySequence(QStringLiteral("Meta+Ctrl+N")));
        for (auto *object : p.w.window->findChildren<QObject *>()) {
            if (object == item) continue;
            QStringList used;
            for (const char *property : {"shortcut", "sequence"}) {
                const QVariant value = object->property(property);
                if (value.isValid()) used << QKeySequence(value.toString()).toString();
            }
            for (const QVariant &value : object->property("sequences").toList()) used << QKeySequence(value.toString()).toString();
            QVERIFY2(!used.contains(QKeySequence(QStringLiteral("Meta+Ctrl+N")).toString()), qPrintable("also used by " + object->objectName()));
        }
    }
    QTRY_VERIFY(item->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(item, "triggered"));
    QVERIFY(p.reader->property("noteTool").toBool());
    QVERIFY(button->property("checked").toBool());
    p.call("focusPages");
    p.key(Qt::Key_Escape);
    QVERIFY(!p.reader->property("noteTool").toBool());
    QVERIFY(!button->property("checked").toBool());
    QVERIFY(cycle102Click(p.w.quick, button));
    QVERIFY(p.reader->property("noteTool").toBool());
    QVERIFY(cycle102Click(p.w.quick, button)); // the button toggles
    QVERIFY(!p.reader->property("noteTool").toBool());
}
