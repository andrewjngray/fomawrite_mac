    // Cycle124: synthetic content only; never loads or captures private writing.
    {
        const auto themeCheck = [&](const QString &name, const std::function<void()> &exercise) {
            const qsizetype before = failures.size();
            exercise();
            publishingThemeChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
                {"newFailures", int(failures.size() - before)}});
        };
        const QString themeSample = QString::fromUtf8(
            "# Synthetic NPS review\n\n"
            "A **synthetic customer survey** compares invented results. Café and 你好 remain ordinary Markdown.\n\n"
            "> Synthetic observation: clearer onboarding helps customers explain the product.\n\n"
            "## Invented results\n\n"
            "| Segment | Responses | NPS |\n| --- | ---: | ---: |\n"
            "| Example North | 120 | +38 |\n| Example South | 95 | +24 |\n\n"
            "- Keep feedback attached to an invented segment.\n- Review the sample each quarter.\n\n"
            "<!-- pagebreak -->\n\n"
            "## Synthetic follow-up\n\n"
            "These fictional numbers exist only to verify publishing output.\n\n"
            "```text\nexample_nps = promoters - detractors\n```\n");
        window->resize(1440, 900);
        backend.setThemePreset("studio");
        backend.setOutputStyle(1);
        check(backend.setExportPaperSize("a4") && backend.setExportOrientation("portrait"),
              "Cannot select A4 portrait for synthetic publishing checks");
        settings->setProperty("autoHideChrome", false);
        settings->setProperty("publishingFormat", "web");
        layout->setProperty("organizerVisible", false);
        layout->setProperty("filesVisible", false);
        if (zoom) {
            QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, "source"), Q_ARG(QVariant, 100));
            QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 100));
        }
        if (loadSample("Synthetic publishing theme", themeSample)) {
            action("previewSplitButton", 1, false);
            check(QMetaObject::invokeMethod(window, "equalizeDocumentPanes") && waitUntil(ready),
                  "Cannot balance document panes for synthetic publishing screenshots");
            renderBarrier(window);
            auto *web = pane->findChild<QQuickItem *>("renderedPreview");
            auto *pdfScroll = pane->findChild<QQuickItem *>("previewScroll");
            auto *menu = window->findChild<QObject *>("previewTemplateMenu");
            auto *choice = child("previewPublishingTheme_claude-like");
            const QString sourcePath = backend.fileUrl().toLocalFile();
            const QString savedHash = fileHash(sourcePath);
            const auto themeReady = [&](const QString &format) {
                return waitUntil([&] {
                    return ready() && !pane->property("holdingPreviousFrame").toBool()
                        && pane->property("documentIdentity").toString() == backend.publishingDocumentIdentity()
                        && pane->property("displayedDocumentIdentity").toString() == backend.publishingDocumentIdentity()
                        && pane->property("publishingMode").toString() == format
                        && !pane->property("outputUrl").toUrl().isEmpty()
                        && QFileInfo(pane->property("outputUrl").toUrl().toLocalFile()).suffix() == (format == "pdf" ? "pdf" : "html")
                        && (format != "pdf" || pane->property("pdfPagesReady").toBool());
                }, 30000);
            };
            const auto chooseFormat = [&](const QString &format) {
                return check(click(window, buttons.value(format == "pdf" ? "pdfPublishingButton" : "webPublishingButton")),
                             "Cannot choose synthetic " + format + " output")
                    && check(themeReady(format), "Synthetic " + format + " publishing did not settle")
                    && check(pane->property("errorText").toString().isEmpty(),
                             "Synthetic publishing error: " + pane->property("errorText").toString());
            };
            themeCheck("footer-theme-menu-and-semantic-web", [&] {
                if (!check(web && pdfScroll && menu && choice, "Publishing theme menu or surfaces are missing")) return;
                if (!check(click(window, buttons.value("previewTemplateButton")), "Cannot open footer publishing theme menu")) return;
                if (!check(waitUntil([&] { return menu->property("opened").toBool(); }),
                           "Footer output style menu did not open")) return;
                auto *themes = window->findChild<QObject *>("previewCustomThemesMenu");
                QQuickItem *entry = nullptr;
                QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem *, entry), Q_ARG(int, 0));
                if (!check(themes && entry && entry->property("text").toString() == "Custom Themes",
                           "Footer output style menu has no Custom Themes entry")) return;
                renderBarrier(window);
                const QPointF point = bounds(entry).center();
                QMouseEvent move(QEvent::MouseMove, point, point, window->mapToGlobal(point.toPoint()),
                                 Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &move);
                if (!check(waitUntil([&] { return themes->property("opened").toBool() && choice->isVisible(); }),
                           "Hovering Custom Themes did not expose Claude Like")) return;
                check(choice->property("text").toString() == "Claude Like", "Footer theme label differs from Claude Like");
                if (!check(click(window, choice), "Cannot choose Claude Like from the footer menu")) return;
                if (!check(waitUntil([&] { return !menu->property("visible").toBool() && !themes->property("visible").toBool()
                        && backend.publishingThemeId() == "claude-like"; }),
                           "Footer selection did not activate Claude Like")) return;
                if (!check(themeReady("web"), "Claude Like Web preview did not settle")) return;
                sourceScroll->setProperty("contentY", 0);
                check(QMetaObject::invokeMethod(pane, "scrollToFraction", Q_ARG(QVariant, 0.0)) && themeReady("web"),
                      "Cannot return synthetic Web review to the top of its document");
                const QVariantMap dom = publishingDomResult(web, QStringLiteral(R"JS((() => {
                    const body = getComputedStyle(document.body), h1 = document.querySelector('#write > h1'),
                        quote = document.querySelector('#write blockquote'), table = document.querySelector('#write table');
                    const q = quote ? getComputedStyle(quote) : null, t = table ? getComputedStyle(table) : null;
                    return { title: h1 ? h1.textContent.trim() : '', background: body.backgroundColor,
                        family: body.fontFamily, headingFamily: h1 ? getComputedStyle(h1).fontFamily : '',
                        quote: quote ? quote.textContent.trim() : '', quoteBackground: q ? q.backgroundColor : '',
                        quoteBorder: q ? q.borderLeftWidth : '', tableCollapse: t ? t.borderCollapse : '',
                        tableBorder: t ? t.borderTopWidth : '', rows: table ? Array.from(table.rows).map(
                            row => Array.from(row.cells).map(cell => cell.textContent.trim())) : [],
                        pagebreaks: document.querySelectorAll('.pagebreak').length, scripts: document.scripts.length };
                })())JS")).toMap();
                report.insert("publishingThemeDom", QJsonObject::fromVariantMap(dom));
                const QVariantList rows = dom.value("rows").toList();
                check(dom.value("title").toString() == "Synthetic NPS review" && dom.value("background").toString() == "rgb(250, 249, 245)"
                    && dom.value("family").toString().startsWith("Georgia") && dom.value("headingFamily") == dom.value("family"),
                    "Claude Like Web preview lost its cream surface or serif typography");
                check(dom.value("quote").toString().startsWith("Synthetic observation:")
                    && dom.value("quoteBackground").toString() == "rgb(243, 237, 229)" && dom.value("quoteBorder").toString() == "2px"
                    && rows.size() == 3 && rows.first().toList() == QVariantList{QString("Segment"), QString("Responses"), QString("NPS")}
                    && rows.at(1).toList() == QVariantList{QString("Example North"), QString("120"), QString("+38")}
                    && dom.value("tableCollapse").toString() == "collapse" && dom.value("tableBorder").toString() == "1px"
                    && dom.value("pagebreaks").toInt() == 1 && dom.value("scripts").toInt() == 0,
                    "Claude Like Web output lost semantic quotes, table cells, pagebreak or script isolation");
                check(editor->property("text").toString() == themeSample && !backend.modified() && fileHash(sourcePath) == savedHash,
                      "Selecting a publishing theme changed source or saved UTF-8 bytes");
                // DOM completion precedes the compositor. Let the exposed Cocoa
                // window paint normally, then capture once; a grab in a polling
                // predicate would conceal the missing natural update being tested.
                settle(350);
                QJsonArray paintedColors;
                const QImage frame = window->grabWindow();
                bool painted = !frame.isNull() && web && web->isVisible();
                if (painted) {
                    const QRectF area = bounds(web);
                    const double sx = double(frame.width()) / window->width();
                    const double sy = double(frame.height()) / window->height();
                    for (double fraction : {0.25, 0.5, 0.75}) {
                        const QPoint sample(qBound(0, qRound((area.left() + area.width() * fraction) * sx), frame.width() - 1),
                                            qBound(0, qRound((area.top() + 5) * sy), frame.height() - 1));
                        const QColor color = frame.pixelColor(sample);
                        paintedColors.append(color.name());
                        painted = painted && qAbs(color.red() - 250) <= 2 && qAbs(color.green() - 249) <= 2
                            && qAbs(color.blue() - 245) <= 2;
                    }
                }
                report.insert("publishingThemeWebPaint", QJsonObject{{"passed", painted}, {"colors", paintedColors},
                    {"previewWidth", pane->width()}, {"sourceWidth", child("editorPane") ? child("editorPane")->width() : 0}});
                if (check(painted, "The actual Web texture did not paint Claude Like before its screenshot")) capture("theme-web");
            });
            themeCheck("browser-pdf-theme-pagination-paper-and-margins", [&] {
                if (!chooseFormat("pdf")) return;
                const QUrl url = pane->property("outputUrl").toUrl();
                QFile file(url.toLocalFile());
                if (!check(file.open(QIODevice::ReadOnly), "Cannot read synthetic browser PDF")) return;
                const QByteArray bytes = file.readAll();
                QPdfDocument pdf;
                if (!check(bytes.startsWith("%PDF-") && pdf.load(url.toLocalFile()) == QPdfDocument::Error::None,
                           "Synthetic PDF is not a readable PDF document")) return;
                check(pdf.pageCount() == 2 && pdf.metaData(QPdfDocument::MetaDataField::Creator).toString().contains("Chromium"),
                      "The themed PDF did not use browser pagination for its two explicit pages");
                const QSizeF size = pdf.pagePointSize(0);
                const QString text = pdf.getAllText(0).text();
                const int headingIndex = text.indexOf("Synthetic NPS review");
                const QRectF heading = headingIndex < 0 ? QRectF() : pdf.getSelectionAtIndex(0, headingIndex, 20).boundingRectangle();
                const double leftMargin = QSettings().value("output/pageMarginLeft", 18).toDouble() * 72.0 / 25.4;
                const double topMargin = QSettings().value("output/pageMarginTop", 18).toDouble() * 72.0 / 25.4;
                const QImage raster = pdf.render(0, QSize(qRound(size.width()), qRound(size.height())));
                int creamSamples = 0;
                if (!raster.isNull()) {
                    for (int y = int(topMargin) + 10; y < raster.height() - int(topMargin) - 10; y += 20)
                        for (int x = int(leftMargin) + 10; x < raster.width() - int(leftMargin) - 10; x += 20) {
                            const QColor color = raster.pixelColor(x, y);
                            if (qAbs(color.red() - 250) <= 1 && qAbs(color.green() - 249) <= 1 && qAbs(color.blue() - 245) <= 1) ++creamSamples;
                        }
                }
                report.insert("publishingThemePdf", QJsonObject{{"pageCount", pdf.pageCount()}, {"widthPoints", size.width()},
                    {"heightPoints", size.height()}, {"creator", pdf.metaData(QPdfDocument::MetaDataField::Creator).toString()},
                    {"headingBounds", rectJson(heading)}, {"leftMarginPoints", leftMargin}, {"topMarginPoints", topMargin},
                    {"creamSamples", creamSamples}, {"sha256", sha256(bytes)}});
                check(qAbs(size.width() - 595) < 2 && qAbs(size.height() - 842) < 2 && !heading.isEmpty()
                    && heading.left() >= leftMargin - 1 && heading.left() <= leftMargin + 12 && heading.top() >= topMargin - 1,
                    "Browser PDF did not retain A4 portrait and the configured content margins");
                check(creamSamples > 20 && bytes.contains("Georgia") && text.contains("Example North")
                    && pdf.getAllText(1).text().contains("Synthetic follow-up"),
                    "Browser PDF lost the selected cream/serif theme or semantic document content");
                check(pdfScroll && pdfScroll->isVisible() && web && !web->isVisible() && renderBarrier(window),
                      "The actual themed PDF pages were not visible in the publishing pane");
                capture("theme-pdf");
                check(backend.setExportPaperSize("letter") && backend.setExportOrientation("landscape"),
                      "Cannot change themed PDF to Letter landscape");
                if (!check(themeReady("pdf"), "Letter landscape PDF did not settle")) return;
                QPdfDocument letter;
                const bool loaded = letter.load(pane->property("outputUrl").toUrl().toLocalFile()) == QPdfDocument::Error::None;
                const QSizeF landscape = loaded ? letter.pagePointSize(0) : QSizeF();
                check(loaded && qAbs(landscape.width() - 792) < 2 && qAbs(landscape.height() - 612) < 2,
                      "Themed PDF ignored a paper/orientation change");
                check(backend.setExportPaperSize("a4") && backend.setExportOrientation("portrait"),
                      "Cannot restore A4 portrait after the theme check");
                check(themeReady("pdf"), "Restored A4 PDF did not settle");
            });
            themeCheck("web-pdf-editor-layout-source-and-undo-preservation", [&] {
                if (!chooseFormat("web")) return;
                action("sourceModeButton", 1, false);
                editor->setProperty("cursorPosition", themeSample.size());
                editor->forceActiveFocus();
                // TextArea treats a leading newline in Key_unknown as a control
                // key. Start this synthetic text event with a printable character.
                const QString addition = "Synthetic unsaved note.\n";
                key(window, Qt::Key_unknown, addition);
                const QString draft = themeSample + addition;
                if (!check(waitUntil([&] { return editor->property("text").toString() == draft; }),
                           "Cannot create one native source edit for theme Undo verification")) return;
                const int revision = backend.documentRevision();
                const auto baseline = currentBounds();
                for (bool live : {false, true}) {
                    action(live ? "liveEditToggle" : "sourceModeButton", 1, live);
                    for (const QString &format : {QString("pdf"), QString("web"), QString("pdf"), QString("web")}) {
                        if (!chooseFormat(format)) return;
                        check(layout->property("effectiveLayoutMode").toInt() == 1 && layout->property("liveEditEnabled").toBool() == live
                            && editor->property("text").toString() == draft && backend.documentRevision() == revision && backend.modified()
                            && backend.publishingThemeId() == "claude-like", "Web/PDF switching changed chosen editor, layout, theme or source");
                        for (const QString &name : stationaryNames)
                            check(bounds(buttons.value(name)) == baseline.value(name), "Web/PDF switching moved layout control: " + name);
                    }
                    action("singleModeButton", 0, live);
                    check(!pane->isVisible() && layout->property("liveEditEnabled").toBool() == live,
                          "Single layout lost the chosen editor while using a publishing theme");
                    action("previewSplitButton", 1, live);
                }
                action("sourceModeButton", 1, false);
                editor->forceActiveFocus();
                QElapsedTimer undoTimer;
                undoTimer.start();
                QMetaObject::invokeMethod(window, "undoEditing");
                const bool immediateExact = editor->property("text").toString() == themeSample;
                const bool immediateModified = backend.modified();
                // Backend keeps its existing conservative dirty indicator after
                // Undo to saved bytes; this cycle verifies exact source/history.
                const bool undoSettled = waitUntil([&] {
                    return editor->property("text").toString() == themeSample;
                });
                report.insert("publishingThemeUndo", QJsonObject{{"passed", undoSettled},
                    {"immediateSourceExact", immediateExact}, {"immediateModified", immediateModified},
                    {"undoSourceMatches", editor->property("text").toString() == themeSample}, {"dirtyAfterUndo", backend.modified()},
                    {"elapsedMs", int(undoTimer.elapsed())}, {"sourceSha256", sha256(editor->property("text").toString().toUtf8())},
                    {"expectedSha256", sha256(themeSample.toUtf8())}, {"savedSha256", fileHash(sourcePath)}});
                check(undoSettled, "Publishing theme/format/layout changes interrupted the source edit's single Undo");
                QMetaObject::invokeMethod(window, "redoEditing");
                check(editor->property("text").toString() == draft, "Publishing presentation changes interrupted exact source Redo");
                QMetaObject::invokeMethod(window, "undoEditing");
                check(editor->property("text").toString() == themeSample && fileHash(sourcePath) == savedHash,
                      "The theme acceptance exercise changed the original synthetic UTF-8 file");
            });
            themeCheck("missing-images-keep-web-and-pdf-readable", [&] {
                expectedMissingPreviewImage = backend.documentBaseUrl().resolved(QUrl("missing-cycle127.png")).toString();
                const QString suffix = "\n\n![Unavailable sample](missing-cycle127.png)\n\nText after the unavailable image.\n";
                check(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, themeSample.size()), Q_ARG(QString, suffix)),
                      "Cannot append the missing-image fixture");
                if (!chooseFormat("web")) return;
                const auto dom = publishingDomResult(web, "document.querySelector('#write').textContent").toString();
                check(dom.contains("Synthetic NPS review") && dom.contains("Text after the unavailable image.")
                    && dom.contains("Unavailable sample") && !pane->property("warningText").toString().isEmpty(),
                    "Missing image erased Web content or omitted its notice");
                capture("missing-image-web");
                if (!chooseFormat("pdf")) return;
                QPdfDocument document;
                check(document.load(pane->property("outputUrl").toUrl().toLocalFile()) == QPdfDocument::Error::None,
                      "Missing image made PDF unreadable");
                QString text;
                for (int i = 0; i < document.pageCount(); ++i) text += document.getAllText(i).text();
                check(text.contains("Synthetic NPS review") && text.contains("Text after the unavailable image.")
                    && text.contains("Unavailable sample"), "Missing image erased the surrounding PDF text");
                QMetaObject::invokeMethod(window, "undoEditing");
                check(editor->property("text").toString() == themeSample && fileHash(sourcePath) == savedHash,
                      "Missing-image preview changed the source or Undo");
                check(themeReady("pdf"), "Preview did not settle after removing the unavailable image");
                expectedMissingPreviewImage.clear();
            });
            themeCheck("unchanged-refresh-is-stable-and-edit-displays-current-output", [&] {
                for (const QString &format : {QString("web"), QString("pdf")}) {
                    if (!chooseFormat(format)) return;
                    const QUrl stable = pane->property("outputUrl").toUrl();
                    for (int i = 0; i < 3; ++i) {
                        QMetaObject::invokeMethod(pane, "reload");
                        if (!check(themeReady(format), "Repeated refresh did not settle")) return;
                        check(pane->property("outputUrl").toUrl() == stable, "Unchanged preview navigated to another output URL");
                    }
                    const QString suffix = "\n\nLatest displayed preview revision.\n";
                    check(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, themeSample.size()), Q_ARG(QString, suffix)),
                          "Cannot edit preview stability fixture");
                    const bool settled = waitUntil([&] {
                        return pane->property("outputUrl").toUrl() != stable && ready()
                            && !pane->property("previewRefreshInFlight").toBool();
                    }, 30000);
                    if (!check(settled, "Latest " + format + " edit did not replace the output")) return;
                    if (format == "pdf") {
                        check(pane->property("actualPdfSource").toUrl() == pane->property("outputUrl").toUrl(),
                              "Latest PDF output differs from the pane's PdfDocument source");
                        QPdfDocument document;
                        check(document.load(pane->property("actualPdfSource").toUrl().toLocalFile()) == QPdfDocument::Error::None
                            && document.pageCount() > 0 && document.getAllText(document.pageCount() - 1).text().contains("Latest displayed preview revision."),
                            "Latest PDF edit is missing from the displayed document");
                    } else check(publishingDomResult(web, "document.querySelector('#write').textContent").toString()
                        .contains("Latest displayed preview revision."), "Latest Web edit is missing from the actual DOM");
                    check(pane->property("documentIdentity").toString() == backend.publishingDocumentIdentity()
                        && pane->property("displayedDocumentIdentity").toString() == backend.publishingDocumentIdentity()
                        && backend.fileUrl() == QUrl::fromLocalFile(sourcePath),
                          "Edited preview belongs to another selected document");
                    const QUrl latest = pane->property("outputUrl").toUrl();
                    settle(500);
                    check(pane->property("outputUrl").toUrl() == latest && !pane->property("previewRefreshInFlight").toBool(),
                          "Preview restarted rendering while idle");
                    QMetaObject::invokeMethod(window, "undoEditing");
                    if (!check(themeReady(format) && editor->property("text").toString() == themeSample,
                               "Preview stability exercise changed source Undo")) return;
                }
            });
            #include "publishingfolderacceptancecheck.inc"
            #include "publishingpreviewacceptancecheck.inc"
            backend.setOutputStyle(0);
            settings->setProperty("publishingFormat", "web");
            check(themeReady("web"), "Basic Web output did not settle after the synthetic theme checks");
        }
    }
