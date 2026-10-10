// Cycle 114: bounded link syntax and captured Source-edit transactions.
class Cycle114LinkFixture {
public:
    QTemporaryDir directory;
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> window;
    QQuickWindow *quick = nullptr;
    QQuickItem *editor = nullptr;
    QObject *popup = nullptr;
    QString error;
    std::unique_ptr<Cycle106MouseInputIsolation> isolation;
    bool start(const QString &source, int width = 960) {
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
        window.reset(component.create()); error = component.errorString();
        if (!window) return false;
        quick = qobject_cast<QQuickWindow *>(window.data());
        editor = window->findChild<QQuickItem *>("sourceEditor"); popup = window->findChild<QObject *>("linkEditor");
        if (!quick || !editor || !popup) return false;
        cycle106KeepNativeFocus(quick); isolation.reset(new Cycle106MouseInputIsolation(quick));
        quick->resize(width, 520); quick->show();
        if (!QTest::qWaitForWindowExposed(quick)) return false;
        auto *layout = window->findChild<QObject *>("workspaceLayout");
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        if (!cycle103Write(directory.filePath("Draft.md"), source.toUtf8()) || !backend.open(QUrl::fromLocalFile(directory.filePath("Draft.md")))) return false;
        QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "source"));
        QTest::qWait(250);
        editor->forceActiveFocus();
        return true;
    }
    bool open(int start, int end) {
        editor->forceActiveFocus();
        if (!QMetaObject::invokeMethod(editor, "select", Q_ARG(int, start), Q_ARG(int, end))) return false;
        QTest::qWait(20);
        if (!QMetaObject::invokeMethod(window.data(), "tryInsertLink")) return false;
        QTest::qWait(150);
        return popup->property("visible").toBool();
    }
    QQuickItem *field(const char *name) const { return window->findChild<QQuickItem *>(name); }
    bool click(const char *name) { return cycle102Click(quick, field(name)); }
    ~Cycle114LinkFixture() { backend.discardRecovery(); }
};

void FomawriteTest::linkSyntaxPreservesSupportedFormsAndRejectsAmbiguity() {
    QQmlEngine engine;
    QFile script(QStringLiteral(":/LinkSyntax.js")); QVERIFY(script.open(QIODevice::ReadOnly));
    QString code = QString::fromUtf8(script.readAll()); code.remove(".pragma library");
    QJSValue evaluated = engine.evaluate(code); QVERIFY2(!evaluated.isError(), qPrintable(evaluated.toString()));
    const auto analyze = [&](const QString &source, int position) {
        return engine.globalObject().property("analyze").call({source, position, position}).toVariant().toMap();
    };
    const QString label = QString::fromUtf8("A [café] \\ note");
    const QString destination = QString::fromUtf8("notes/東京 (draft).md#part");
    const QString title = QStringLiteral("A \"quoted\" title");
    const QString markdown = engine.globalObject().property("serialize").call({label, destination, title}).toString();
    const auto parsed = analyze(markdown, 3).value("existing").toMap();
    QCOMPARE(parsed.value("label").toString(), label);
    QCOMPARE(parsed.value("url").toString(), destination);
    QCOMPARE(parsed.value("title").toString(), title);
    for (const QString &source : {
        QStringLiteral("[outer [inner](inside.md)](outside.md)"),
        QStringLiteral("```\n``` not a close\n[text](inside.md)\n```"),
        QStringLiteral("    [text](inside.md)"), QStringLiteral("<!-- [text](inside.md) -->"),
        QStringLiteral("[text]: destination.md"), QStringLiteral("`[text](inside.md)`"),
        QStringLiteral("![text](inside.md)"), QStringLiteral("[text][reference]"),
        QStringLiteral("[[text]]"), QStringLiteral("[text](nested(one).md)"),
        QStringLiteral("[first\ntext](target.md)"), QStringLiteral("[first](\ntext.md\n)"),
        QStringLiteral("[text]\n\n[text]: target.md"), QStringLiteral("<text@example.com>")}) {
        const int position = source.contains("inner") ? source.indexOf("inner") : source.indexOf("text");
        QVERIFY2(!analyze(source, position).value("error").toString().isEmpty(), qPrintable(source));
    }
    QVERIFY(analyze("Ordinary text", 3).value("error").toString().isEmpty());
    const auto emptyLabel = analyze("[](target.md)", 1).value("existing").toMap();
    QVERIFY(!emptyLabel.isEmpty()); QCOMPARE(emptyLabel.value("label").toString(), QString());
}

