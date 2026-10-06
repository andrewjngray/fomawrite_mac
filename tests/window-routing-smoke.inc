// Included only in the FOMAWRITE_CONTEXT_SMOKE integration-test build.
// Exercise actual context-menu dispatch, session creation and native tabbing.
const auto smokeCheck = [&](bool ok, const char *message) {
    if (!ok) { qCritical("FAIL: %s", message); app.exit(1); }
    else qInfo("PASS: %s", message);
    return ok;
};
const QUrl smokeFile = QUrl::fromLocalFile(smokeDirectory.filePath("Clicked.md"));
QFile smokeSource(smokeFile.toLocalFile());
smokeSource.open(QIODevice::WriteOnly);
smokeSource.write("# A composed writing workspace\n\nA disposable document for native workspace checks.\n\n"
                  "## Keep the writing in view\n\nThe organizer, files and writing canvas each own their controls. "
                  "A narrow window keeps the document available and remembers the requested columns.\n\n"
                  "- Local Markdown remains the source.\n- Layout changes preserve the document.\n\n"
                  "## Return to the wider view\n\nThe requested columns return when there is room.\n");
smokeSource.close();
const auto smokeTrigger = [&](const std::shared_ptr<Session> &session, const QString &label) {
    auto *menu = session->window->findChild<QObject *>("libraryContextMenu");
    if (!menu) return false;
    menu->setProperty("target", session->backend->libraryItemInfo(smokeFile));
    for (auto *item : menu->findChildren<QObject *>())
        if (item->property("text").toString() == label && item->metaObject()->indexOfSignal("triggered()") >= 0)
            return QMetaObject::invokeMethod(item, "triggered");
    return false;
};
const auto smokeWaitFor = [&](std::function<bool()> ready, std::function<void()> next) {
    auto *timer = new QTimer(&app);
    timer->setInterval(40);
    QObject::connect(timer, &QTimer::timeout, &app, [timer, ready, next] {
        if (!ready()) return;
        timer->stop(); timer->deleteLater(); next();
    });
    timer->start();
};
#include "cycle100-navigation-native.inc"
std::function<void(int)> smokePrint;
smokePrint = [&](int index) {
    const QStringList labels{"Print Rendered Document…", "Print Markdown Source…", "Paginated Preview…"};
    if (index == labels.size()) {
        if (!smokeCheck(sessions.first()->backend->libraryItemAction(smokeFile, "reveal"), "Finder reveal request accepted for sample")) return;
        if (!smokeCheck(sessions.first()->backend->libraryItemAction(smokeFile, "share"), "native sharing picker request accepted for sample")) return;
        QTimer::singleShot(300, &app, [&] {
            for (auto &session : sessions) session->backend->discardRecovery();
            app.guardedQuit();
        });
        return;
    }
    auto seen = std::make_shared<bool>(false);
    auto *cancel = new QTimer(&app);
    cancel->setInterval(100);
    QObject::connect(cancel, &QTimer::timeout, &app, [seen, cancel] {
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (qobject_cast<QPrintDialog *>(widget) || qobject_cast<QPrintPreviewDialog *>(widget)) {
                *seen = true;
                static_cast<QDialog *>(widget)->reject();
                cancel->stop(); cancel->deleteLater();
                return;
            }
        }
    });
    cancel->start();
    if (!smokeCheck(smokeTrigger(sessions.first(), labels[index]), "dispatch print submenu action")) return;
    smokeWaitFor([seen] { return *seen; }, [&, index] {
        qInfo() << "PASS: opened and canceled" << QStringList{"rendered print", "source print", "paginated preview"}[index];
        smokePrint(index + 1);
    });
};

