// Review cycle R2 — performance profile of the hot paths flagged by
// INDEPENDENT_REVIEW.md: A2 (theme-heavy preview cost), A3 (PDF render and
// profile reuse), P1 (mapper scaling) and image decode caching.
//
// These slots are measurements: every number is printed with qInfo and the
// only assertions are generous sanity bounds so the slots stay green in CI on
// slower machines. Read the figures, not the pass/fail. The recorded run is in
// research/perf/README.md.
//
// Depends on the anonymous-namespace helpers of cycle132-publishing.inc
// (Cycle132Editor, cycle132Write, cycle132Read, cycle132Ready); this file is
// included after it in tst_fomawrite.cpp.

#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace {
double perfMs(const QElapsedTimer &timer) { return timer.nsecsElapsed() / 1.0e6; }

// Raw Chromium phases, independent of PublishingPdf: load a file URL in a page
// and wait for loadFinished; print the loaded page to PDF bytes.
double perfLoadInPage(QWebEnginePage &page, const QUrl &url, bool *ok) {
    QEventLoop loop;
    bool loaded = false;
    QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool success) { loaded = success; loop.quit(); });
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);
    QElapsedTimer timer; timer.start();
    page.load(url);
    loop.exec();
    if (ok) *ok = loaded;
    return perfMs(timer);
}
double perfPrintPage(QWebEnginePage &page, const QPageLayout &layout, qint64 *bytes) {
    QEventLoop loop;
    QByteArray pdf;
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);
    QElapsedTimer timer; timer.start();
    page.printToPdf([&](const QByteArray &result) { pdf = result; loop.quit(); }, layout);
    loop.exec();
    if (bytes) *bytes = pdf.size();
    return perfMs(timer);
}

// Plain prose without Markdown syntax characters, so every line is one
// editable paragraph and the mapper's inline parser takes the plain-text path.
QString perfProse(int words, int seed = 0) {
    static const QStringList lexicon{"lorem", "ipsum", "dolor", "sit", "amet", "consectetur", "adipiscing", "elit",
        "sed", "do", "eiusmod", "tempor", "incididunt", "ut", "labore", "et", "dolore", "magna", "aliqua", "enim",
        "ad", "minim", "veniam", "quis", "nostrud", "exercitation", "ullamco", "laboris", "nisi", "aliquip", "ex",
        "ea", "commodo", "consequat", "duis", "aute", "irure", "in", "reprehenderit", "voluptate", "velit", "esse"};
    QString text;
    for (int i = 0; i < words; ++i) {
        if (i) text += (i % 11 == 10) ? QStringLiteral(", ") : QStringLiteral(" ");
        text += lexicon.at((i * 7 + seed) % lexicon.size());
    }
    return text + QLatin1Char('.');
}

QString perfSingleLine(int bytes) {
    QString line = perfProse(bytes / 5 + 8);
    line.truncate(bytes);
    return line;
}

// A document of the given approximate size made of ordinary paragraphs,
// headings and list items (about 80 UTF-16 units per line).
QString perfDocument(int bytes) {
    QString text;
    int block = 0;
    while (text.size() < bytes) {
        if (block % 7 == 0) text += "## Section " + QString::number(block / 7 + 1) + "\n\n";
        else if (block % 7 == 5) text += "- " + perfProse(10, block) + "\n- " + perfProse(9, block + 1) + "\n\n";
        else text += perfProse(70, block) + "\n\n";
        ++block;
    }
    return text;
}

// Ten explicit pages (pagebreaks) each holding roughly a page of prose.
QString perfTenPageDocument() {
    QString text = "# Performance profile fixture\n\n";
    for (int page = 0; page < 10; ++page) {
        if (page) text += "<!-- pagebreak -->\n\n";
        text += "## Page " + QString::number(page + 1) + "\n\n";
        for (int paragraph = 0; paragraph < 5; ++paragraph) text += perfProse(75, page * 5 + paragraph) + "\n\n";
        text += "- " + perfProse(8, page) + "\n- " + perfProse(9, page + 1) + "\n\n";
    }
    return text;
}