void FomawriteTest::linkEditorAppliesAtomicRoundTripsAndNoOpSaves() {
    Cycle103SettingsSnapshot isolatedSettings;
    const QString original = QString::fromUtf8("Before café after.\n");
    Cycle114LinkFixture f; QVERIFY2(f.start(original), qPrintable(f.error));
    const auto revision = f.backend.documentRevision();
    QVERIFY(f.open(7, 11));
    QCOMPARE(f.field("linkEditorHeading")->property("text").toString(), "Insert link");
    f.field("linkEditorLabel")->setProperty("text", QString::fromUtf8("A [café]"));
    f.field("linkEditorUrl")->setProperty("text", QString::fromUtf8("notes/東京 (draft).md"));
    f.field("linkEditorTitle")->setProperty("text", "A \"quoted\" title");
    QVERIFY(f.click("linkEditorApplyButton"));
    QTRY_VERIFY(!f.popup->property("visible").toBool());
    const QString expected = QString::fromUtf8("Before [A \\[café\\]](<notes/東京 (draft).md> \"A \\\"quoted\\\" title\") after.\n");
    QCOMPARE(f.editor->property("text").toString(), expected);
    QVERIFY(f.backend.documentRevision() > revision);
    QCOMPARE(f.backend.sourceLinkAt(10), f.backend.resolveDocumentLink(QString::fromUtf8("notes/東京 (draft).md")));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "undo")); QCOMPARE(f.editor->property("text").toString(), original);
    QVERIFY(QMetaObject::invokeMethod(f.editor, "redo")); QCOMPARE(f.editor->property("text").toString(), expected);
    QVERIFY(f.open(10, 10));
    QCOMPARE(f.field("linkEditorHeading")->property("text").toString(), "Edit link");
    QCOMPARE(f.field("linkEditorApplyButton")->property("text").toString(), "Save");
    QCOMPARE(f.field("linkEditorUrl")->property("text").toString(), QString::fromUtf8("notes/東京 (draft).md"));
    const auto undoBeforeNoOp = f.editor->property("canUndo"), redoBeforeNoOp = f.editor->property("canRedo");
    const bool modifiedBeforeNoOp = f.backend.modified();
    QVERIFY(f.click("linkEditorApplyButton")); QTRY_VERIFY(!f.popup->property("visible").toBool());
    // QTextDocument revisions include focus/syntax formatting; Undo and exact
    // Markdown establish that a no-op save did not create an edit.
    QCOMPARE(f.editor->property("canUndo"), undoBeforeNoOp); QCOMPARE(f.editor->property("canRedo"), redoBeforeNoOp);
    QCOMPARE(f.backend.modified(), modifiedBeforeNoOp);
    QCOMPARE(f.editor->property("text").toString(), expected);
    QVERIFY(QMetaObject::invokeMethod(f.editor, "undo")); QCOMPARE(f.editor->property("text").toString(), original);
    // Preserve existing title quoting, empty labels and inline emphasis on an unchanged Save.
    for (const QString &existing : {QStringLiteral("[](target.md)"), QStringLiteral("[text]()"), QStringLiteral("[text](< a file.md > 'Title')"), QStringLiteral("[*text*](<a file.md> 'A title')")}) {
        f.editor->setProperty("text", existing);
        QVERIFY(f.open(1, 1)); const bool modified = f.backend.modified();
        const auto undo = f.editor->property("canUndo"), redo = f.editor->property("canRedo");
        QVERIFY(f.click("linkEditorApplyButton")); QTRY_VERIFY(!f.popup->property("visible").toBool());
        QCOMPARE(f.editor->property("text").toString(), existing); QCOMPARE(f.backend.modified(), modified);
        QCOMPARE(f.editor->property("canUndo"), undo); QCOMPARE(f.editor->property("canRedo"), redo);
    }
    f.editor->setProperty("text", "[\\*literal\\*](target.md)");
    QVERIFY(f.open(3, 3));
    f.field("linkEditorTitle")->setProperty("text", "A title");
    QVERIFY(f.click("linkEditorApplyButton")); QTRY_VERIFY(!f.popup->property("visible").toBool());
    QCOMPARE(f.editor->property("text").toString(), "[\\*literal\\*](target.md \"A title\")");
    // Go → Open Link accepts escaped destinations and rejects active URL schemes.
    f.editor->setProperty("text", "[text](folder/a\\(b\\).md \"title\") [bad](javascript:alert) [mail](mailto:a@example.com)");
    QCOMPARE(f.backend.sourceLinkAt(2), f.backend.resolveDocumentLink("folder/a(b).md"));
    QVERIFY(f.backend.sourceLinkAt(38).isEmpty());
    QCOMPARE(f.backend.sourceLinkAt(f.editor->property("text").toString().indexOf("mail]")), QUrl("mailto:a@example.com"));
    QCOMPARE(cycle103Read(f.directory.filePath("Draft.md")), original.toUtf8());
}

