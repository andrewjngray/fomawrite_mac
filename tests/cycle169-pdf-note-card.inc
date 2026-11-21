#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
void FomawriteTest::splitSourceFindCentersTypedText() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(cycle164Open(p, f, QStringLiteral("Ågyp sample"), 1));
    p.w.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "openSearch", Q_ARG(QVariant, true), Q_ARG(QVariant, false)));
    for (const char *name : {"searchField", "replaceField"}) {
        auto *field = p.w.window->findChild<QQuickItem *>(name);
        QVERIFY(field && field->isVisible());
        field->forceActiveFocus();
        cycle111KeyClick(p.w.quick, Qt::Key_unknown, QStringLiteral("Ågyp"));
        QCOMPARE(field->property("text").toString(), QStringLiteral("Ågyp"));
        const QRectF cursor = field->property("cursorRectangle").toRectF();
        QVERIFY(cursor.height() > 0);
        auto *background = qvariant_cast<QQuickItem *>(field->property("background"));
        QVERIFY(background);
        const QRectF bounds = background->mapRectToItem(field, background->boundingRect());
        QVERIFY(bounds.top() <= cursor.top() && bounds.bottom() >= cursor.bottom());
        QVERIFY2(qAbs(cursor.center().y() - bounds.center().y()) <= 1.5,
                 qPrintable(QStringLiteral("%1 cursor %2 field centre %3").arg(name).arg(cursor.center().y()).arg(bounds.center().y())));
    }
    QVERIFY(cycle102Capture(p.w.quick, QStringLiteral("cycle169-split-find")));
}

void FomawriteTest::pdfNotePinCopiesAndShowsAHoverCard() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    const QString note = QStringLiteral("Note text.\n\nSecond paragraph.");
    QVERIFY(cycle164Open(p, f, QStringLiteral("## Page 1\n\n<!-- fomawrite:pdf note page=1 at=300,400 author=Jamie%20Lee created=2026-11-22T01:34:00Z -->\n") + note + '\n', 2));
    QTRY_COMPARE(cycle164Notes(p).size(), 1);
    p.reveal(0, QPointF(300, 400));
    const QPoint point = p.scene(0, QPointF(300, 400)).toPoint();
    auto *menu = p.object("pdfPageMenu");
    auto *copy = p.object("pdfContextCopy");
    auto *quote = p.object("pdfContextCopyQuote");
    cycle162ContextClick(p.w.quick, point);
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(copy->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(copy, "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(QGuiApplication::clipboard()->text(), note);
    cycle162ContextClick(p.w.quick, point);
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(quote->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(quote, "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(QGuiApplication::clipboard()->text(), PdfSession::markdownQuote(note, "geometry.pdf", 0, "1"));
    auto *hover = p.item("pdfNoteHover");
    QVERIFY(hover);
    QTest::mouseMove(p.w.quick, point + QPoint(32, 16));
    QTest::mouseMove(p.w.quick, point);
    QTRY_VERIFY(hover->isVisible());
    QCOMPARE(p.item("pdfNoteHoverAuthor")->property("text").toString(), QStringLiteral("Jamie Lee"));
    QVERIFY(p.item("pdfNoteHoverText")->property("text").toString().contains(note));
    QTest::mouseMove(p.w.quick, point + QPoint(32, 16));
    QTRY_VERIFY(!hover->isVisible());
}

void FomawriteTest::pdfNoteCardDragsAndKeepsCreationDetails() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    p.reader->setProperty("noteAuthor", QStringLiteral("Jamie Lée -->"));
    p.call("armNote");
    cycle164Click(p, 0, QPointF(300, 400));
    auto *draft = p.object("pdfNoteEditor");
    QTRY_VERIFY(draft->property("opened").toBool());
    auto *input = draft->findChild<QQuickItem *>("pdfNoteInput");
    input->setProperty("text", QStringLiteral("Created note."));
    p.call("finishNote");
    QTRY_VERIFY(!draft->property("visible").toBool());
    const QVariantMap saved = p.w.backend.pdfMarks()->noteDraft(0);
    QVERIFY(QDateTime::fromString(saved.value("created").toString(), Qt::ISODate).isValid());
    QCOMPARE(saved.value("author").toString(), QStringLiteral("Jamie Lée -->"));
    const QString notes = p.w.editor->property("text").toString();
    cycle164Click(p, 0, QPointF(300, 400));
    QTRY_VERIFY(draft->property("opened").toBool());
    auto *header = draft->findChild<QQuickItem *>("pdfNoteDragHandle");
    QVERIFY(header);
    QVERIFY(!draft->findChild<QQuickItem *>("pdfNoteCreated")->property("text").toString().isEmpty());
    const qreal oldY = draft->property("y").toReal();
    const QPoint from = header->mapToScene(QPointF(header->width() / 2, header->height() / 2)).toPoint();
    const QPoint offset(0, -65);
    QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(p.w.quick, from + offset / 2);
    QTest::mouseMove(p.w.quick, from + offset);
    QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, from + offset);
    QTRY_VERIFY(draft->property("y").toReal() < oldY - 20);
    QCOMPARE(p.w.editor->property("text").toString(), notes);
    QVERIFY(cycle102Capture(p.w.quick, QStringLiteral("cycle169-pdf-note-card")));
    auto *close = draft->findChild<QQuickItem *>("pdfNoteClose");
    QVERIFY(cycle102Click(p.w.quick, close));
    QTRY_VERIFY(!draft->property("visible").toBool());
    QCOMPARE(p.w.editor->property("text").toString(), notes);
    cycle164Click(p, 0, QPointF(300, 400));
    QTRY_VERIFY(draft->property("opened").toBool());
    auto *remove = draft->findChild<QQuickItem *>("pdfNoteDelete");
    QVERIFY(cycle102Click(p.w.quick, remove));
    QTRY_VERIFY(!draft->property("visible").toBool());
    QVERIFY(!p.w.editor->property("text").toString().contains("Created note."));
    QTRY_COMPARE(cycle164Notes(p).size(), 0);
    QTRY_VERIFY(p.list->hasActiveFocus());
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "undoEditing"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), notes);
}
#endif