// Waits on the signal itself (no polling slices) so the figure is the
// renderer's latency, not the test harness's.
double perfWaitReady(Backend &backend, QSignalSpy &ready, quint64 id, QVariantMap *result, QElapsedTimer &timer, int timeoutMs = 35000) {
    if (cycle132Ready(ready, id, result)) return perfMs(timer);
    QEventLoop loop;
    QObject::connect(&backend, &Backend::publishingPreviewReady, &loop, [&](quint64 readyId, const QVariantMap &) {
        if (readyId == id) loop.quit();
    });
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
    return cycle132Ready(ready, id, result) ? perfMs(timer) : -1;
}
}

// A2 — theme-heavy preview: what a request costs with a 16 MiB font embedded
// in the selected theme, on the memoized path (edit) and on a cache hit.
void FomawriteTest::perfThemeHeavyPreviewCost() {
    Cycle103SettingsSnapshot settings;
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    const QString id = "perf-heavy";
    const QString theme = folder + '/' + id + ".css";
    const QString assets = folder + '/' + id + "-assets";
    const QString font = assets + "/heavy.woff";
    const auto cleanup = qScopeGuard([&] { QFile::remove(theme); QDir(assets).removeRecursively(); });
    QVERIFY(QDir().mkpath(assets));
    // The sanitizer embeds fonts by suffix, so a synthetic 16 MiB file costs
    // the same base64 and lexer work as a real one.
    QVERIFY(cycle132Write(font, QByteArray(16 * 1024 * 1024, 'A')));
    QVERIFY(cycle132Write(theme, "@font-face{font-family:Heavy;src:url(perf-heavy-assets/heavy.woff)} #write{font-family:Heavy}"));
    Cycle132Editor fixture; QVERIFY(fixture.create());
    const QString source = perfDocument(6 * 1024);
    fixture.editor->setProperty("text", source);
    QTest::qWait(400);
    QObject consumer;
    QElapsedTimer timer;

    // Baseline: the basic style (no folder theme) on the same document.
    timer.start();
    const auto basic = fixture.backend.requestPublishingPreview("html", &consumer, "p:0");
    const double basicMs = perfMs(timer);
    QVERIFY2(basic["ok"].toBool(), qPrintable(basic["error"].toString()));
    const qint64 basicBytes = QFileInfo(basic["url"].toUrl().toLocalFile()).size();

    timer.start();
    QVERIFY2(fixture.backend.selectPublishingTheme(id), qPrintable(fixture.backend.status()));
    const double selectMs = perfMs(timer);

    // (a) first request with the theme selected.
    timer.start();
    const auto first = fixture.backend.requestPublishingPreview("html", &consumer, "p:1");
    const double firstMs = perfMs(timer);
    QVERIFY2(first["ok"].toBool(), qPrintable(first["error"].toString()));
    const qint64 themedBytes = QFileInfo(first["url"].toUrl().toLocalFile()).size();
    QVERIFY(themedBytes > 16 * 1024 * 1024);

    // (b) one-character source edit: memoized CSS, new body, new output file.
    fixture.editor->setProperty("text", source + "x");
    timer.start();
    const auto edited = fixture.backend.requestPublishingPreview("html", &consumer, "p:2");
    const double editMs = perfMs(timer);
    QVERIFY2(edited["ok"].toBool(), qPrintable(edited["error"].toString()));
    QVERIFY(edited["url"] != first["url"]);

    // (c) no change: the cache hit. publishingHtml() still assembles the full
    // document before the key is compared, so this is the assembly floor.
    timer.start();
    const auto hit = fixture.backend.requestPublishingPreview("html", &consumer, "p:3");
    const double hitMs = perfMs(timer);
    QVERIFY2(hit["ok"].toBool(), qPrintable(hit["error"].toString()));
    QCOMPARE(hit["url"], edited["url"]);
    timer.start();
    const auto hitAgain = fixture.backend.requestPublishingPreview("html", &consumer, "p:4");
    const double hitAgainMs = perfMs(timer);
    QCOMPARE(hitAgain["url"], edited["url"]);

    // What every request paid before Cycle 132: the full sanitizer pass.
    timer.start();
    QString error;
    const auto css = PublishingThemes::css(id, &error);
    const double cssMs = perfMs(timer);
    QVERIFY2(css.has_value(), qPrintable(error));

    // The parts of the per-request assembly that scale with the CSS size.
    timer.start();
    const QByteArray cssUtf8 = css->toUtf8();
    const double utf8Ms = perfMs(timer);
    timer.start();
    const QByteArray cssHash = QCryptographicHash::hash(cssUtf8, QCryptographicHash::Sha256);
    const double hashMs = perfMs(timer);
    QVERIFY(!cssHash.isEmpty());
    QString embedError, embedWarning;
    PublishingHtml::ImageCache imageCache;
    QByteArray assetSignature;
    const QString assembled = "<!doctype html><html><head><style>" + *css + "</style></head><body><article id=\"write\"><p>x</p></article></body></html>";
    timer.start();
    const QString scanned = PublishingHtml::embedImages(assembled, QUrl(), true, &embedError, &embedWarning, &imageCache, &assetSignature);
    const double embedMs = perfMs(timer);
    QCOMPARE(scanned.size(), assembled.size());

    // What the preview pane (and the PDF renderer) then pays to parse the
    // 22 MB page with its inline data: font, versus the basic page.
    QWebEngineProfile profile;
    QWebEnginePage page(&profile);
    bool loaded = false;
    const double basicLoadMs = perfLoadInPage(page, basic["url"].toUrl(), &loaded); QVERIFY(loaded);
    const double themedLoadMs = perfLoadInPage(page, first["url"].toUrl(), &loaded); QVERIFY(loaded);
    const double themedReloadMs = perfLoadInPage(page, edited["url"].toUrl(), &loaded); QVERIFY(loaded);

    qInfo("perf A2 theme-heavy preview (16 MiB .woff, %lld-char source):", static_cast<long long>(source.size()));
    qInfo("perf A2   basic style request (no theme)          %9.2f ms  (%lld bytes html)", basicMs, static_cast<long long>(basicBytes));
    qInfo("perf A2   selectPublishingTheme (2x css())        %9.2f ms", selectMs);
    qInfo("perf A2   (a) first request, theme selected       %9.2f ms  (%lld bytes html)", firstMs, static_cast<long long>(themedBytes));
    qInfo("perf A2   (b) request after 1-char edit (memo)    %9.2f ms", editMs);
    qInfo("perf A2   (c) request with no change (cache hit)  %9.2f ms", hitMs);
    qInfo("perf A2   (c') second cache hit                   %9.2f ms", hitAgainMs);
    qInfo("perf A2   PublishingThemes::css(id) once (pre-132 per-request cost) %9.2f ms  (%lld chars)", cssMs, static_cast<long long>(css->size()));
    qInfo("perf A2   css.toUtf8() %7.2f ms   sha256(css) %7.2f ms   embedImages scan of assembled html %7.2f ms", utf8Ms, hashMs, embedMs);
    qInfo("perf A2   Chromium page load: basic html %7.2f ms   themed 22 MB html %7.2f ms   themed again (edited) %7.2f ms", basicLoadMs, themedLoadMs, themedReloadMs);
    QVERIFY(basicMs < 15000); QVERIFY(selectMs < 15000); QVERIFY(firstMs < 15000);
    QVERIFY(editMs < 15000); QVERIFY(hitMs < 15000); QVERIFY(cssMs < 15000);
    fixture.backend.discardRecovery();
}