void FomawriteTest::linkEditorRejectsChangedAndReloadedTargets() {
    Cycle103SettingsSnapshot isolatedSettings;
    const QString original = "Before selected after.\n";
    Cycle114LinkFixture f; QVERIFY2(f.start(original, 720), qPrintable(f.error));
    for (int scenario = 0; scenario < 4; ++scenario) {
        QVERIFY(f.backend.open(QUrl::fromLocalFile(f.directory.filePath("Draft.md")))); QTest::qWait(100);
        QVERIFY(f.open(7, 15)); f.field("linkEditorUrl")->setProperty("text", "Kept destination.md");
        if (scenario <= 1) {
            QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Changed ")));
            if (scenario == 1) QVERIFY(QMetaObject::invokeMethod(f.editor, "undo"));
        } else if (scenario == 2) {
            QVERIFY(f.backend.open(QUrl::fromLocalFile(f.directory.filePath("Draft.md"))));
        } else {
            QVERIFY(cycle103Write(f.directory.filePath("Other.md"), original.toUtf8()));
            QVERIFY(f.backend.open(QUrl::fromLocalFile(f.directory.filePath("Other.md"))));
        }
        const QString before = f.editor->property("text").toString(); const auto revision = f.backend.documentRevision();
        QVERIFY(f.click("linkEditorApplyButton")); QTRY_VERIFY(f.popup->property("visible").toBool());
        QCOMPARE(f.editor->property("text").toString(), before); QCOMPARE(f.backend.documentRevision(), revision);
        QCOMPARE(f.field("linkEditorUrl")->property("text").toString(), "Kept destination.md");
        QVERIFY(f.field("linkEditorError")->property("text").toString().contains("document changed"));
        // The error label wraps and the popup clamps to its new height; on the
        // CI runner that takes longer than a fixed wait, so poll for it.
        const QRectF viewport(QPointF(), f.quick->size());
        const auto inside = [&](const char *name) {
            auto *item = f.field(name);
            return item && item->isVisible() && viewport.contains(QRectF(item->mapToScene(QPointF()), item->size()));
        };
        for (const char *name : {"linkEditorLabel", "linkEditorUrl", "linkEditorTitle", "linkEditorApplyButton", "linkEditorCancelButton", "linkEditorError"})
            QTRY_VERIFY2_WITH_TIMEOUT(inside(name), name, 3000);
        QVERIFY(f.click("linkEditorCancelButton")); QTRY_VERIFY(!f.popup->property("visible").toBool());
    }
}

void FomawriteTest::linkEditorCancelAndEscapeRestoreWritingSelection() {
    Cycle103SettingsSnapshot isolatedSettings;
    const QString source = "Before selected after.\n" + QString("An ordinary long paragraph to give the page a scroll position.\n\n").repeated(80);
    Cycle114LinkFixture f; QVERIFY2(f.start(source, 720), qPrintable(f.error));
    auto *scroll = f.window->findChild<QObject *>("editorScroll"); QVERIFY(scroll);
    const int selection = source.indexOf("ordinary", source.size() / 2);
    for (const char *fieldName : {"linkEditorLabel", "linkEditorUrl", "linkEditorTitle", "linkEditorCancelButton"}) {
        QVERIFY(f.open(selection + 8, selection));
        const qreal scrollBefore = f.popup->property("savedScrollY").toReal();
        QVERIFY(scrollBefore > 0);
        auto *field = f.field(fieldName); QVERIFY(field);
        if (QString(fieldName) == "linkEditorCancelButton") QVERIFY(f.click(fieldName));
        else { field->forceActiveFocus(); cycle111KeyClick(f.quick, Qt::Key_Escape); }
        QTRY_VERIFY(!f.popup->property("visible").toBool()); QTRY_VERIFY(f.editor->hasActiveFocus());
        QCOMPARE(f.editor->property("selectionStart").toInt(), selection); QCOMPARE(f.editor->property("selectionEnd").toInt(), selection + 8);
        QCOMPARE(f.editor->property("cursorPosition").toInt(), selection);
        QTRY_VERIFY(qAbs(scroll->property("contentY").toReal() - scrollBefore) < 1); QCOMPARE(f.editor->property("text").toString(), source);
        QVERIFY(!f.backend.modified());
    }
    QVERIFY(f.open(7, 15)); f.field("linkEditorUrl")->setProperty("text", "");
    QVERIFY(f.click("linkEditorApplyButton")); QVERIFY(f.popup->property("visible").toBool());
    QCOMPARE(f.editor->property("text").toString(), source); QVERIFY(f.field("linkEditorError")->isVisible());
    QVERIFY(f.click("linkEditorCancelButton")); QTRY_VERIFY(!f.popup->property("visible").toBool());
}
