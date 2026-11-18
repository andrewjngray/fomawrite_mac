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