// Read the same QML state used by the production checkpoint writer, rather
// than querying geometry that may have been temporarily contracted.
const auto smokePaneState = [](QWindow *window) {
    QVariant state = window->property("workspacePaneState");
    if (state.metaType() == QMetaType::fromType<QJSValue>())
        state = state.value<QJSValue>().toVariant();
    return state.toMap();
};
const auto smokeRestorePanes = [](QWindow *window, const QVariantMap &state) {
    return QMetaObject::invokeMethod(window, "restoreWorkspacePaneState", Q_ARG(QVariant, state));
};
#include "cycle103-migration.inc"
const auto smokeHeadersAlign = [](QWindow *window) {
    const auto aligns = [window](const char *headerName, const char *paneName) {
        auto *header = window->findChild<QQuickItem *>(QString::fromLatin1(headerName));
        auto *pane = window->findChild<QQuickItem *>(QString::fromLatin1(paneName));
        if (!header || !pane || header->isVisible() != pane->isVisible()) return false;
        if (!pane->isVisible()) return true;
        return qAbs(header->mapToScene(QPointF()).x() - pane->mapToScene(QPointF()).x()) < 1
            && qAbs(header->width() - pane->width()) < 1 && pane->width() > 0;
    };
    auto *editor = window->findChild<QQuickItem *>(QStringLiteral("editorPane"));
    auto *previewHeader = window->findChild<QQuickItem *>(QStringLiteral("previewHeader"));
    auto *preview = window->findChild<QQuickItem *>(QStringLiteral("previewPane"));
    if (!editor || !previewHeader || !preview) return false;
    return aligns("organizerHeader", "organizerSlot") && aligns("filesHeader", "filesSlot")
        && aligns("documentHeader", editor->isVisible() ? "editorPane" : "previewPane")
        && (previewHeader->isVisible() == (editor->isVisible() && preview->isVisible()))
        && (!previewHeader->isVisible()
            || (qAbs(previewHeader->mapToScene(QPointF()).x() - preview->mapToScene(QPointF()).x()) < 1
                && qAbs(previewHeader->width() - preview->width()) < 1));
};
const QString smokeCaptureDirectory = qEnvironmentVariable("FOMAWRITE_WORKSPACE_CAPTURE_DIR");
const auto smokeCaptureWorkspace = [&](QWindow *window, const QString &name) {
    if (smokeCaptureDirectory.isEmpty()) return true;
    auto *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (!smokeCheck(quickWindow && QDir().mkpath(smokeCaptureDirectory), "prepare app-only workspace capture")) return false;
    const QImage image = quickWindow->grabWindow();
    return smokeCheck(!image.isNull() && image.save(QDir(smokeCaptureDirectory).filePath(name)),
                      "save synthetic app-only workspace screenshot");
};
std::shared_ptr<Session> smokeWorkspaceOriginal, smokeWorkspaceSecond, smokeWorkspaceTab;
QVariantMap smokeOriginalPanes, smokeSecondPanes, smokeTabPanes;
QString smokeWorkspaceText;
QString smokeWorkspaceSavedText;
bool smokeWorkspaceModified = false;
bool smokeWorkspaceUndo = false;
int smokeWorkspaceSelectionStart = 0;
int smokeWorkspaceSelectionEnd = 0;
// These checks exercise Qt's native accessibility interfaces and actual shortcut
// dispatch. They do not stand in for listening to VoiceOver or changing displays.
const auto smokeContainsFocus = [](QWindow *window, const QString &objectName) {
    auto *quick = qobject_cast<QQuickWindow *>(window);
    auto *root = window->findChild<QQuickItem *>(objectName);
    for (auto *item = quick ? quick->activeFocusItem() : nullptr; item; item = item->parentItem())
        if (item == root) return true;
    return false;
};
const auto smokeFocusDiagnostic = [](QWindow *window, const char *phase) {
    auto *quick = qobject_cast<QQuickWindow *>(window);
    auto *focused = quick ? quick->activeFocusItem() : nullptr;
    qInfo() << "FOCUS DIAGNOSTIC:" << phase << "active" << window->isActive()
            << "applicationState" << QGuiApplication::applicationState()
            << "focusWindowIsTarget" << (QGuiApplication::focusWindow() == window)
            << "focusItem" << (focused ? focused->objectName() : QStringLiteral("<none>"))
            << "focusItemActive" << (focused && focused->hasActiveFocus())
            << "workspaceOwnsKeyboard" << window->property("workspaceOwnsKeyboard");
    for (auto *object : window->findChildren<QObject *>()) {
        const auto sequence = object->property("sequence").toString();
        if (sequence.contains("F6"))
            qInfo() << "FOCUS SHORTCUT:" << sequence << "enabled" << object->property("enabled")
                    << "context" << object->property("context");
    }
};
const auto smokeKey = [](QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    QKeyEvent press(QEvent::KeyPress, key, modifiers);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, modifiers);
    QCoreApplication::sendEvent(window, &release);
};
const auto smokeTabRoundTrip = [&](std::function<void()> next) {
#ifdef Q_OS_MACOS
    smokeWorkspaceTab->backend->nativeWindowAction(QStringLiteral("detach"));
    smokeWaitFor([&] { return macWorkspaceState(smokeWorkspaceTab->window).isEmpty()
                               && macWorkspaceState(smokeWorkspaceOriginal->window).isEmpty(); }, [&, next] {
        if (!smokeCheck(smokePaneState(smokeWorkspaceOriginal->window) == smokeOriginalPanes
                       && smokePaneState(smokeWorkspaceTab->window) == smokeTabPanes,
                       "detaching a native tab preserves both independent pane layouts")) return;
        restoreMacWorkspaceTabs({smokeWorkspaceOriginal->window, smokeWorkspaceTab->window});
        smokeWaitFor([&] {
            const QString group = macWorkspaceState(smokeWorkspaceOriginal->window).value("group").toString();
            return !group.isEmpty() && group == macWorkspaceState(smokeWorkspaceTab->window).value("group").toString();
        }, [&, next] {
            if (!smokeCheck(macWorkspaceState(smokeWorkspaceSecond->window).isEmpty()
                           && smokePaneState(smokeWorkspaceOriginal->window) == smokeOriginalPanes
                           && smokePaneState(smokeWorkspaceTab->window) == smokeTabPanes
                           && smokePaneState(smokeWorkspaceSecond->window) == smokeSecondPanes,
                           "production native tab restoration keeps separate-window layouts independent")) return;
            next();
        });
    });
#else
    next();
#endif
};
void smokeActivateApplication(QWindow *window);
int smokeActivationAttempts = 0;
std::function<void(std::function<void()>)> smokeAcceptance;
smokeAcceptance = [&](std::function<void()> next) {
    auto *window = smokeWorkspaceSecond->window.data();
    if (!window->isActive() || QGuiApplication::focusWindow() != window
        || QGuiApplication::applicationState() != Qt::ApplicationActive) {
        if (++smokeActivationAttempts > 20) {
            smokeFocusDiagnostic(window, "native keyboard activation timed out");
            smokeCheck(false, "native keyboard fixture activates its application and target window");
            return;
        }
        smokeActivateApplication(window);
        QTimer::singleShot(100, &app, [&, next] { smokeAcceptance(next); });
        return;
    }
    if (!smokeCheck(true, "native keyboard fixture has active application and target window")) return;
    auto *workspaceButton = window->findChild<QQuickItem *>(QStringLiteral("topChromeLibraryButton"));
    auto *access = QAccessible::queryAccessibleInterface(workspaceButton);
    if (!smokeCheck(access && access->role() == QAccessible::Button
                    && access->text(QAccessible::Name) == QStringLiteral("Workspace columns and document modes"),
                    "workspace control exposes its button role and descriptive accessible name")) return;
    auto *source = window->findChild<QQuickItem *>(QStringLiteral("sourceEditor"));
    auto *sourceAccess = QAccessible::queryAccessibleInterface(source);
    if (!smokeCheck(sourceAccess && sourceAccess->text(QAccessible::Name) == QStringLiteral("Markdown editor")
                   && sourceAccess->textInterface(), "source editor exposes named accessible text interface")) return;
    source->forceActiveFocus();
    smokeFocusDiagnostic(window, "before Source F6");
    smokeKey(window, Qt::Key_F6);
    QTimer::singleShot(100, &app, [&, next] {
        auto *window = smokeWorkspaceSecond->window.data();
        smokeFocusDiagnostic(window, "after Source F6");
        if (!smokeCheck(smokeContainsFocus(window, QStringLiteral("previewPane")), "F6 moves keyboard focus from Source into Preview")) return;
        smokeKey(window, Qt::Key_F6);
        QTimer::singleShot(100, &app, [&, next] {
            auto *window = smokeWorkspaceSecond->window.data();
            if (!smokeCheck(smokeContainsFocus(window, QStringLiteral("topChrome")), "F6 reaches the workspace toolbar")) return;
            smokeKey(window, Qt::Key_F6, Qt::ShiftModifier);
            QTimer::singleShot(100, &app, [&, next] {
                auto *window = smokeWorkspaceSecond->window.data();
                if (!smokeCheck(smokeContainsFocus(window, QStringLiteral("previewPane")), "Shift+F6 traverses workspace regions in reverse")) return;
                QMetaObject::invokeMethod(window, "focusWorkspaceRegion", Q_ARG(QVariant, QStringLiteral("files")));
                window->resize(720, 820);
                QTimer::singleShot(180, &app, [&, next] {
                    auto *window = smokeWorkspaceSecond->window.data();
                    auto *quick = qobject_cast<QQuickWindow *>(window);
                    if (!smokeCheck(quick->activeFocusItem() && quick->activeFocusItem()->isVisible()
                        && smokeContainsFocus(window, QStringLiteral("editorPane")),
                        "contraction transfers keyboard focus out of a hidden Files pane")) return;
                    auto *settings = window->findChild<QObject *>(QStringLiteral("workspaceSettings"));
                    settings->setProperty("toolbarVisibilityMode", 0);
                    QMetaObject::invokeMethod(window, "focusWorkspaceRegion", Q_ARG(QVariant, QStringLiteral("toolbar")));
                    auto *chrome = window->findChild<QObject *>(QStringLiteral("topChrome"));
                    if (!smokeCheck(chrome->property("keyboardReveal").toBool()
                                    && chrome->property("toolbarContentOpacity").toDouble() == 1,
                                    "keyboard toolbar focus reveals faded actions")) return;
                    QMetaObject::invokeMethod(window, "toggleWorkspacePane", Q_ARG(QVariant, QStringLiteral("files")));
                    auto *drawer = window->findChild<QObject *>(QStringLiteral("workspaceNavigationDrawer"));
                    smokeWaitFor([drawer] { return drawer->property("opened").toBool(); }, [&, drawer, next] {
                        auto *window = smokeWorkspaceSecond->window.data();
                        auto *close = window->findChild<QQuickItem *>(QStringLiteral("navigationCloseButton"));
                        auto *closeAccess = QAccessible::queryAccessibleInterface(close);
                        if (!smokeCheck(close && close->hasActiveFocus() && closeAccess
                            && closeAccess->text(QAccessible::Name) == QStringLiteral("Close navigation"),
                            "compact navigation opens with a named, focused Close control")) return;
                        smokeKey(window, Qt::Key_Escape);
                        smokeWaitFor([drawer] { return !drawer->property("visible").toBool(); }, [&, next] {
                            auto *window = smokeWorkspaceSecond->window.data();
                            auto *button = window->findChild<QQuickItem *>(QStringLiteral("topChromeLibraryButton"));
                            if (!smokeCheck(button->hasActiveFocus(), "Escape closes compact navigation and restores its initiating keyboard focus")) return;
                            window->findChild<QObject *>(QStringLiteral("workspaceSettings"))->setProperty("toolbarVisibilityMode", 1);
                            window->resize(1440, 820);
                            smokeWorkspaceTab->window->raise(); smokeWorkspaceTab->window->requestActivate();
                            QTimer::singleShot(240, &app, [&, next] {
                                auto *window = smokeWorkspaceSecond->window.data();
                                if (!smokeCheck(!window->isActive() && smokeWorkspaceTab->window->isActive(),
                                                "separate native tab window can become active without altering the inactive workspace")) return;
                                if (!smokeCheck(smokeHeadersAlign(window) && smokePaneState(window) == smokeSecondPanes,
                                                "inactive workspace keeps aligned columns and requested state")) return;
                                auto *inactiveButton = window->findChild<QObject *>(QStringLiteral("topChromeLibraryButton"));
                                if (!smokeCheck(!inactiveButton->property("focusIndicated").toBool(),
                                                "inactive window suppresses the active keyboard focus ring")) return;
                                if (!smokeCaptureWorkspace(window, QStringLiteral("workspace-inactive.png"))) return;
                                window->raise(); window->requestActivate();
                                QMetaObject::invokeMethod(window, "toggleFullScreen");
                                smokeWaitFor([window] { return window->visibility() == QWindow::FullScreen; }, [&, next] {
                                    QTimer::singleShot(900, &app, [&, next] {
                                        auto *window = smokeWorkspaceSecond->window.data();
                                        if (!smokeCheck(smokeHeadersAlign(window) && smokePaneState(window) == smokeSecondPanes,
                                                        "native full screen preserves aligned headers and requested pane state")) return;
                                        if (!smokeCaptureWorkspace(window, QStringLiteral("workspace-fullscreen.png"))) return;
                                        QMetaObject::invokeMethod(window, "toggleFullScreen");
                                        smokeWaitFor([window] { return window->visibility() == QWindow::Windowed; }, [&, next] {
                                            QTimer::singleShot(900, &app, [&, next] {
                                                auto *window = smokeWorkspaceSecond->window.data();
                                                if (!smokeCheck(smokeHeadersAlign(window) && smokePaneState(window) == smokeSecondPanes,
                                                                "leaving native full screen restores the workspace composition")) return;
                                                auto *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
                                                if (!smokeCheck(editor->property("text").toString() == smokeWorkspaceText
                                                    && editor->property("selectionStart").toInt() == smokeWorkspaceSelectionStart
                                                    && editor->property("selectionEnd").toInt() == smokeWorkspaceSelectionEnd
                                                    && editor->property("canUndo").toBool() && smokeWorkspaceSecond->backend->modified(),
                                                    "keyboard navigation, activation and full screen preserve the draft, selection and Undo")) return;
                                                smokeTabRoundTrip(next);
                                            });
                                        });
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    });
};
std::function<void(int)> smokeResizeWorkspace;
smokeResizeWorkspace = [&](int index) {
    const QList<int> widths{1440, 1120, 900, 720, 1440};
    auto *window = smokeWorkspaceSecond->window.data();
    if (index == widths.size()) {
        smokeAcceptance([&] {
        smokeWorkspaceSecond->backend->setThemePreset(QStringLiteral("dark"));
        QTimer::singleShot(200, &app, [&] {
            if (!smokeCheck(smokeHeadersAlign(smokeWorkspaceSecond->window), "dark headers retain pane alignment")) return;
            if (!smokeCaptureWorkspace(smokeWorkspaceSecond->window, QStringLiteral("workspace-dark.png"))) return;
            smokeWorkspaceSecond->backend->setThemePreset(QStringLiteral("studio"));
            auto *editor = smokeWorkspaceSecond->window->findChild<QObject *>(QStringLiteral("sourceEditor"));
            if (!smokeCheck(editor && QMetaObject::invokeMethod(editor, "undo"), "undo remains available after native workspace changes")) return;
            QTimer::singleShot(120, &app, [&] {
                auto *editor = smokeWorkspaceSecond->window->findChild<QObject *>(QStringLiteral("sourceEditor"));
                if (!smokeCheck(editor && editor->property("text").toString() == smokeWorkspaceSavedText,
                               "Undo restores the exact saved source after native resizing")) return;
                // The editor intentionally keeps its modified flag after Undo.
                // Reload this disposable file to leave native Quit unprompted.
                smokeWorkspaceSecond->backend->reloadFromDisk();
                if (!smokeCheck(!smokeWorkspaceSecond->backend->modified(), "reload disposable QA source before native Quit")) return;
                smokePrint(0);
            });
        });
        });
        return;
    }
    window->resize(widths[index], 820);
    window->raise(); window->requestActivate();
    QTimer::singleShot(200, &app, [&, index] {
        auto *window = smokeWorkspaceSecond->window.data();
        auto *layout = window->findChild<QObject *>(QStringLiteral("workspaceLayout"));
        const QList<int> widths{1440, 1120, 900, 720, 1440};
        if (!smokeCheck(window->width() == widths[index], "native window reaches requested QA width")) return;
        if (!smokeCheck(layout && layout->property("effectiveOrganizerVisible").toBool() == (index == 0 || index == 4)
                       && layout->property("effectiveFilesVisible").toBool() == (index <= 1 || index == 4)
                       && layout->property("effectiveLayoutMode").toInt() == (index == 3 ? 0 : 1),
                       "native contraction hides columns in order and restores the requested split")) return;
        if (!smokeCheck(smokePaneState(window) == smokeSecondPanes, "resize preserves desired visibility, widths and mode")) return;
        if (!smokeCheck(smokePaneState(smokeWorkspaceOriginal->window) == smokeOriginalPanes
                       && smokePaneState(smokeWorkspaceTab->window) == smokeTabPanes,
                       "resizing a separate window leaves both native-tab pane states independent")) return;
        if (!smokeCheck(smokeHeadersAlign(window), "headers align with actual pane boundaries after native resize")) return;
        auto *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        if (!smokeCheck(editor && editor->property("text").toString() == smokeWorkspaceText
                       && editor->property("canUndo").toBool() == smokeWorkspaceUndo
                       && editor->property("selectionStart").toInt() == smokeWorkspaceSelectionStart
                       && editor->property("selectionEnd").toInt() == smokeWorkspaceSelectionEnd
                       && smokeWorkspaceSecond->backend->modified() == smokeWorkspaceModified,
                       "native layout changes preserve source, selection, undo availability and dirty state")) return;
        persist();
        const auto saved = WorkspaceStore::read(workspacePath);
        if (!smokeCheck(saved.size() == 3 && saved[1].toObject()["paneState"].toObject().toVariantMap() == smokeSecondPanes,
                       "production workspace checkpoint saves desired state during contraction")) return;
        if (index == 3 && !smokeCaptureWorkspace(window, QStringLiteral("workspace-compact.png"))) return;
        if (index == 4 && !smokeCaptureWorkspace(window, QStringLiteral("workspace-wide.png"))) return;
        qInfo() << "PASS: workspace resize width" << widths[index];
        smokeResizeWorkspace(index + 1);
    });
};
const auto smokeWorkspace = [&](const std::shared_ptr<Session> &original,
                                const std::shared_ptr<Session> &second,
                                const std::shared_ptr<Session> &tab) {
    smokeWorkspaceOriginal = original; smokeWorkspaceSecond = second; smokeWorkspaceTab = tab;
    auto *studioAction = second->window->findChild<QObject *>(QStringLiteral("studioWorkspaceAction"));
    const int outputStyle = second->backend->outputStyle();
    auto *editor = second->window->findChild<QObject *>(QStringLiteral("sourceEditor"));
    if (!smokeCheck(editor && studioAction, "native workspace exposes source and Studio action")) return;
    smokeWorkspaceSavedText = editor->property("text").toString();
    if (!smokeCheck(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, editor->property("length").toInt()),
                                            Q_ARG(QString, QStringLiteral("\nA native QA draft edit.\n")))
                   && QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 4), Q_ARG(int, 12)),
                   "prepare an unsaved native QA edit and selection")) return;
    smokeWorkspaceText = editor->property("text").toString();
    smokeWorkspaceModified = second->backend->modified();
    smokeWorkspaceUndo = editor->property("canUndo").toBool();
    smokeWorkspaceSelectionStart = editor->property("selectionStart").toInt();
    smokeWorkspaceSelectionEnd = editor->property("selectionEnd").toInt();
    if (!smokeCheck(smokeWorkspaceModified && smokeWorkspaceUndo, "native QA begins with dirty source and usable Undo")) return;
    if (!smokeCheck(QMetaObject::invokeMethod(studioAction, "triggered"), "invoke Studio writing layout action")) return;
    auto *settings = second->window->findChild<QObject *>(QStringLiteral("workspaceSettings"));
    const auto studioState = smokePaneState(second->window);
    if (!smokeCheck(second->backend->themePreset() == QStringLiteral("studio") && settings
                   && settings->property("writingAppearance").toString() == QStringLiteral("editorial")
                   && studioState.value("layoutMode").toInt() == 0
                   && studioState.value("organizerVisible").toBool() && studioState.value("filesVisible").toBool()
                   && second->backend->outputStyle() == outputStyle,
                   "Studio action applies the writing composition and preserves output style")) return;
    smokeOriginalPanes = {{"version", 2}, {"organizerVisible", false}, {"filesVisible", true},
        {"layoutMode", 0}, {"liveEditEnabled", false}, {"lastEditingLayoutMode", 0}, {"organizerWidth", 232.0}, {"fileWidth", 304.0}, {"previewWidth", 400.0}};
    smokeSecondPanes = {{"version", 2}, {"organizerVisible", true}, {"filesVisible", true},
        {"layoutMode", 1}, {"liveEditEnabled", false}, {"lastEditingLayoutMode", 1}, {"organizerWidth", 216.0}, {"fileWidth", 296.0}, {"previewWidth", 360.0}};
    smokeTabPanes = {{"version", 2}, {"organizerVisible", true}, {"filesVisible", false},
        {"layoutMode", 2}, {"liveEditEnabled", false}, {"lastEditingLayoutMode", 0}, {"organizerWidth", 224.0}, {"fileWidth", 320.0}, {"previewWidth", 420.0}};
    if (!smokeCheck(smokeRestorePanes(original->window, smokeOriginalPanes)
                   && smokeRestorePanes(second->window, smokeSecondPanes)
                   && smokeRestorePanes(tab->window, smokeTabPanes), "restore three independent per-window pane states")) return;
    QTimer::singleShot(160, &app, [&] {
        if (!smokeCheck(smokePaneState(smokeWorkspaceOriginal->window) == smokeOriginalPanes
                       && smokePaneState(smokeWorkspaceSecond->window) == smokeSecondPanes
                       && smokePaneState(smokeWorkspaceTab->window) == smokeTabPanes,
                       "native tabs and separate windows retain distinct requested layouts")) return;
        persist();
        const auto saved = WorkspaceStore::read(workspacePath);
        if (!smokeCheck(saved.size() == 3, "production checkpoint saves every test window")) return;
        const auto restoredState = saved[1].toObject()["paneState"].toObject().toVariantMap();
        if (!smokeCheck(smokeRestorePanes(smokeWorkspaceSecond->window, smokeOriginalPanes)
                       && smokeRestorePanes(smokeWorkspaceSecond->window, restoredState)
                       && smokePaneState(smokeWorkspaceSecond->window) == smokeSecondPanes,
                       "saved production pane state restores through the root API")) return;
        smokeResizeWorkspace(0);
    });
};
QTimer::singleShot(200, &app, [&] {
    if (!smokeCheck(sessions.size() == 1, "one initial test window")) return;
    auto original = sessions.first();
    if (!smokeReferenceMigration(original)) return;
    if (!smokeCheck(original->backend->open(smokeFile), "open disposable source")) return;
    if (!smokeCheck(smokeTrigger(original, "Open in New Window"), "trigger New Window from context menu")) return;
    smokeWaitFor([&] { return sessions.size() >= 2; }, [&, original] {
        if (!smokeCheck(sessions.size() == 2, "New Window creates a second editor for the selected open file")) return;
        auto second = sessions.last();
        if (!smokeCheck(QFileInfo(second->backend->fileUrl().toLocalFile()).canonicalFilePath()
                       == QFileInfo(smokeFile.toLocalFile()).canonicalFilePath(), "new window loads the clicked file")) return;
#ifdef Q_OS_MACOS
        if (!smokeCheck(macWorkspaceState(second->window).isEmpty(), "New Window is separate from native tab groups")) return;
#endif
        if (!smokeCheck(smokeTrigger(original, "Open in New Tab"), "trigger New Tab from context menu")) return;
        smokeWaitFor([&] { return sessions.size() >= 3; }, [&, original, second] {
            if (!smokeCheck(sessions.size() == 3, "New Tab creates a third editor")) return;
            auto tab = sessions.last();
#ifdef Q_OS_MACOS
            const auto sourceGroup = macWorkspaceState(original->window).value("group").toString();
            if (!smokeCheck(!sourceGroup.isEmpty() && sourceGroup == macWorkspaceState(tab->window).value("group").toString(),
                            "new tab joins its source window")) return;
            if (!smokeCheck(macWorkspaceState(second->window).isEmpty(), "unrelated window stays separate")) return;
#endif
            createWindow(smokeFile);
            if (!smokeCheck(sessions.size() == 3, "ordinary opening still focuses an existing editor")) return;
            smokeNavigation(original, second, tab, [&, original, second, tab] {
                // Let documentLoaded's deferred cursor reset finish before this
                // independent acceptance fixture creates its selected draft.
                QTimer::singleShot(120, &app, [&, original, second, tab] {
                    smokeWorkspace(original, second, tab);
                });
            });
        });
    });
});
QTimer::singleShot(45000, &app, [&] { qCritical("Window routing/workspace integration test timed out"); app.exit(2); });
