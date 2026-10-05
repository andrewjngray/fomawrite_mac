            // Exercise the exposed Cocoa surfaces with saved, disposable documents.
            // No grabWindow(), requestUpdate() or renderBarrier() occurs while
            // waiting: those calls can hide the missing natural repaint under test.
            {
                const QStringList titles{QStringLiteral("Hello Stranger — synthetic document A"),
                    QStringLiteral("NPS review — synthetic document B"),
                    QStringLiteral("Final selection — synthetic document C")};
                const QList<QColor> markerColors{QColor(205, 40, 65), QColor(30, 145, 70), QColor(35, 85, 215)};
                QList<QUrl> documents;
                QStringList contents;
                QJsonArray observations;
                const bool savedSynchronizedScroll = settings->property("synchronizedScroll").toBool();
                bool fixturesReady = true;
                for (int index = 0; index < titles.size(); ++index) {
                    const QString imageName = QString("preview-marker-%1.png").arg(index);
                    QImage marker(420, 100, QImage::Format_RGB32);
                    marker.fill(markerColors.at(index));
                    fixturesReady = check(marker.save(temporary.filePath(imageName)), "Cannot save synthetic preview marker") && fixturesReady;
                    const QString text = "# " + titles.at(index) + "\n\n![Synthetic document marker](" + imageName
                        + ")\n\nOnly this selected document may appear in the publishing pane.\n";
                    const QString path = temporary.filePath(QString("Preview document %1.md").arg(index));
                    QSaveFile file(path);
                    fixturesReady = check(file.open(QIODevice::WriteOnly) && file.write(text.toUtf8()) == text.toUtf8().size()
                        && file.commit(), "Cannot save synthetic preview document") && fixturesReady;
                    documents.append(QUrl::fromLocalFile(path));
                    contents.append(text);
                }
                const auto belongsToSelection = [&](QQuickItem *surface, int index) {
                    const QString displayed = surface->property("displayedDocumentIdentity").toString();
                    return backend.fileUrl() == documents.at(index)
                        && surface->property("documentIdentity").toString() == backend.publishingDocumentIdentity()
                        && (displayed.isEmpty() || displayed == backend.publishingDocumentIdentity());
                };
                const auto selectDocument = [&](int index) {
                    if (!check(backend.open(documents.at(index)), "Cannot select synthetic preview document " + QString::number(index))) return false;
                    // Cross-document invalidation is synchronous: any retained
                    // texture must disappear before new output is requested.
                    check(!pane->property("holdingPreviousFrame").toBool() && belongsToSelection(pane, index),
                          "Previous document remains labelled or overlaid after selection changed");
                    return check(waitUntil([&] { return editor->property("text").toString() == contents.at(index); }),
                                 "Selected synthetic document did not reach its source editor");
                };
                // Loader/Repeater delegates need not share the root's QObject
                // ownership tree. Their visual ancestry is authoritative here.
                std::function<QQuickItem *(QQuickItem *, const QString &)> visualDescendant;
                visualDescendant = [&](QQuickItem *parentItem, const QString &name) -> QQuickItem * {
                    if (parentItem->objectName() == name) return parentItem;
                    for (auto *item : parentItem->childItems())
                        if (auto *found = visualDescendant(item, name)) return found;
                    return nullptr;
                };
                const auto readinessState = [&](QQuickWindow *host, QQuickItem *surface, int index, const QString &format) {
                    auto *actualWeb = visualDescendant(surface, QStringLiteral("renderedPreview"));
                    return QJsonObject{{"expectedFormat", format}, {"actualFormat", surface->property("publishingMode").toString()},
                        {"expectedDocumentUrl", documents.at(index).toString()}, {"selectedDocumentUrl", backend.fileUrl().toString()},
                        {"selectedDocumentIdentity", backend.publishingDocumentIdentity()},
                        {"documentIdentity", surface->property("documentIdentity").toString()},
                        {"appliedDocumentIdentity", surface->property("appliedDocumentIdentity").toString()},
                        {"displayedDocumentIdentity", surface->property("displayedDocumentIdentity").toString()},
                        {"refreshRevision", surface->property("refreshRevision").toInt()},
                        {"appliedRevision", surface->property("appliedRevision").toInt()},
                        {"requestToken", surface->property("requestToken").toString()},
                        {"pendingRequest", surface->property("pendingRequest").toDouble()},
                        {"previewRefreshInFlight", surface->property("previewRefreshInFlight").toBool()},
                        {"viewportRefreshPending", surface->property("viewportRefreshPending").toBool()},
                        {"webViewportRequests", surface->property("webViewportRequests").toInt()},
                        {"webCommandEpoch", surface->property("webCommandEpoch").toInt()},
                        {"outputUrl", surface->property("outputUrl").toUrl().toString()},
                        {"webOutputUrl", surface->property("webOutputUrl").toUrl().toString()},
                        {"actualWebUrl", actualWeb ? actualWeb->property("url").toUrl().toString() : QString()},
                        {"webLoading", actualWeb && actualWeb->property("loading").toBool()},
                        {"webVisible", actualWeb && actualWeb->isVisible()},
                        {"actualPdfSource", surface->property("actualPdfSource").toUrl().toString()},
                        {"pdfPagesReady", surface->property("pdfPagesReady").toBool()},
                        {"pdfVisiblePagesPainted", surface->property("pdfVisiblePagesPainted").toBool()},
                        {"errorText", surface->property("errorText").toString()},
                        {"warningText", surface->property("warningText").toString()},
                        {"windowVisible", host->isVisible()}, {"windowExposed", host->isExposed()}, {"paneVisible", surface->isVisible()},
                        {"layoutMode", layout->property("layoutMode").toInt()}, {"effectiveLayoutMode", layout->property("effectiveLayoutMode").toInt()},
                        {"outputStyle", backend.outputStyle()}, {"publishingThemeId", backend.publishingThemeId()}};
                };
                const auto naturallyReady = [&](QQuickWindow *host, QQuickItem *surface, int index, const QString &format) {
                    return waitUntil([&] {
                        const QUrl outputUrl = surface->property("outputUrl").toUrl();
                        return host->isExposed() && surface->isVisible()
                            && !surface->property("viewportRefreshPending").toBool()
                            && !surface->property("previewRefreshInFlight").toBool()
                            && surface->property("errorText").toString().isEmpty()
                            && surface->property("publishingMode").toString() == format
                            && backend.fileUrl() == documents.at(index)
                            && surface->property("documentIdentity").toString() == backend.publishingDocumentIdentity()
                            && surface->property("displayedDocumentIdentity").toString() == backend.publishingDocumentIdentity()
                            && !outputUrl.isEmpty() && QFileInfo(outputUrl.toLocalFile()).suffix() == (format == "pdf" ? "pdf" : "html")
                            && (format != "pdf" || (surface->property("actualPdfSource").toUrl() == outputUrl
                                && surface->property("pdfPagesReady").toBool()
                                && surface->property("pdfVisiblePagesPainted").toBool()));
                    }, 30000);
                };
                const auto verifyActualSurface = [&](QQuickWindow *host, QQuickItem *surface, int index,
                                                      const QString &format, const QString &label) {
                    // Cocoa marks this nonactivating QA window unexposed when
                    // another app covers it. Raise before each observation so
                    // the normal compositor can paint; never force a frame.
                    host->raise();
                    int naturalFrames = 0;
                    QObject frameReceiver;
                    const QMetaObject::Connection frameObserver = QObject::connect(host, &QQuickWindow::frameSwapped, &frameReceiver,
                        [&] { ++naturalFrames; }, Qt::QueuedConnection);
                    QElapsedTimer readinessTimer;
                    readinessTimer.start();
                    const bool readyNaturally = naturallyReady(host, surface, index, format);
                    if (!check(readyNaturally, "Preview did not become naturally ready: " + label)) {
                        QObject::disconnect(frameObserver);
                        const QJsonObject state = readinessState(host, surface, index, format);
                        observations.append(QJsonObject{{"scenario", label}, {"format", format}, {"naturallyReady", false},
                            {"elapsedMs", int(readinessTimer.elapsed())}, {"naturalFramesBeforeTimeout", naturalFrames}, {"state", state}});
                        qWarning().noquote() << "Preview readiness timeout:" << label << QJsonDocument(state).toJson(QJsonDocument::Compact);
                        return false;
                    }
                    // The status/DOM event is earlier than Chromium's compositor.
                    // This fixed event-loop settle contains no forced rendering.
                    settle(350);
                    QObject::disconnect(frameObserver);
                    check(naturalFrames > 0, "No natural Cocoa frame was presented before the preview capture: " + label);
                    const QUrl outputUrl = surface->property("outputUrl").toUrl();
                    const QUrl actualPdfSource = surface->property("actualPdfSource").toUrl();
                    const QString displayed = surface->property("displayedDocumentIdentity").toString();
                    QPdfDocument reference;
                    QQuickItem *paper = nullptr;
                    QImage referenceRaster;
                    QString actualTitle;
                    bool contentCorrect = false;
                    if (format == "pdf") {
                        const bool loaded = reference.load(actualPdfSource.toLocalFile()) == QPdfDocument::Error::None;
                        actualTitle = loaded && reference.pageCount() > 0 ? reference.getAllText(0).text() : QString();
                        paper = visualDescendant(surface, QStringLiteral("publishingPdfPage_0"));
                        contentCorrect = loaded && actualPdfSource == outputUrl && actualTitle.contains(titles.at(index))
                            && paper && paper->isVisible();
                    } else {
                        auto *actualWeb = surface->findChild<QQuickItem *>("renderedPreview");
                        actualTitle = actualWeb ? publishingDomResult(actualWeb, "document.querySelector('#write > h1')?.textContent.trim()").toString() : QString();
                        contentCorrect = actualWeb && actualWeb->isVisible() && actualTitle == titles.at(index)
                            && actualWeb->property("url").toUrl() == outputUrl;
                    }
                    check(contentCorrect && displayed == backend.publishingDocumentIdentity() && backend.fileUrl() == documents.at(index), "Actual preview content/source/identity differs from selection: " + label);
                    // One capture only, after the surface has naturally settled.
                    const QImage frame = host->grabWindow();
                    const QString screenshot = output.filePath(label + ".png");
                    if (check(!frame.isNull() && frame.save(screenshot), "Cannot capture settled synthetic preview: " + label)) screenshots.append(screenshot);
                    const auto matchesMarker = [&](const QColor &color) {
                        const QColor expected = markerColors.at(index);
                        return qAbs(color.red() - expected.red()) <= 22 && qAbs(color.green() - expected.green()) <= 22
                            && qAbs(color.blue() - expected.blue()) <= 22;
                    };
                    int actualMarkerSamples = 0, referenceMarkerSamples = 0, overlappingMarkerSamples = 0;
                    if (!frame.isNull()) {
                        const double sx = double(frame.width()) / host->width(), sy = double(frame.height()) / host->height();
                        const QRectF pageArea = paper ? bounds(paper) : bounds(surface);
                        // Exclude the footer and clip the physical capture to the
                        // pane's reading viewport, not the rest of the app window.
                        const QRectF viewport = bounds(surface).adjusted(0, 0, 0, -surface->property("bottomInset").toDouble());
                        const QRectF visible = pageArea.intersected(viewport).intersected(QRectF(QPointF(), host->size()));
                        if (paper && contentCorrect) referenceRaster = reference.render(0,
                            QSize(qRound(pageArea.width() * sx), qRound(pageArea.height() * sy)));
                        for (int y = qMax(0, qRound(visible.top() * sy) + 1); y < qMin(frame.height(), qRound(visible.bottom() * sy) - 1); y += 4) {
                            for (int x = qMax(0, qRound(visible.left() * sx) + 1); x < qMin(frame.width(), qRound(visible.right() * sx) - 1); x += 4) {
                                const bool actualMarker = matchesMarker(frame.pixelColor(x, y));
                                if (actualMarker) ++actualMarkerSamples;
                                if (!referenceRaster.isNull()) {
                                    const int rx = qBound(0, qRound(x - pageArea.left() * sx), referenceRaster.width() - 1);
                                    const int ry = qBound(0, qRound(y - pageArea.top() * sy), referenceRaster.height() - 1);
                                    if (matchesMarker(referenceRaster.pixelColor(rx, ry))) {
                                        ++referenceMarkerSamples;
                                        if (actualMarker) ++overlappingMarkerSamples;
                                    }
                                }
                            }
                        }
                    }
                    const bool painted = actualMarkerSamples > 40 && (format != "pdf" ||
                        (referenceMarkerSamples > 40 && overlappingMarkerSamples >= referenceMarkerSamples * 0.65));
                    check(painted, "Actual settled " + format + " pane is blank or displays a previous document marker: " + label);
                    observations.append(QJsonObject{{"scenario", label}, {"format", format}, {"selectedDocumentIdentity", backend.publishingDocumentIdentity()}, {"selectedDocumentUrl", documents.at(index).toString()},
                        {"displayedDocumentIdentity", displayed}, {"outputUrl", outputUrl.toString()}, {"actualPdfSource", actualPdfSource.toString()},
                        {"expectedHeading", titles.at(index)}, {"contentCorrect", contentCorrect}, {"actualMarkerSamples", actualMarkerSamples},
                        {"referenceMarkerSamples", referenceMarkerSamples}, {"overlappingMarkerSamples", overlappingMarkerSamples},
                        {"actualPanePainted", painted}, {"naturalFramesBeforeCapture", naturalFrames}, {"capturePolicy", "One capture after natural readiness and a fixed 350 ms event-loop settle"}});
                    return contentCorrect && painted && naturalFrames > 0 && displayed == backend.publishingDocumentIdentity() && backend.fileUrl() == documents.at(index);
                };
                if (fixturesReady) {
                    window->raise();
                    settings->setProperty("synchronizedScroll", false);
                    layout->setProperty("layoutMode", 1);
                    layout->setProperty("visualEditEnabled", false);
                    backend.selectPublishingTheme("claude-like");
                    themeCheck("natural-cocoa-pdf-document-a-to-nps-document-b", [&] {
                        settings->setProperty("publishingFormat", "pdf");
                        if (!selectDocument(0) || !verifyActualSurface(window, pane, 0, "pdf", "preview-pdf-document-a")) return;
                        if (selectDocument(1)) verifyActualSurface(window, pane, 1, "pdf", "preview-pdf-document-b");
                    });
                    themeCheck("natural-cocoa-rapid-a-b-c-selection", [&] {
                        settings->setProperty("publishingFormat", "pdf");
                        // Use a fresh output layout so A starts an uncached PDF
                        // request before B/C selection supersedes it.
                        backend.setExportOrientation("landscape");
                        bool observedPendingRequest = false;
                        for (int index : {0, 1, 2}) {
                            if (!check(backend.open(documents.at(index)), "Cannot rapidly select synthetic preview document")) return;
                            check(!pane->property("holdingPreviousFrame").toBool() && belongsToSelection(pane, index),
                                  "Rapid selection retained a previous document overlay/identity");
                            if (index == 0) {
                                observedPendingRequest = waitUntil([&] { return pane->property("pendingRequest").toDouble() >= 0; }, 5000);
                                check(observedPendingRequest, "Rapid selection fixture did not observe an in-flight PDF request");
                            }
                        }
                        observations.append(QJsonObject{{"scenario", "rapid-selection-in-flight-request"}, {"observedPendingRequest", observedPendingRequest}});
                        if (!verifyActualSurface(window, pane, 2, "pdf", "preview-pdf-rapid-final-c")) return;
                        const QUrl finalOutput = pane->property("outputUrl").toUrl();
                        settle(700);
                        check(pane->property("documentIdentity").toString() == backend.publishingDocumentIdentity()
                            && pane->property("displayedDocumentIdentity").toString() == backend.publishingDocumentIdentity()
                            && backend.fileUrl() == documents.at(2)
                            && pane->property("actualPdfSource").toUrl() == finalOutput && pane->property("outputUrl").toUrl() == finalOutput
                            && !pane->property("previewRefreshInFlight").toBool(), "A stale rapid-selection callback replaced the final document");
                    });
                    themeCheck("natural-cocoa-hidden-show-theme-and-web-pdf-modes", [&] {
                        backend.setExportOrientation("portrait");
                        layout->setProperty("layoutMode", 0);
                        if (!check(waitUntil([&] { return !pane->isVisible(); }), "Cannot hide the publishing pane")) return;
                        if (!selectDocument(1)) return;
                        backend.setOutputStyle(0);
                        settings->setProperty("publishingFormat", "web");
                        layout->setProperty("layoutMode", 1);
                        if (!verifyActualSurface(window, pane, 1, "web", "preview-web-after-hidden-switch")) return;
                        backend.selectPublishingTheme("claude-like");
                        settings->setProperty("publishingFormat", "pdf");
                        if (!verifyActualSurface(window, pane, 1, "pdf", "preview-pdf-after-theme-mode-switch")) return;
                        window->hide();
                        settle(100);
                        if (!selectDocument(2)) { window->show(); return; }
                        window->show();
                        window->raise();
                        if (!verifyActualSurface(window, pane, 2, "pdf", "preview-pdf-after-window-show")) return;
                        settings->setProperty("publishingFormat", "web");
                        verifyActualSurface(window, pane, 2, "web", "preview-web-after-pdf-return");
                    });
                    themeCheck("cold-pdf-preview-component-cached-output", [&] {
                        // A new PublishingPreview has never assigned PdfDocument.source.
                        // It receives the cached PDF for the selected saved document.
                        // This tests component startup; it does not claim app relaunch.
                        layout->setProperty("layoutMode", 0);
                        if (!selectDocument(1)) return;
                        QQmlApplicationEngine coldEngine;
                        coldEngine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
                        QObject::connect(&coldEngine, &QQmlEngine::warnings, &coldEngine, [&](const QList<QQmlError> &warnings) {
                            for (const auto &warning : warnings) qmlWarnings.append(warning.toString());
                        });
                        coldEngine.setInitialProperties({{"renderer", QVariant::fromValue(&backend)}, {"markdown", contents.at(1)},
                            {"documentBaseUrl", backend.documentBaseUrl()}, {"documentIdentity", backend.publishingDocumentIdentity()}, {"publishingMode", "pdf"}});
                        coldEngine.load(QUrl(QStringLiteral("qrc:/PublishingPreview.qml")));
                        auto *coldSurface = coldEngine.rootObjects().isEmpty() ? nullptr
                            : qobject_cast<QQuickItem *>(coldEngine.rootObjects().constFirst());
                        if (!check(coldSurface, "Cannot create a fresh PDF preview component")) return;
                        QQuickWindow coldWindow;
                        coldWindow.setFlag(Qt::WindowDoesNotAcceptFocus, true);
                        InputIsolation coldInput(app, &coldWindow);
                        coldWindow.resize(760, 900);
                        coldSurface->setParentItem(coldWindow.contentItem());
                        coldSurface->setWidth(coldWindow.width());
                        coldSurface->setHeight(coldWindow.height());
                        coldWindow.show();
                        coldWindow.raise();
                        verifyActualSurface(&coldWindow, coldSurface, 1, "pdf", "preview-cold-pdf-component");
                        coldWindow.hide();
                    });
                    // Restore the fixture used by the surrounding theme checks.
                    backend.open(QUrl::fromLocalFile(sourcePath));
                    settings->setProperty("synchronizedScroll", savedSynchronizedScroll);
                    layout->setProperty("layoutMode", 1);
                    window->show();
                    window->raise();
                }
                report.insert("publishingPreviewCorrectness", QJsonObject{{"platform", QGuiApplication::platformName()},
                    {"input", "Synthetic saved-document selection and property changes; physical input not tested"},
                    {"observations", observations}});
            }
