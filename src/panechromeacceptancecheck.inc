    // Cycle117: pane-specific controls, file identity and compact library rows.
    const auto chromeCheck = [&](const QString &name, const std::function<void()> &exercise) {
        const qsizetype before = failures.size();
        exercise();
        paneChromeChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
            {"newFailures", int(failures.size() - before)}, {"windowWidth", window->width()},
            {"windowHeight", window->height()}});
    };
    const auto paneAnchorCheck = [&] {
        check(renderBarrier(window), "Cannot render pane-owned footer controls");
        auto *sourcePane = child("editorPane");
        auto *appearance = buttons.value("sourceAppearanceButton");
        auto *outputStyle = buttons.value("previewTemplateButton");
        auto *visual = child("documentEditingControls");
        auto *modeGroup = child("documentViewControls");
        if (!check(sourcePane && appearance && outputStyle && visual && modeGroup, "Pane-owned footer controls are missing")) return;
        check(appearance->isVisible() == sourcePane->isVisible(), "Source appearance is assigned to the wrong pane");
        check(outputStyle->isVisible() == pane->isVisible(), "Preview template is assigned to the wrong pane");
        for (const auto &[control, owner] : {std::pair{appearance, sourcePane}, std::pair{outputStyle, pane}}) {
            if (!control->isVisible()) continue;
            check(qAbs(bounds(control).left() - bounds(owner).left() - 12) <= 0.5
                  && bounds(control).right() <= bounds(owner).right() - 11.5, "Style control is not contained at its pane's leading edge: " + control->objectName());
        }
        auto *publishing = child("documentPublishingControls");
        check(visual->isVisible() == sourcePane->isVisible(), "Editing capsule lost its editor pane");
        if (visual->isVisible()) {
            const qreal paneEnd = pane->isVisible() ? bounds(sourcePane).right() : bounds(modeGroup).left();
            check(qAbs(bounds(visual).right() - paneEnd + 12) <= 1.5
                  && bounds(appearance).right() + 7.5 <= bounds(visual).left(), "Editing capsule is not beneath its pane's trailing edge");
        }
        check(publishing && publishing->isVisible() == pane->isVisible(), "Publishing capsule lost its output pane");
        if (publishing && publishing->isVisible())
            check(bounds(outputStyle).right() + 7.5 <= bounds(publishing).left()
                  && bounds(publishing).right() + 7.5 <= bounds(modeGroup).left(), "Publishing controls overlap output style or arrangement");
        check(qAbs(bounds(modeGroup).right() - window->width() + 12) <= 0.5, "View capsule moved from the right edge");
    };
    chromeCheck("pane-footer-divider-and-compact-menu-access", [&] {
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        window->resize(1280, 720); backend.setThemePreset("studio");
        const QString text = QString::fromUtf8("# Pane tools\n\nA café draft keeps its Markdown — 你好.\n");
        if (!loadSample("Pane tools", text)) return;
        QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, text.size()), Q_ARG(QString, "Unsaved ending."));
        const QString draft = editor->property("text").toString();
        action("previewSplitButton", 1, false); paneAnchorCheck();
        const qreal oldTemplateX = bounds(buttons.value("previewTemplateButton")).left();
        const qreal oldPreviewWidth = pane->width();
        auto *divider = child("documentSplitDivider");
        if (!check(divider, "Document divider is unavailable for pane-footer check")) return;
        const QPointF start = bounds(divider).center();
        const auto pointer = [&](QEvent::Type type, const QPointF &point, Qt::MouseButton button, Qt::MouseButtons pressed) {
            QMouseEvent event(type, point, point, window->mapToGlobal(point.toPoint()), button, pressed, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &event); QCoreApplication::processEvents();
        };
        pointer(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        pointer(QEvent::MouseMove, start + QPointF(70, 0), Qt::NoButton, Qt::LeftButton);
        pointer(QEvent::MouseButtonRelease, start + QPointF(70, 0), Qt::LeftButton, Qt::NoButton);
        check(waitUntil([&] { return ready(); }), "Divider drag did not settle"); paneAnchorCheck();
        check(pane->width() <= oldPreviewWidth - 50 && bounds(buttons.value("previewTemplateButton")).left() >= oldTemplateX + 50,
              "Preview style control did not follow a real divider drag");
        capture("pane-footer-wide-split");
        for (int width : {960, 803}) {
            window->resize(width, 620); settle(80);
            QMetaObject::invokeMethod(layout, "updateWidth", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 320));
            action("previewSplitButton", 1, false); paneAnchorCheck();
            for (const auto &[trigger, menuName, itemName] : {
                     std::tuple{"sourceAppearanceButton", "sourceAppearanceMenu", "sourceAppearanceBook"},
                     std::tuple{"previewTemplateButton", "previewTemplateMenu", "previewTemplate6"}}) {
                check(click(window, buttons.value(trigger)), "Cannot open pane style menu");
                auto *menu = window->findChild<QObject *>(menuName);
                check(menu && waitUntil([&] { return menu->property("opened").toBool(); }), "Pane style menu did not open");
                check(click(window, child(itemName)), "Cannot choose pane style from compact menu");
                check(editor->property("text").toString() == draft, "Pane style selection changed Markdown");
            }
            capture(QString("pane-footer-narrow-split-%1").arg(width));
        }
        backend.setThemePreset("dark"); settle(80); capture("pane-footer-narrow-dark");
        window->resize(1280, 720); action("previewOnly", 2, false); paneAnchorCheck(); capture("pane-footer-full-preview");
        sourceSingle(); paneAnchorCheck(); capture("pane-footer-source");
        QMetaObject::invokeMethod(window, "undoEditing"); check(editor->property("text").toString() == text, "Pane tools added an Undo unit");
        QMetaObject::invokeMethod(window, "redoEditing"); check(editor->property("text").toString() == draft, "Pane tools changed Redo");
    });
    chromeCheck("preview-header-file-identity-and-edited-state", [&] {
        window->resize(1280, 720); backend.setThemePreset("studio");
        const QString filename = QString::fromUtf8("Actual café filename with a different heading");
        const QString text = "# This heading is not the filename\n\nA synthetic writing sample.\n";
        if (!loadSample(filename, text)) return;
        action("previewSplitButton", 1, false);
        auto *name = child("previewDocumentName"), *edited = child("previewDocumentEdited"), *icon = child("previewDocumentIcon");
        auto *fullName = child("fullPreviewDocumentName"), *fullEdited = child("fullPreviewDocumentEdited");
        if (!check(name && edited && icon && fullName && fullEdited, "Preview header identity controls are missing")) return;
        check(renderBarrier(window), "Cannot render header identity");
        check(name->isVisible() && icon->isVisible() && !edited->isVisible()
              && name->property("text").toString() == filename + ".md" && name->property("font").value<QFont>().bold(),
              "Saved Preview header does not show its actual bold filename");
        QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, text.size()), Q_ARG(QString, "An unsaved edit."));
        const QString draft = editor->property("text").toString();
        check(waitUntil([&] { return edited->isVisible(); }), "Preview header omitted Edited state");
        capture("preview-header-edited-wide");
        window->resize(803, 620); settle(80);
        QMetaObject::invokeMethod(layout, "updateWidth", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 320));
        action("previewSplitButton", 1, false);
        check(renderBarrier(window), "Cannot render narrow Preview identity");
        auto *header = child("previewHeader");
        check(name->isVisible() && edited->isVisible() && bounds(header).contains(bounds(name)) && bounds(header).contains(bounds(edited))
              && bounds(name).right() <= bounds(edited).left(), "Narrow Preview filename/Edited overlap or disappear");
        capture("preview-header-edited-narrow");
        action("previewOnly", 2, false);
        check(fullName->isVisible() && fullEdited->isVisible() && fullName->property("text").toString() == filename + ".md",
              "Full Preview loses file identity or Edited state");
        capture("preview-header-full-edited");
        backend.save(); check(waitUntil([&] { return !backend.modified() && !fullEdited->isVisible(); }), "Save did not clear Edited state");
        QFile savedFile(backend.fileUrl().toLocalFile());
        check(savedFile.open(QIODevice::ReadOnly) && savedFile.readAll() == draft.toUtf8(), "Header save workflow did not preserve exact source");
        capture("preview-header-saved");
    });
    chromeCheck("compact-library-actual-filenames-and-child-opening", [&] {
        backend.setThemePreset("studio"); window->resize(1440, 800);
        sourceSingle();
        const QString sampleRoot = temporary.filePath("Library samples"); QDir().mkpath(sampleRoot + "/Research");
        const QList<QPair<QString, QString>> files{
            {"First actual file.md", "# A deliberately different heading\n\nA longer public sample excerpt that wraps naturally within the compact library row without becoming a tall card.\n"},
            {"Second actual file.md", "# Another heading\n\nA second calm writing sample.\n"},
            {"Research/Child actual file.md", "# A child heading\n\nA document inside a real folder.\n"}};
        for (const auto &[name, text] : files) {
            QFile file(sampleRoot + "/" + name);
            check(file.open(QIODevice::WriteOnly) && file.write(text.toUtf8()) == text.toUtf8().size(), "Cannot create library sample");
        }
        auto *model = backend.library();
        model->setProperty("rootFolder", QUrl::fromLocalFile(sampleRoot)); model->setProperty("navigationMode", 0);
        layout->setProperty("organizerVisible", true); layout->setProperty("filesVisible", true); layout->setProperty("fileWidth", 340);
        auto *library = child("libraryPane");
        library->setProperty("showExcerpts", true); library->setProperty("dateMode", 1);
        std::function<QList<QQuickItem *>(QQuickItem *)> rows = [&](QQuickItem *parent) {
            QList<QQuickItem *> found;
            for (auto *item : parent->childItems()) {
                if (item->objectName() == "libraryEntry") found.append(item);
                found.append(rows(item));
            }
            return found;
        };
        const auto entry = [&](const QString &name) -> QQuickItem * {
            for (auto *row : rows(library)) {
                auto *label = row->findChild<QQuickItem *>("libraryEntryTitle");
                if (label && label->property("text").toString() == name) return row;
            }
            return nullptr;
        };
        check(waitUntil([&] { return entry("Research") && entry("First actual file.md"); }), "Library rows did not appear");
        if (!check(entry("Research"), "Library folder row missing")) return;
        check(click(window, entry("Research")), "Cannot expand real library folder");
        check(waitUntil([&] { return entry("Child actual file.md"); }), "Expanded library child did not appear");
        auto *first = entry("First actual file.md"), *nested = entry("Child actual file.md"), *folder = entry("Research");
        if (!check(first && nested && folder, "Named library sample rows are unavailable")) return;
        check(renderBarrier(window), "Cannot render compact library rows");
        check(nested->property("leftPadding").toReal() == folder->property("leftPadding").toReal() + 18
              && first->property("leftPadding") == folder->property("leftPadding"), "Child indentation is not based on real tree depth");
        for (auto *row : {first, nested}) {
            auto *name = row->findChild<QQuickItem *>("libraryEntryTitle"), *date = row->findChild<QQuickItem *>("libraryEntryDate");
            auto *excerpt = row->findChild<QQuickItem *>("libraryEntryExcerpt"), *icon = row->findChild<QQuickItem *>("libraryEntryIcon");
            if (!check(name && date && excerpt && icon, "Library row metadata missing")) continue;
            check(icon->isVisible() && date->isVisible() && excerpt->isVisible()
                  && name->property("font").value<QFont>().weight() >= QFont::DemiBold
                  && bounds(name).right() < bounds(date).left()
                  && qAbs(bounds(name).center().y() - bounds(date).center().y()) < 2
                  && excerpt->property("maximumLineCount").toInt() == 2
                  && excerpt->property("color") == date->property("color") && row->height() <= 66,
                  "Compact library row does not retain its icon/name/date/two-line excerpt layout");
        }
        check(click(window, first), "Cannot open actual filename row");
        check(waitUntil([&] { return backend.fileUrl() == QUrl::fromLocalFile(QFileInfo(sampleRoot + "/First actual file.md").canonicalFilePath()); }), "Filename row opened the wrong document");
        check(editor->property("text").toString() == files[0].second, "Actual-file click lost the document text");
        capture("compact-library-light");
        backend.setThemePreset("dark"); settle(80); capture("compact-library-dark");
        check(waitUntil([&] { return entry("Child actual file.md"); }), "Child row disappeared after opening another document");
        check(click(window, entry("Child actual file.md")), "Cannot open indented child row");
        check(waitUntil([&] { return backend.fileUrl() == QUrl::fromLocalFile(QFileInfo(sampleRoot + "/Research/Child actual file.md").canonicalFilePath()); }), "Child row opened the wrong document");
        check(editor->property("text").toString() == files[2].second && !backend.modified(), "Child navigation changed source or dirty state");
    });
