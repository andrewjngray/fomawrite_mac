    // Cycle 161: the PDF pane in the shipped bundle. A two-page PDF with known
    // text, written here with QPdfWriter, opens through the real Backend: the
    // reader replaces the preview, pages render, find locates the word, zoom
    // widens the page, and leaving PDF mode brings the preview back.
    const auto pdfCheck = [&](const QString &name, const std::function<void()> &exercise) {
        const qsizetype before = failures.size(); exercise();
        pdfChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
            {"newFailures", int(failures.size() - before)}});
    };
    pdfCheck("pdf-reader-pages-find-zoom-and-return", [&] {
        window->resize(1280, 800);
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        settings->setProperty("autoHideChrome", false);
        if (!loadSample("PDF return point", "# Before the PDF\n\nNotes come back here.\n")) return;
        const QUrl markdown = backend.fileUrl();
        action("previewSplitButton", 1, false);
        const QString pdfPath = output.filePath("pdf-check.pdf");
        {
            QPdfWriter writer(pdfPath);
            writer.setPageSize(QPageSize(QPageSize::A4));
            writer.setResolution(72);
            QPainter painter(&writer);
            painter.setFont(QFont(QStringLiteral("Helvetica"), 20));
            painter.drawText(72, 100, QStringLiteral("Fomawrite reads this page."));
            writer.newPage();
            painter.drawText(72, 100, QStringLiteral("The second page names a marigold."));
        }
        if (!check(backend.open(QUrl::fromLocalFile(pdfPath)), "Cannot open the generated PDF through Backend")) return;
        QQuickItem *reader = nullptr;
        check(waitUntil([&] { reader = window->findChild<QQuickItem *>("pdfReader");
                              return reader && reader->isVisible() && reader->property("ready").toBool(); }, 5000),
              "PDF reader did not replace the preview");
        if (!reader) return;
        check(!pane->isVisible(), "Publishing preview stayed visible in PDF mode");
        const std::function<QList<QQuickItem *>(QQuickItem *)> pagesOf = [&](QQuickItem *root) {
            QList<QQuickItem *> found;
            for (auto *child : root->childItems()) {
                if (child->objectName() == QLatin1String("pdfPage")) found << child;
                found << pagesOf(child);
            }
            return found;
        };
        auto *list = reader->findChild<QQuickItem *>("pdfPageList");
        check(list && list->property("count").toInt() == 2, "PDF reader does not list the two pages");
        if (!list) return;
        check(waitUntil([&] { for (auto *page : pagesOf(list)) if (page->property("rendered").toBool()) return true; return false; }, 5000),
              "No PDF page rendered");
        QMetaObject::invokeMethod(reader, "openFind", Q_ARG(QVariant, false));
        auto *field = reader->findChild<QQuickItem *>("pdfFindField");
        check(field && enterText(window, field, QStringLiteral("marigold")), "Cannot type into Find in PDF");
        auto *session = backend.pdf();
        check(waitUntil([&] { return !session->searching() && session->searchResults().size() == 1
                              && reader->property("findIndex").toInt() == 0; }, 5000),
              "Find in PDF did not locate the known word once");
        check(session->searchResults().value(0).toMap().value("page").toInt() == 1, "Find in PDF found the word on the wrong page");
        check(waitUntil([&] { return reader->property("currentPage").toInt() == 1; }), "Find did not bring the second page into view");
        capture("pdf-reader-find");
        QMetaObject::invokeMethod(reader, "closeFind");
        QQuickItem *firstPage = nullptr;
        for (auto *page : pagesOf(list)) if (page->property("page").toInt() == reader->property("currentPage").toInt()) firstPage = page;
        const qreal before = firstPage ? firstPage->width() : 0;
        check(click(window, reader->findChild<QQuickItem *>("pdfZoomIn")), "Cannot click PDF zoom in");
        check(waitUntil([&] { return firstPage && firstPage->width() > before + 1; }), "Zoom in did not widen the page");
        check(!backend.modified(), "Reading the PDF changed the notes");
        if (!check(backend.open(markdown), "Cannot return to the Markdown document")) return;
        auto *slot = window->findChild<QQuickItem *>("pdfReaderSlot");
        check(waitUntil([&] { return !session->active() && slot && !slot->property("active").toBool() && !slot->isVisible()
                              && pane->isVisible() && layout->property("effectiveLayoutMode").toInt() == 1; }, 5000),
              QString("Leaving PDF mode did not restore the preview pane (PDF open %1, reader %2, preview visible %3, layout %4/%5)")
                  .arg(session->active()).arg(slot && slot->property("active").toBool()).arg(pane->isVisible())
                  .arg(layout->property("layoutMode").toInt()).arg(layout->property("effectiveLayoutMode").toInt()));
    });
    // Cycle 163: a highlight made on the page is drawn over its word and
    // written into the notes as a marker and quote; Undo takes both away. The
    // notes are a file beside the PDF, reloaded at the end so the document is
    // left unmodified (an undone edit still counts as a change in Fomawrite).
    pdfCheck("pdf-highlight-overlay-notes-and-undo", [&] {
        const QString pdfPath = output.filePath("pdf-check.pdf");
        if (!check(QFileInfo::exists(pdfPath), "The generated PDF is missing")) return;
        const QString notesBefore = QStringLiteral("# Notes\n\nRead closely.\n");
        {
            QFile notes(pdfPath + QStringLiteral(".md"));
            if (!check(notes.open(QIODevice::WriteOnly | QIODevice::Truncate) && notes.write(notesBefore.toUtf8()) == notesBefore.toUtf8().size(),
                       "Cannot write the PDF's notes")) return;
        }
        const QUrl markdown = backend.fileUrl();
        if (!check(backend.open(QUrl::fromLocalFile(pdfPath)), "Cannot open the generated PDF with its notes")) return;
        QQuickItem *reader = nullptr;
        // The first group's reader may still await deletion: take the one in use.
        check(waitUntil([&] { reader = nullptr;
                              for (auto *item : window->findChildren<QQuickItem *>("pdfReader"))
                                  if (item->isVisible() && item->property("ready").toBool()) reader = item;
                              return reader != nullptr; }, 5000),
              QString("PDF reader did not open for marking (PDF %1, reader %2, visible %3, ready %4, layout %5)")
                  .arg(backend.pdf()->state()).arg(reader != nullptr).arg(reader && reader->isVisible())
                  .arg(reader && reader->property("ready").toBool()).arg(layout->property("effectiveLayoutMode").toInt()));
        if (!reader) return;
        check(waitUntil([&] { return editor->property("text").toString() == notesBefore; }), "The notes are not in the editor");
        auto *session = backend.pdf();
        auto *marks = backend.pdfMarks();
        const int start = int(session->pageText(0).indexOf(QStringLiteral("reads")));
        if (!check(start >= 0, "The generated PDF's text has no \"reads\"")) return;
        const QVariantMap selection = session->selectionAt(0, start, 5);
        const QRectF word = selection.value("rects").toList().value(0).toRectF();
        QMetaObject::invokeMethod(reader, "goToPage", Q_ARG(QVariant, 0));
        QMetaObject::invokeMethod(reader, "setSelection", Q_ARG(QVariant, 0), Q_ARG(QVariant, selection));
        QMetaObject::invokeMethod(reader, "focusPages");
        QVariant result;
        QMetaObject::invokeMethod(reader, "markSelection", Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(QVariant, QStringLiteral("highlight")), Q_ARG(QVariant, QStringLiteral("yellow")));
        check(result.toString() == QLatin1String("added"), "Highlighting the selection did not add a mark");
        const QString marker = QStringLiteral("<!-- fomawrite:pdf highlight page=1 colour=yellow text=%1+5 -->\n> reads\n").arg(start);
        check(waitUntil([&] { return editor->property("text").toString().contains(marker); }),
              "The notes do not hold the highlight's marker and quote: " + editor->property("text").toString());
        const std::function<QList<QQuickItem *>(QQuickItem *, const QString &)> named = [&](QQuickItem *root, const QString &name) {
            QList<QQuickItem *> found;
            for (auto *child : root->childItems()) {
                if (child->objectName() == name && child->isVisible()) found << child;
                found << named(child, name);
            }
            return found;
        };
        QQuickItem *page = nullptr;
        for (auto *item : named(reader, QStringLiteral("pdfPage"))) if (item->property("page").toInt() == 0) page = item;
        check(waitUntil([&] { return page && page->property("rendered").toBool() && named(reader, QStringLiteral("pdfMark_highlight")).size() == 1; }, 5000),
              "The highlight is not drawn on the rendered page");
        if (page) {
            check(renderBarrier(window), "Cannot render the highlighted page");
            const QImage image = window->grabWindow();
            const qreal scale = reader->property("pageScale").toReal();
            const QPointF below = page->mapToScene(QPointF(word.center().x(), word.bottom() + 0.16 * word.height()) * scale) * image.devicePixelRatio();
            const QPointF away = page->mapToScene(QPointF(word.center().x(), word.bottom() + 1.5 * word.height()) * scale) * image.devicePixelRatio();
            const QColor tinted = image.pixelColor(below.toPoint()), plain = image.pixelColor(away.toPoint());
            check(tinted.red() > 220 && tinted.green() > 200 && tinted.blue() < 170,
                  "The highlight is not drawn over the word (" + tinted.name() + ")");
            check(plain.red() > 235 && plain.green() > 235 && plain.blue() > 235, "The highlight spills beyond its line (" + plain.name() + ")");
            capture("pdf-highlight");
        }
        QMetaObject::invokeMethod(window, "undoEditing");
        check(waitUntil([&] { return editor->property("text").toString() == notesBefore; }, 5000), "Undo did not take the marker and quote out of the notes");
        check(waitUntil([&] { return marks->items().isEmpty() && named(reader, QStringLiteral("pdfMark_highlight")).isEmpty(); }),
              "Undo did not take the highlight off the page");
        backend.reloadFromDisk();
        check(waitUntil([&] { return !backend.modified() && editor->property("text").toString() == notesBefore; }), "The notes were left modified");
        if (!check(backend.open(markdown), "Cannot return to the Markdown document after marking")) return;
        check(waitUntil([&] { return !session->active(); }, 5000), "Leaving PDF mode after marking failed");
    });
