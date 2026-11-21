#ifdef Q_OS_MACOS
void FomawriteTest::pdfNoteDraftCommitsAndCancelsInPdfOnlyView() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);

    QVERIFY(cycle102Click(p.w.quick, p.item("pdfNote")));
    QVERIFY(p.reader->property("noteTool").toBool());
    cycle164Click(p, 0, QPointF(300, 400));
    auto *draft = p.object("pdfNoteEditor");
    QVERIFY(draft);
    QTRY_VERIFY(draft->property("opened").toBool());
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    QCOMPARE(cycle164Notes(p).size(), 0);
    QVERIFY(!p.w.backend.modified());
    auto *input = draft->findChild<QQuickItem *>("pdfNoteInput");
    QVERIFY(input);
    QTRY_VERIFY(input->hasActiveFocus());
    cycle111KeyClick(p.w.quick, 'x');
    QTRY_COMPARE(input->property("text").toString(), QStringLiteral("x"));
    cycle111KeyClick(p.w.quick, Qt::Key_Escape);
    QTRY_VERIFY(!p.reader->property("noteDraftOpen").toBool());
    QVERIFY(p.w.editor->property("text").toString().isEmpty());
    QVERIFY(!p.w.backend.modified());

    QVERIFY(cycle102Click(p.w.quick, p.item("pdfNote")));
    QVERIFY(p.reader->property("noteTool").toBool());
    cycle164Click(p, 0, QPointF(300, 400));
    QTRY_VERIFY(draft->property("opened").toBool());
    input->setProperty("text", QStringLiteral("Ask about this.\n\nFollow up tomorrow."));
    QVERIFY(cycle102Capture(p.w.quick, QStringLiteral("cycle167-pdf-note-popover")));
    auto *done = draft->findChild<QQuickItem *>("pdfNoteDone");
    QVERIFY(done);
    QVERIFY(cycle102Click(p.w.quick, done));
    QTRY_VERIFY(!p.reader->property("noteDraftOpen").toBool());
    QTRY_COMPARE(cycle164Notes(p).size(), 1);
    QTRY_VERIFY(p.list->hasActiveFocus());
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    const QString beforeUndo = p.w.editor->property("text").toString();
    QVERIFY(beforeUndo.contains(QStringLiteral("\nAsk about this.\n\nFollow up tomorrow.\n")));

    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "undoEditing"));
    QTRY_VERIFY(p.w.editor->property("text").toString().isEmpty());
    QTRY_COMPARE(cycle164Notes(p).size(), 0);
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "redoEditing"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), beforeUndo);
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "saveDocument"));
    QFile saved(f.path("geometry.pdf.md"));
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), beforeUndo.toUtf8());
}

void FomawriteTest::pdfNoteDraftEditsTheFullBodyWithoutTouchingOtherNotes() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    const QString body = QString(900, QLatin1Char('a')) + QStringLiteral("\n\nFull ending.");
    const QString first = cycle164Marker(1, 300, 400);
    const QString other = cycle164Marker(1, 110, 160) + QStringLiteral("\nUnrelated text.\n");
    const QString before = QStringLiteral("My notes\n\n## Page 1\n\n") + first + QLatin1Char('\n') + body + QStringLiteral("\n\n") + other;
    QVERIFY(cycle164Open(p, f, before, 2));

    cycle164Click(p, 0, QPointF(300, 400));
    auto *draft = p.object("pdfNoteEditor");
    QVERIFY(draft);
    QTRY_VERIFY(draft->property("opened").toBool());
    auto *input = draft->findChild<QQuickItem *>("pdfNoteInput");
    QVERIFY(input);
    QCOMPARE(input->property("text").toString(), body);
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    input->setProperty("text", QStringLiteral("# A heading"));
    p.call("finishNote");
    QVERIFY(draft->property("opened").toBool());
    QCOMPARE(p.w.editor->property("text").toString(), before);
    input->setProperty("text", QStringLiteral("Revised note."));
    p.call("finishNote");
    QTRY_VERIFY(!p.reader->property("noteDraftOpen").toBool());
    QTRY_COMPARE(p.w.editor->property("text").toString(), QString(before).replace(body, QStringLiteral("Revised note.")));
    QTRY_VERIFY(p.list->hasActiveFocus());
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "undoEditing"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), before);
}
#endif
