            // Cycle125: exercise folder drop and real menu clicks, not only backend APIs.
            const QString folder = PublishingThemes::directory();
            const QString firstId = "cycle125-a", secondId = "cycle125-b", badId = "cycle125-bad";
            const auto writeTheme = [&](const QString &id, const QByteArray &css) {
                QSaveFile file(folder + '/' + id + ".css");
                return file.open(QIODevice::WriteOnly) && file.write(css) == css.size() && file.commit();
            };
            const auto menuItem = [&](const QString &name) { return window->findChild<QQuickItem *>(name); };
            const auto soleSelection = [&](const QString &prefix) {
                int checkedCount = 0;
                QString checkedName;
                for (auto *object : window->findChildren<QObject *>()) {
                    const QString name = object->objectName();
                    const bool theme = name.startsWith(prefix + "PublishingTheme_");
                    const QString suffix = name.mid((prefix + "Template").size());
                    const bool basic = name.startsWith(prefix + "Template") && suffix.size() == 1 && suffix[0].isDigit();
                    if ((theme || basic) && object->property("checked").toBool()) { ++checkedCount; checkedName = name; }
                }
                const QString expected = backend.publishingThemeId().isEmpty()
                    ? prefix + "Template" + QString::number(backend.outputStyle())
                    : prefix + "PublishingTheme_" + backend.publishingThemeId();
                return checkedCount == 1 && checkedName == expected;
            };
            const auto chooseItem = [&](const QString &name) {
                if (!check(click(window, buttons.value("previewTemplateButton")), "Cannot open theme folder menu")) return false;
                if (!check(waitUntil([&] { return menu->property("opened").toBool(); }), "Theme folder menu did not open for " + name)) return false;
                auto *item = menuItem(name);
                if (!check(item != nullptr, "Theme menu item missing: " + name)) return false;
                // Selecting an index scrolls the capped ListView to a lower action.
                for (int index = 0; index < menu->property("count").toInt(); ++index) {
                    QQuickItem *at = nullptr;
                    QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem *, at), Q_ARG(int, index));
                    if (at == item) { menu->setProperty("currentIndex", index); break; }
                }
                renderBarrier(window);
                if (!check(click(window, item), "Cannot click theme menu item: " + name)) return false;
                return check(waitUntil([&] { return !menu->property("visible").toBool(); }), "Theme action left its menu open");
            };
            themeCheck("folder-drop-selection-and-rejected-choice-checkmarks", [&] {
                if (!chooseFormat("web")) return;
                if (!check(writeTheme(firstId, "#write h1{color:rgb(12,34,56)!important}")
                        && writeTheme(secondId, "#write h1{color:rgb(65,43,21)!important}")
                        && writeTheme(badId, "#write{background:url(https://example.invalid/image.png)}"),
                        "Cannot create disposable folder themes")) return;
                if (!check(waitUntil([&] { return menuItem("previewPublishingTheme_" + secondId) != nullptr; }, 5000),
                           "Copied CSS themes did not appear automatically")) return;
                for (const QString &id : {firstId, secondId, secondId, firstId}) {
                    if (!chooseItem("previewPublishingTheme_" + id)) return;
                    if (!check(waitUntil([&] { return backend.publishingThemeId() == id && soleSelection("preview") && soleSelection("native"); }),
                               "Theme click did not retain exactly one authoritative checkmark")) return;
                    if (!check(themeReady("web"), "Folder theme Web preview did not settle")) return;
                    const QString expected = id == firstId ? "rgb(12, 34, 56)" : "rgb(65, 43, 21)";
                    check(publishingDomResult(web, "getComputedStyle(document.querySelector('#write h1')).color").toString() == expected,
                          "Theme checkmark changed without applying selected CSS");
                }
                if (!chooseItem("previewPublishingTheme_" + badId)) return;
                auto *notice = window->findChild<QObject *>("publishingThemeNotice");
                check(waitUntil([&] { return notice && notice->property("visible").toBool() && !backend.publishingThemeError().isEmpty(); }),
                      "Rejected theme did not show an actionable notice");
                check(backend.publishingThemeId() == firstId && soleSelection("preview") && soleSelection("native"),
                      "Rejected theme left an extra checkmark or changed the active theme");
                if (notice) QMetaObject::invokeMethod(notice, "close");
                capture("folder-theme-menu-selection");
                check(editor->property("text").toString() == themeSample && fileHash(sourcePath) == savedHash,
                      "Folder theme selection changed Markdown");
            });
            themeCheck("folder-css-edits-reload-and-basic-settings-actions", [&] {
                if (!check(backend.selectPublishingTheme(firstId), "Cannot restore folder fixture theme")) return;
                if (!check(writeTheme(firstId, "#write h1{color:rgb(76,54,32)!important}"), "Cannot atomically edit a folder theme")) return;
                check(waitUntil([&] {
                    return publishingDomResult(web, "getComputedStyle(document.querySelector('#write h1')).color").toString() == "rgb(76, 54, 32)";
                }, 10000), "Edited folder CSS did not refresh the actual preview automatically");
                if (!chooseItem("previewThemesReload")) return;
                check(backend.publishingThemeId() == firstId && soleSelection("preview") && soleSelection("native"),
                      "Reload changed the chosen theme or checkmarks");
                if (!chooseItem("previewTemplate3")) return;
                check(backend.publishingThemeId().isEmpty() && backend.outputStyle() == 3 && soleSelection("preview") && soleSelection("native"),
                      "Custom Settings did not select only the basic custom settings");
                if (!chooseItem("previewPublishingTheme_" + secondId)) return;
                check(backend.publishingThemeId() == secondId && soleSelection("preview") && soleSelection("native"),
                      "Switching from basic settings to folder CSS lost authoritative selection");
                capture("folder-theme-restored");
            });
            themeCheck("theme-import-and-basic-settings-dialog-actions", [&] {
                auto *importDialog = window->findChild<QObject *>("publishingThemeDialog");
                auto *settingsDialog = window->findChild<QObject *>("outputStyleDialog");
                auto *notice = window->findChild<QObject *>("publishingThemeNotice");
                auto *hub = window->findChild<QObject *>("exportHub");
                if (!check(importDialog && settingsDialog && notice && hub, "Theme action dialogs are missing")) return;
                const auto acceptFile = [&](QObject *dialog, const QString &path) {
                    const bool result = dialog->setProperty("selectedFile", QUrl::fromLocalFile(path))
                        && QMetaObject::invokeMethod(dialog, "accepted");
                    QMetaObject::invokeMethod(dialog, "close");
                    return check(result && waitUntil([&] { return !dialog->property("visible").toBool(); }), "Cannot complete isolated theme file dialog callback");
                };
                const QString importedPath = temporary.filePath("cycle125-imported.css");
                QFile imported(importedPath);
                if (!check(imported.open(QIODevice::WriteOnly), "Cannot create CSS import fixture")) return;
                imported.write("#write h1{color:#224466}"); imported.close();
                if (!chooseItem("previewThemeImport")) return;
                check(importDialog->property("visible").toBool(), "Import Theme did not open its file picker");
                if (!acceptFile(importDialog, importedPath)) return;
                const QString importedId = backend.publishingThemeId();
                check(importedId.startsWith("cycle125-imported-") && soleSelection("preview") && soleSelection("native"),
                      "Import Theme callback did not install and select the CSS theme");
                const QString jsonPath = temporary.filePath("cycle125-settings.json");
                QFile json(jsonPath);
                if (!check(json.open(QIODevice::WriteOnly), "Cannot create basic settings fixture")) return;
                json.write("{\"fontFamily\":\"Georgia\",\"pointSize\":14}"); json.close();
                if (!chooseItem("previewTemplateCustomLoad")) return;
                check(settingsDialog->property("visible").toBool(), "Load Basic Settings did not open its JSON picker");
                if (!acceptFile(settingsDialog, jsonPath)) return;
                check(backend.publishingThemeId().isEmpty() && backend.outputStyle() == 3 && backend.outputFont() == "Georgia"
                    && backend.outputPointSize() == 14 && soleSelection("preview") && soleSelection("native"),
                    "Loading basic JSON settings did not apply the chosen font/size/selection");
                if (!check(json.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot write invalid settings fixture")) return;
                json.write("not JSON"); json.close();
                if (!chooseItem("previewTemplateCustomLoad") || !acceptFile(settingsDialog, jsonPath)) return;
                check(notice->property("visible").toBool() && backend.outputFont() == "Georgia" && backend.outputPointSize() == 14
                    && soleSelection("preview") && soleSelection("native"), "Invalid JSON did not report failure while retaining accepted settings");
                QMetaObject::invokeMethod(notice, "close");
                check(waitUntil([&] { return !notice->property("visible").toBool(); }), "Settings error notice did not close");
                if (!chooseItem("previewTemplateCustomEdit")) return;
                check(hub->property("visible").toBool(), "Edit Basic Settings did not open its settings editor");
                QMetaObject::invokeMethod(hub, "close");
                check(waitUntil([&] { return !hub->property("visible").toBool(); }), "Basic settings editor did not close");
                if (!importedId.isEmpty()) QFile::remove(folder + '/' + importedId + ".css");
                check(editor->property("text").toString() == themeSample && fileHash(sourcePath) == savedHash,
                      "Theme management actions changed source text");
            });
            backend.setOutputStyle(0);
            for (const QString &id : {firstId, secondId, badId}) QFile::remove(folder + '/' + id + ".css");
            backend.reloadPublishingThemes();
