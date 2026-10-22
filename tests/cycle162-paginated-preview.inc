#include "publishingpdf.h"
#include <QQuickItemGrabResult>
#include <QPrintPreviewDialog>
#include <QMetaProperty>

void FomawriteTest::windowsPaginatedPreviewRendersWithoutPrinter_data() {
    QTest::addColumn<bool>("live");
    QTest::newRow("source") << false;
    QTest::newRow("live") << true;
}

void FomawriteTest::windowsPaginatedPreviewRendersWithoutPrinter() {
    QFETCH(bool, live);
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString original = QString::fromUtf8("# First café page\n\nKeep Unicode 😀 and source bytes.\n\n<!-- pagebreak -->\n\n# Second page\n\nA second sheet.\n");
    const QString path = directory.filePath("preview.md");
    QVERIFY(cycle103Write(path, original.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    auto *desktopLoader=f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(desktopLoader);
    if(!f.window->property("isWindows").toBool()) desktopLoader->setProperty("active",true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    // Require the real component before exercising the test-only host opening
    // path. Native Windows additionally opens it from the actual File menu.
    QVERIFY2(QFileInfo::exists(QFINDTESTDATA("../src/WindowsPaginatedPreview.qml")),
             "Windows Paginated Preview must have a printer-independent component");
    auto *loader = f.window->findChild<QObject *>("windowsPaginatedPreviewLoader");
    QVERIFY2(loader, "Windows Paginated Preview must be wired into the real window");
    QVERIFY(!loader->property("active").toBool());
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QTRY_COMPARE(f.editor->property("text").toString(), original);
    QCoreApplication::processEvents();
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, original.size()), Q_ARG(QString, QStringLiteral("Unsaved ending."))));
    const QString draft = f.editor->property("text").toString();
    const int start = draft.indexOf(QString::fromUtf8("café"));
    QQuickItem *liveView = nullptr;
    if (live) {
        QVERIFY(QMetaObject::invokeMethod(f.window.data(), "setLiveEditing", Q_ARG(QVariant, true)));
        QTRY_VERIFY_WITH_TIMEOUT(f.backend.editorBridge()->isReady(), 15000);
        liveView = f.window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(liveView);
        QTRY_COMPARE_WITH_TIMEOUT(cycle143PageResult(liveView, "window.fomawriteEditor.getText()", 0).toString(), draft, 10000);
        const QString script = QString("(()=>{const v=window.fomawriteEditor.view;v.dispatch({selection:{anchor:%1,head:%2}});v.focus();return v.state.selection.main.to;})()").arg(start).arg(start + 4);
        QCOMPARE(cycle143PageResult(liveView, script, 0).toInt(), start + 4);
        liveView->forceActiveFocus();
    } else {
        f.editor->forceActiveFocus();
        QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, start), Q_ARG(int, start + 4)));
    }
    QFile css(directory.filePath("preview-blue.css")); QVERIFY(css.open(QIODevice::WriteOnly));
    css.write("#write h1{background:#1177cc!important;color:white!important;padding:20px;}@media print{*{-webkit-print-color-adjust:exact;print-color-adjust:exact;}}"); css.close();
    QVERIFY(f.backend.importPublishingTheme(QUrl::fromLocalFile(css.fileName())));
    const QString theme = f.backend.publishingThemeId();
    const auto removeTheme = qScopeGuard([&] { QFile::remove(QDir(PublishingThemes::directory()).filePath(theme + ".css")); });
    QVERIFY(f.backend.setExportPaperSize("letter")); QVERIFY(f.backend.setExportOrientation("landscape"));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings"); QVERIFY(settings);
    QVariantMap workspace, layout, persisted;
    for (const char *key : {"publishingFormat", "sourceZoom", "previewZoom", "linkedZoom"}) workspace.insert(key, settings->property(key));
    for (const char *key : {"effectiveLayoutMode", "liveEditEnabled", "organizerVisible", "filesVisible"}) layout.insert(key, f.layout->property(key));
    const auto syncSettings = [&] {
        for (auto *object : f.window->findChildren<QObject *>())
            if (object->inherits("QQmlSettings") && !QMetaObject::invokeMethod(object,"sync")) return false;
        return true;
    };
    QVERIFY(syncSettings());
    // Settings::sync flushes QSettings, but Qt's QML property cache is written
    // later. Wait for actual initial defaults before snapshotting persistence.
    QVariantMap initialDefaults;
    for(auto *object:f.window->findChildren<QObject *>()) if(object->inherits("QQmlSettings")) {
        const QString category=object->property("category").toString();
        for(int index=object->metaObject()->propertyOffset();index<object->metaObject()->propertyCount();++index) {
            const auto property=object->metaObject()->property(index);
            QVariant value=property.read(object);
            if(value.metaType()==QMetaType::fromType<QJSValue>()) value=value.value<QJSValue>().toVariant();
            initialDefaults.insert(category.isEmpty()?QString::fromLatin1(property.name()):category+'/'+property.name(),value);
        }
    }
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        QSettings stored; stored.sync();
        for(auto it=initialDefaults.cbegin();it!=initialDefaults.cend();++it)
            if(stored.value(it.key())!=it.value()) return false;
        return true;
    })(),3000);
    QSettings snapshot; snapshot.sync();
    for (const QString &key : snapshot.allKeys()) persisted.insert(key, snapshot.value(key));
    QSignalSpy saved(&f.backend, &Backend::saveSucceeded);
    const QUrl fileIdentity = f.backend.fileUrl();
    const int revision = f.backend.documentRevision();
    const bool undo = f.editor->property("canUndo").toBool(); QVERIFY(undo);
    const int renderers = f.backend.publisher()->findChildren<PublishingPdf *>().size();
    QSignalSpy requests(&f.backend, SIGNAL(paginatedPreviewRequested()));
    QVERIFY(requests.isValid());
    const auto open = [&] {
        auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
        auto *target = bar ? bar->findChild<QQuickItem *>("windows_printPreview") : nullptr;
        QObject *fileMenu = nullptr;
        if (bar) for (auto *candidate : bar->findChildren<QObject *>())
            if (candidate->property("title").toString() == "&File") { fileMenu = candidate; break; }
        QQuickItem *entry = nullptr;
        for (auto *candidate : f.window->findChildren<QQuickItem *>())
            if (candidate->inherits("QQuickMenuBarItem") && qvariant_cast<QObject *>(candidate->property("menu")) == fileMenu) { entry = candidate; break; }
        if (!target || !fileMenu || !entry || !cycle102Click(f.quick, entry)) return false;
        if (!QTest::qWaitFor([&] { return fileMenu->property("opened").toBool(); }, 3000)) return false;
        QTest::keyClick(f.quick, Qt::Key_Home);
        for (int step = 0; step < fileMenu->property("count").toInt() + 1; ++step) {
            if (target->property("highlighted").toBool()) break;
            QTest::keyClick(f.quick, Qt::Key_Down);
        }
        if (!QTest::qWaitFor([&] { return target->property("highlighted").toBool() && target->y()>0; },3000)) return false;
        auto *viewport=qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(fileMenu->property("contentItem")));
        if(!viewport) return false;
        const auto exposed = [&] {
            const auto point = target->mapToScene(QPointF(target->width()/2, target->height()/2));
            if (!target->isVisible() || !target->isEnabled()
                || !QRectF(QPointF(), f.quick->size()).contains(point)) return false;
            for (auto *parent = target->parentItem(); parent; parent = parent->parentItem())
                if (parent->clip() && !parent->mapRectToScene(parent->boundingRect()).contains(point)) return false;
            return true;
        };
        QSignalSpy frames(f.quick,&QQuickWindow::frameSwapped); f.quick->update();
        if(!QTest::qWaitFor([&]{return !frames.isEmpty();},1000)) return false;
        QElapsedTimer clock; clock.start();
        for(int step=0;step<32 && !exposed();++step) {
            const auto point=viewport->mapRectToScene(viewport->boundingRect()).center();
            const qreal before=viewport->property("contentY").toReal();
            QWheelEvent wheel(point,f.quick->mapToGlobal(point.toPoint()),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
            wheel.setTimestamp(quint64(clock.msecsSinceReference()+clock.elapsed()));
            QCoreApplication::sendEvent(f.quick,&wheel);
            if(!QTest::qWaitFor([&]{return viewport->property("contentY").toReal()>before;},1000)) return false;
            if(!QTest::qWaitFor([&]{return !viewport->property("moving").toBool();},1000)) return false;
        }
        if(!QTest::qWaitFor(exposed,1000)) return false;
#ifdef Q_OS_WIN
        QSignalSpy triggered(target, SIGNAL(triggered()));
        QTest::mouseClick(f.quick, Qt::LeftButton, Qt::NoModifier,
                          target->mapToScene(QPointF(target->width()/2, target->height()/2)).toPoint());
        return triggered.count() == 1;
#else
        QTest::keyClick(f.quick,Qt::Key_Escape);
        if(!QTest::qWaitFor([&]{return !fileMenu->property("visible").toBool();},3000)) return false;
        // Host coverage exercises the exact production wrapper and Publisher;
        // Backend's Windows signal/menu route remains a native-CI assertion.
        return QMetaObject::invokeMethod(f.window.data(), "openWindowsPaginatedPreview");
#endif
    };
    const auto unchanged = [&] {
        if (saved.count()!=0 || f.editor->property("text").toString() != draft || !f.backend.modified()
            || f.backend.documentRevision() != revision || f.editor->property("canUndo").toBool() != undo
            || cycle103Read(path) != original.toUtf8()
            || f.backend.fileUrl() != fileIdentity
            || f.backend.publishingThemeId() != theme
            || f.backend.exportPaperSize() != "letter" || f.backend.exportOrientation() != "landscape") {
            qWarning() << "Preview preservation document" << "textEqual" << (f.editor->property("text").toString()==draft)
                       << "modified" << f.backend.modified() << "revision" << f.backend.documentRevision() << revision
                       << "undo" << f.editor->property("canUndo") << undo << "diskEqual" << (cycle103Read(path)==original.toUtf8())
                       << "saved" << saved.count() << "theme" << f.backend.publishingThemeId() << theme;
            return false;
        }
        for (auto it=workspace.cbegin();it!=workspace.cend();++it) if(settings->property(it.key().toLatin1().constData())!=it.value()) {
            qWarning()<<"Preview preservation workspace"<<it.key()<<settings->property(it.key().toLatin1().constData())<<it.value(); return false;
        }
        for (auto it=layout.cbegin();it!=layout.cend();++it) if(f.layout->property(it.key().toLatin1().constData())!=it.value()) {
            qWarning()<<"Preview preservation layout"<<it.key()<<f.layout->property(it.key().toLatin1().constData())<<it.value(); return false;
        }
        if(!syncSettings()) return false;
        QSettings current; current.sync(); QVariantMap values;
        for(const QString &key:current.allKeys()) values.insert(key,current.value(key));
        if(values!=persisted) {
            for(const QString &key:values.keys()+persisted.keys())
                if(values.value(key)!=persisted.value(key)) qWarning()<<"Preview preservation persisted"<<key<<values.value(key)<<persisted.value(key);
            return false;
        }
        return true;
    };
    f.isolation.reset();
    for (int pass = 0; pass < 2; ++pass) {
        QVERIFY(open());
#ifdef Q_OS_WIN
        QCOMPARE(requests.count(), pass + 1);
#else
        QCOMPARE(requests.count(), 0);
#endif
        QTRY_VERIFY(loader->property("active").toBool());
        auto *dialog = qvariant_cast<QObject *>(loader->property("item")); QVERIFY(dialog);
        auto *quick = qobject_cast<QQuickWindow *>(dialog); QVERIFY(quick);
        QTRY_VERIFY(quick->isVisible()); QTRY_VERIFY(quick->isActive());
        QCOMPARE(quick->modality(), Qt::ApplicationModal);
        for (QWidget *widget : QApplication::topLevelWidgets())
            QVERIFY2(!qobject_cast<QPrintPreviewDialog *>(widget), "Windows paginated preview must not initialize a printer-backed viewer");
        QCOMPARE(dialog->property("zoomPercent").toInt(), 100);
        QCOMPARE(quick->transientParent(), f.quick);
        auto *surface = dialog->findChild<QQuickItem *>("paginatedPdfPreview"); QVERIFY(surface);
        QPointer<QObject> consumer(surface);
        QTRY_VERIFY_WITH_TIMEOUT(surface->property("pdfPagesReady").toBool() && !surface->property("viewportRefreshPending").toBool(), 35000);
        QVERIFY2(surface->property("errorText").toString().isEmpty(), qPrintable(surface->property("errorText").toString()));
        QPdfDocument pdf; QCOMPARE(pdf.load(surface->property("actualPdfSource").toUrl().toLocalFile()), QPdfDocument::Error::None);
        QCOMPARE(pdf.pageCount(), 2);
        for (int page = 0; page < 2; ++page) {
            QVERIFY(pdf.getAllText(page).text().contains(page ? "Second page" : QString::fromUtf8("First café page")));
            if(page==1) QVERIFY(pdf.getAllText(page).text().contains("Unsaved ending."));
            const auto points=pdf.pagePointSize(page); QVERIFY(qAbs(points.width()-792)<2 && qAbs(points.height()-612)<2);
            QVERIFY(QMetaObject::invokeMethod(surface, "scrollToFraction", Q_ARG(QVariant, page ? 1.0 : 0.0)));
            QQuickItem *image=nullptr;
            QTRY_VERIFY_WITH_TIMEOUT((image=cycle103VisualItems(surface, "publishingPdfPage_"+QString::number(page)).value(0)) != nullptr, 5000);
            QTRY_COMPARE_WITH_TIMEOUT(image->property("status").toInt(), 1, 5000); // Image.Ready.
            auto *paper=image->parentItem() ? image->parentItem()->parentItem() : nullptr; QVERIFY(paper);
            QCOMPARE(paper->property("color").value<QColor>(),QColor(Qt::white));
            auto grab=paper->grabToImage(); QVERIFY(grab);
            QTRY_VERIFY_WITH_TIMEOUT(!grab->image().isNull(), 5000);
            int blue=0; const QImage pixels=grab->image();
            for(int y=0;y<pixels.height();++y) for(int x=0;x<pixels.width();++x) {
                const auto color=pixels.pixelColor(x,y);
                if(qAbs(color.red()-17)<8 && qAbs(color.green()-119)<8 && qAbs(color.blue()-204)<8) ++blue;
            }
            QVERIFY2(blue>100, "Actual preview page image must paint the selected CSS, not a gray/empty viewer");
            qInfo()<<"Paginated PDF paint"<<(live?"live":"source")<<"reopen"<<pass<<"page"<<page+1<<"blue pixels"<<blue<<"size"<<pixels.size();
            if(pass==0) {
                const QString captures=QDir(QFileInfo(QFINDTESTDATA("tst_fomawrite.cpp")).absolutePath()).filePath("../research/cycle-162/captures");
                QVERIFY(QDir().mkpath(captures));
                QVERIFY(pixels.save(QDir(captures).filePath(QString("%1-page%2.png").arg(live?"live":"source").arg(page+1))));
            }
        }
        if(pass==0) {
            const QString captures=QDir(QFileInfo(QFINDTESTDATA("tst_fomawrite.cpp")).absolutePath()).filePath("../research/cycle-162/captures");
            const QImage frame=quick->grabWindow(); QVERIFY(!frame.isNull());
            QVERIFY(frame.save(QDir(captures).filePath(live?"live-window.png":"source-window.png")));
        }
        auto *zoom=dialog->findChild<QQuickItem *>("paginatedZoomIn"); QVERIFY(zoom);
        QVERIFY(cycle102Click(quick,zoom)); QCOMPARE(dialog->property("zoomPercent").toInt(),110);
        QTest::keyClick(quick, Qt::Key_X);
        QTest::keyClick(quick, Qt::Key_S, Qt::ControlModifier);
        QVERIFY(unchanged());
        QPointer<QQuickWindow> closingWindow(quick);
        const QKeySequence closingSequence(QKeySequence::Close);
        QCOMPARE(closingSequence.count(),1);
        const auto closingKey=closingSequence[0];
        if(pass==0) QTest::keyClick(quick,Qt::Key_Escape);
        else QTest::keyPress(quick,closingKey.key(),closingKey.keyboardModifiers());
        QTRY_VERIFY(!loader->property("active").toBool());
        QTRY_VERIFY(consumer.isNull());
        QTRY_VERIFY(closingWindow.isNull());
        // QtTest's modifier keyRelease still dereferences its original QWindow
        // after releasing the letter and processing events. A closing window
        // may die there; send Qt key-release to the new focus owner instead.
        if(pass==1) QTest::keyRelease(f.quick,closingKey.key(),closingKey.keyboardModifiers());
        QTRY_COMPARE(f.backend.publisher()->findChildren<PublishingPdf *>().size(),renderers);
        QVERIFY(unchanged());
    }
    // Closing before the debounce/render completes must retire this consumer
    // too; a later request must never address the destroyed preview window.
    QVERIFY(open());
    auto *earlyDialog = qvariant_cast<QObject *>(loader->property("item")); QVERIFY(earlyDialog);
    auto *earlySurface = earlyDialog->findChild<QQuickItem *>("paginatedPdfPreview"); QVERIFY(earlySurface);
    QPointer<QObject> earlyConsumer(earlySurface);
    QVERIFY(earlySurface->property("viewportRefreshPending").toBool());
    QVERIFY(qobject_cast<QQuickWindow *>(earlyDialog)->close());
    QTRY_VERIFY(!loader->property("active").toBool());
    QTRY_VERIFY(earlyConsumer.isNull());
    QTRY_COMPARE(f.backend.publisher()->findChildren<PublishingPdf *>().size(),renderers);
    QVERIFY(unchanged());
    if(live) {
        QCOMPARE(cycle143PageResult(liveView,"window.fomawriteEditor.getText()",0).toString(),draft);
        const auto selection=cycle143PageResult(liveView,"JSON.stringify({from:window.fomawriteEditor.view.state.selection.main.from,to:window.fomawriteEditor.view.state.selection.main.to})",0).toString();
        const auto selected=QJsonDocument::fromJson(selection.toUtf8()).object();
        QCOMPARE(selected.value("from").toInt(),start); QCOMPARE(selected.value("to").toInt(),start+4);
    } else {
        QCOMPARE(f.editor->property("selectionStart").toInt(),start); QCOMPARE(f.editor->property("selectionEnd").toInt(),start+4);
    }
    QVERIFY(QMetaObject::invokeMethod(f.editor,"undo")); QTRY_COMPARE(f.editor->property("text").toString(),original);
    QVERIFY(QMetaObject::invokeMethod(f.editor,"redo")); QTRY_COMPARE(f.editor->property("text").toString(),draft);
    f.backend.discardRecovery();
}