// A3 — PDF render cost: three sequential renders of a ten-page document on one
// consumer (shared profile, page per job), then a fresh consumer (new profile).
void FomawriteTest::perfPdfRenderProfileReuse() {
    Cycle103SettingsSnapshot settings;
    Cycle132Editor fixture; QVERIFY(fixture.create());
    QSignalSpy ready(&fixture.backend, &Backend::publishingPreviewReady);
    QObject consumer;
    QElapsedTimer timer;

    // Chromium cold start is paid by whichever render comes first in the
    // process. Isolate it with a one-line document.
    fixture.editor->setProperty("text", "# Warm-up\n\nOne paragraph.\n");
    QTest::qWait(400);
    timer.start();
    auto request = fixture.backend.requestPublishingPreview("pdf", &consumer, "warm:1");
    QVERIFY2(request["pending"].toBool(), qPrintable(request["error"].toString()));
    QVariantMap result;
    const double warmMs = perfWaitReady(fixture.backend, ready, request["requestId"].toULongLong(), &result, timer);
    QVERIFY2(warmMs >= 0 && result["ok"].toBool(), qPrintable(result["error"].toString()));

    const QString source = perfTenPageDocument();
    double renderMs[3] = {-1, -1, -1};
    int pages = 0;
    for (int round = 0; round < 3; ++round) {
        fixture.editor->setProperty("text", source + QString("\nRevision %1.\n").arg(round));
        timer.start();
        request = fixture.backend.requestPublishingPreview("pdf", &consumer, QString("pdf:%1").arg(round));
        QVERIFY2(request["pending"].toBool(), qPrintable(request["error"].toString()));
        renderMs[round] = perfWaitReady(fixture.backend, ready, request["requestId"].toULongLong(), &result, timer);
        QVERIFY2(renderMs[round] >= 0 && result["ok"].toBool(), qPrintable(result["error"].toString()));
        if (round == 0) {
            QPdfDocument document;
            QCOMPARE(document.load(result["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
            pages = document.pageCount();
        }
    }
    // Backend post-processing proxy: storePublishingOutput() loads the PDF
    // again and extracts every page's text for heading anchors.
    timer.start();
    {
        QPdfDocument document;
        QCOMPARE(document.load(result["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
        for (int page = 0; page < document.pageCount(); ++page) QVERIFY(!document.getAllText(page).text().isEmpty());
    }
    const double anchorsMs = perfMs(timer);
    // A second consumer gets its own PublishingPdf and therefore its own
    // off-the-record profile: the cost of the profile that Cycle 132 now shares.
    QObject freshConsumer;
    timer.start();
    request = fixture.backend.requestPublishingPreview("pdf", &freshConsumer, "fresh:1");
    QVERIFY2(request["pending"].toBool(), qPrintable(request["error"].toString()));
    const double freshMs = perfWaitReady(fixture.backend, ready, request["requestId"].toULongLong(), &result, timer);
    QVERIFY2(freshMs >= 0 && result["ok"].toBool(), qPrintable(result["error"].toString()));
    // And the synchronous export path: renderer, profile and page per call.
    fixture.editor->setProperty("text", source + "\nExport revision.\n");
    timer.start();
    const auto blocking = fixture.backend.publishingPreview("pdf");
    const double blockingMs = perfMs(timer);
    QVERIFY2(blocking["ok"].toBool(), qPrintable(blocking["error"].toString()));

    // Raw Chromium phases for the same document, to see what page reuse could
    // save: profile creation, page creation, load, print.
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QObject htmlConsumer;
    const auto stored = fixture.backend.requestPublishingPreview("html", &htmlConsumer, "raw:1");
    QVERIFY2(stored["ok"].toBool(), qPrintable(stored["error"].toString()));
    const QByteArray document = cycle132Read(stored["url"].toUrl()); QVERIFY(document.contains("Export revision"));
    const QString pathA = directory.filePath("a.html"), pathB = directory.filePath("b.html");
    QVERIFY(cycle132Write(pathA, document));
    QVERIFY(cycle132Write(pathB, QByteArray(document).replace("Export revision", "Reuse revision")));
    const QPageLayout layout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
    bool loaded = false; qint64 bytes = 0;
    timer.start();
    auto *profile = new QWebEngineProfile;
    profile->setHttpCacheType(QWebEngineProfile::NoCache);
    const double profileMs = perfMs(timer);
    timer.start();
    auto *page = new QWebEnginePage(profile);
    page->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    const double pageMs = perfMs(timer);
    const double load1Ms = perfLoadInPage(*page, QUrl::fromLocalFile(pathA), &loaded); QVERIFY(loaded);
    const double print1Ms = perfPrintPage(*page, layout, &bytes); QVERIFY(bytes > 0);
    const double load2Ms = perfLoadInPage(*page, QUrl::fromLocalFile(pathB), &loaded); QVERIFY(loaded);
    const double print2Ms = perfPrintPage(*page, layout, &bytes); QVERIFY(bytes > 0);
    const double print3Ms = perfPrintPage(*page, layout, &bytes); QVERIFY(bytes > 0);
    timer.start();
    auto *secondPage = new QWebEnginePage(profile);
    secondPage->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    const double secondPageMs = perfMs(timer);
    const double load3Ms = perfLoadInPage(*secondPage, QUrl::fromLocalFile(pathA), &loaded); QVERIFY(loaded);
    const double print4Ms = perfPrintPage(*secondPage, layout, &bytes); QVERIFY(bytes > 0);
    delete secondPage; delete page; delete profile;

    qInfo("perf A3 PDF render (%d pages, %lld-char source):", pages, static_cast<long long>(source.size()));
    qInfo("perf A3   warm-up one-paragraph render (includes Chromium start when first in process) %9.2f ms", warmMs);
    qInfo("perf A3   render 1 (same consumer: shared profile, new page)   %9.2f ms", renderMs[0]);
    qInfo("perf A3   render 2 (after source edit)                         %9.2f ms", renderMs[1]);
    qInfo("perf A3   render 3 (after source edit)                         %9.2f ms", renderMs[2]);
    qInfo("perf A3   render on a fresh consumer (new PublishingPdf + new profile) %9.2f ms", freshMs);
    qInfo("perf A3   QPdfDocument load + getAllText of the %d-page result (anchor extraction proxy) %9.2f ms", pages, anchorsMs);
    qInfo("perf A3   publishingPreview(\"pdf\") blocking export path        %9.2f ms", blockingMs);
    qInfo("perf A3 raw Chromium phases for the same document:");
    qInfo("perf A3   new QWebEngineProfile %6.2f ms   new QWebEnginePage %6.2f ms   second page on same profile %6.2f ms", profileMs, pageMs, secondPageMs);
    qInfo("perf A3   first page: load %6.2f ms + printToPdf %6.2f ms;  reload new content in same page: load %6.2f ms + print %6.2f ms;  print again without load %6.2f ms", load1Ms, print1Ms, load2Ms, print2Ms, print3Ms);
    qInfo("perf A3   second page: load %6.2f ms + print %6.2f ms", load3Ms, print4Ms);
    QVERIFY(pages >= 10);
    for (double ms : renderMs) QVERIFY(ms < 30000);
    QVERIFY(freshMs < 30000);
    fixture.backend.discardRecovery();
}

// P1 — mapper scaling: SourceVisualMapping::create on one long line at four
// sizes, one visual edit on a 10 KB line, and the full visual pipeline on a
// 1 MiB multi-line document.
void FomawriteTest::perfMapperScaling() {
    QElapsedTimer timer;
    const int sizes[] = {1024, 10 * 1024, 100 * 1024, 1024 * 1024};
    double createMs[4] = {0, 0, 0, 0};
    bool measured[4] = {false, false, false, false};
    for (int i = 0; i < 4; ++i) {
        if (i == 3) {
            // Only attempt the 1 MiB line when the measured growth predicts it
            // finishes in bounded time; otherwise report the extrapolation.
            const double exponent = std::log(createMs[2] / createMs[1]) / std::log(10.0);
            const double predicted = createMs[2] * std::pow(10.0, exponent);
            if (predicted > 12000) {
                qInfo("perf P1   create 1 MiB single line: skipped, extrapolated %9.0f ms from exponent %.2f", predicted, exponent);
                createMs[3] = predicted;
                break;
            }
        }
        const QString line = perfSingleLine(sizes[i]);
        timer.start();
        const auto mapping = SourceVisualMapping::create(line);
        createMs[i] = perfMs(timer);
        measured[i] = true;
        QCOMPARE(mapping.visualText(), line);
    }
    const double exponent10to100 = std::log(createMs[2] / createMs[1]) / std::log(10.0);
    const double exponent1to10 = std::log(createMs[1] / createMs[0]) / std::log(10.0);

    // One visual edit on a 10 KB line: the candidate reprojection runs create()
    // again, so a keystroke pays for two projections of the line.
    const QString tenKb = perfSingleLine(10 * 1024);
    const auto tenKbMapping = SourceVisualMapping::create(tenKb);
    timer.start();
    const auto edit = tenKbMapping.sourceEditForVisualReplacement({5000, 5}, QStringLiteral("hello"));
    const double editMs = perfMs(timer);
    QVERIFY(edit.has_value());

    // The same sizes as multi-line documents: the quadratic term is per line,
    // so ordinary documents stay linear.
    const QString hundredKb = perfDocument(100 * 1024);
    timer.start();
    const auto hundredKbMapping = SourceVisualMapping::create(hundredKb);
    const double createHundredKbMs = perfMs(timer);
    QVERIFY(hundredKbMapping.blocks().size() > 100);
    const QString oneMb = perfDocument(1024 * 1024);
    timer.start();
    const auto oneMbMapping = SourceVisualMapping::create(oneMb);
    const double createDocMs = perfMs(timer);
    QVERIFY(oneMbMapping.blocks().size() > 1000);

    Cycle132Editor fixture; QVERIFY(fixture.create());
    timer.start();
    fixture.editor->setProperty("text", oneMb);
    const double setTextMs = perfMs(timer);
    timer.start();
    const auto projection = fixture.backend.visualProjection();
    const double projectionMs = perfMs(timer);
    QCOMPARE(projection["source"].toString().size(), oneMb.size());
    // One Visual keystroke as the QML pane drives it: applyVisualEdit (create +
    // candidate create) followed by the reprojection it triggers.
    const int visualCursor = projection["visualText"].toString().indexOf("lorem") + 3;
    timer.start();
    const bool applied = fixture.backend.applyVisualEdit(visualCursor, 0, "z", oneMb);
    const double applyMs = perfMs(timer);
    QVERIFY(applied);
    timer.start();
    const auto reprojection = fixture.backend.visualProjection();
    const double reprojectionMs = perfMs(timer);
    QCOMPARE(reprojection["source"].toString().size(), oneMb.size() + 1);

    qInfo("perf P1 SourceVisualMapping::create, one line of prose:");
    for (int i = 0; i < 4; ++i)
        qInfo("perf P1   %7d bytes  %10.2f ms%s", sizes[i], createMs[i], measured[i] ? "" : "  (extrapolated)");
    qInfo("perf P1   growth exponent 1 KB->10 KB %.2f, 10 KB->100 KB %.2f  (O(n)=1, O(n^2)=2)", exponent1to10, exponent10to100);
    qInfo("perf P1   sourceEditForVisualReplacement on the 10 KB line  %9.2f ms", editMs);
    qInfo("perf P1   100 KB multi-line document create       %9.2f ms", createHundredKbMs);
    qInfo("perf P1 1 MiB multi-line document (%lld lines):", static_cast<long long>(oneMb.count('\n')));
    qInfo("perf P1   SourceVisualMapping::create             %9.2f ms", createDocMs);
    qInfo("perf P1   TextEdit.text = document (Source pane)  %9.2f ms", setTextMs);
    qInfo("perf P1   Backend::visualProjection()             %9.2f ms", projectionMs);
    qInfo("perf P1   Backend::applyVisualEdit (1 keystroke)  %9.2f ms", applyMs);
    qInfo("perf P1   visualProjection() after the keystroke  %9.2f ms  => one Visual keystroke ~%.0f ms", reprojectionMs, applyMs + reprojectionMs);
    QVERIFY(createMs[2] < 15000); QVERIFY(editMs < 15000); QVERIFY(projectionMs < 15000); QVERIFY(applyMs < 15000);
    fixture.backend.discardRecovery();
}

// Image decode caching: twenty small PNGs referenced from one document.
void FomawriteTest::perfImageDecodeCaching() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QString source = "# Images\n\n";
    qint64 pngBytes = 0;
    for (int i = 0; i < 20; ++i) {
        QImage image(64, 64, QImage::Format_RGB32);
        image.fill(QColor::fromHsv((i * 18) % 360, 200, 220));
        const QString name = QString("image-%1.png").arg(i);
        QVERIFY(image.save(directory.filePath(name)));
        pngBytes += QFileInfo(directory.filePath(name)).size();
        source += QString("![Image %1](%2)\n\n").arg(i).arg(name) + perfProse(30, i) + "\n\n";
    }
    Cycle132Editor fixture; QVERIFY(fixture.create());
    fixture.editor->setProperty("text", source);
    fixture.backend.saveAs(QUrl::fromLocalFile(directory.filePath("source.md")));
    QTest::qWait(400);
    QObject consumer;
    QElapsedTimer timer;
    timer.start();
    const auto first = fixture.backend.requestPublishingPreview("html", &consumer, "img:1");
    const double firstMs = perfMs(timer);
    QVERIFY2(first["ok"].toBool(), qPrintable(first["error"].toString()));
    QVERIFY(first["warning"].toString().isEmpty());
    QCOMPARE(cycle132Read(first["url"].toUrl()).count("data:image/png;base64,"), 20);
    fixture.editor->setProperty("text", source + "x");
    timer.start();
    const auto second = fixture.backend.requestPublishingPreview("html", &consumer, "img:2");
    const double secondMs = perfMs(timer);
    QVERIFY2(second["ok"].toBool(), qPrintable(second["error"].toString()));
    QVERIFY(second["url"] != first["url"]);

    // The decode step in isolation: cold cache versus warm cache.
    const QUrl baseUrl = QUrl::fromLocalFile(directory.path() + '/');
    QString error, warning;
    const QString body = PublishingHtml::body(source, &error); QVERIFY(error.isEmpty());
    PublishingHtml::ImageCache cache;
    QByteArray signature;
    timer.start();
    const QString cold = PublishingHtml::embedImages(body, baseUrl, true, &error, &warning, &cache, &signature);
    const double coldMs = perfMs(timer);
    QVERIFY(error.isEmpty() && warning.isEmpty()); QCOMPARE(cache.size(), 20);
    timer.start();
    const QString warm = PublishingHtml::embedImages(body, baseUrl, true, &error, &warning, &cache, &signature);
    const double warmMs = perfMs(timer);
    QCOMPARE(warm, cold);
    timer.start();
    const QString uncached = PublishingHtml::embedImages(body, baseUrl, true, &error, &warning, nullptr, nullptr);
    const double uncachedMs = perfMs(timer);
    QCOMPARE(uncached, cold);

    // One large photo-sized PNG: the decode the cache was built to avoid.
    QImage large(3000, 2000, QImage::Format_RGB32);
    for (int y = 0; y < large.height(); ++y) {
        auto *row = reinterpret_cast<QRgb *>(large.scanLine(y));
        for (int x = 0; x < large.width(); ++x) row[x] = qRgb((x * 255) / large.width(), (y * 255) / large.height(), (x ^ y) & 0xff);
    }
    QVERIFY(large.save(directory.filePath("large.png")));
    const qint64 largeBytes = QFileInfo(directory.filePath("large.png")).size();
    const QString largeBody = PublishingHtml::body("![Large](large.png)\n", &error); QVERIFY(error.isEmpty());
    PublishingHtml::ImageCache largeCache;
    timer.start();
    const QString largeCold = PublishingHtml::embedImages(largeBody, baseUrl, true, &error, &warning, &largeCache, &signature);
    const double largeColdMs = perfMs(timer);
    QVERIFY2(error.isEmpty() && warning.isEmpty(), qPrintable(error + warning)); QVERIFY(largeCold.contains("data:image/png;base64,"));
    timer.start();
    const QString largeWarm = PublishingHtml::embedImages(largeBody, baseUrl, true, &error, &warning, &largeCache, &signature);
    const double largeWarmMs = perfMs(timer);
    QCOMPARE(largeWarm.size(), largeCold.size());

    qInfo("perf images: 20 PNGs (%lld bytes on disk), %lld-char source", static_cast<long long>(pngBytes), static_cast<long long>(source.size()));
    qInfo("perf images   first request (decode + embed)        %9.2f ms", firstMs);
    qInfo("perf images   request after 1-char edit (cached)    %9.2f ms", secondMs);
    qInfo("perf images   embedImages cold cache %7.2f ms   warm cache %7.2f ms   no cache (pre-132) %7.2f ms", coldMs, warmMs, uncachedMs);
    qInfo("perf images   one 3000x2000 PNG (%lld bytes): embedImages cold (read + decode + base64) %7.2f ms   warm cache %7.2f ms", static_cast<long long>(largeBytes), largeColdMs, largeWarmMs);
    QVERIFY(firstMs < 15000); QVERIFY(secondMs < 15000);
    fixture.backend.discardRecovery();
}
