# Cycle132 — publishing robustness

Build **0.3.0-dev28 / macOS 0.3.0 (132)**. Source/test changes only; no packaged bundles were refreshed (see Limits).

Addresses the publishing-lifecycle findings A1–A8, E4 of the [independent review](../../INDEPENDENT_REVIEW.md) (§3). Cycle 129 removed one trigger of the "blank or stale preview" incident; this cycle removes the design that made it possible: any file change in the themes folder cancelled every in-flight render, the expensive theme work ran before the cache was consulted, and every failure path threw away the last readable output.

## What changed

| Finding | Behavior now |
| --- | --- |
| A1 — any change in the themes folder (Finder `.DS_Store`, editor swap files, logs, edits to an unselected theme) cancelled renders and restarted the preview debounce | `refreshPublishingThemes` still notices folder changes cheaply, but compares what matters: the **catalog listing** and the **selected theme's sanitized CSS (or its error)**. Only the latter emits the new `publishingCssChanged` signal, which is what bumps the settings generation, cancels renders and reloads the preview. Catalog-only changes emit `publishingThemesChanged` for menus and nothing else. Unrelated files emit nothing. |
| A2 — theme CSS (fonts base64-encoded, up to 64 MiB) was re-read and every image re-decoded on every request, and the cache key was a SHA-256 over the whole HTML | The selected theme's CSS is **memoized per folder state** (`updatePublishingCss` / `selectedPublishingCss`); decoded images are cached per path while size and mtime are unchanged (`PublishingHtml::ImageCache`); the cache key uses a **fingerprint** of the inputs (body, CSS hash, custom CSS, print CSS, title, image stat signature, warning) instead of hashing megabytes of embedded bytes. |
| A3 — a new Chromium profile per PDF render | One off-the-record profile per `PublishingPdf` (no cache, no cookies, no storage), pages still per job for true cancellation. |
| A4 — timeout/failure blanked the last good render; a >30 s document could never be previewed | `failRefresh` keeps the output that belongs to the current document behind the notice; output from another document is still cleared. The QML deadline is 35 s so the renderer's own 30 s error is the one reported. |
| A5 — Web renderer crash left a silent blank pane | `onRenderProcessTerminated` reports and reloads. |
| A6 — Web completion depended on a viewport script that can return without completing; re-assigning an unchanged URL never navigated | Load success completes the refresh directly; `webOutputUrl` drives `web.url` imperatively and an unchanged URL forces `reload()`/navigation. |
| A7 — watcher silently stopped at 511 unsorted entries | Entries are sorted before capping; watches cover directories and `.css` files (the only files edited in place), snapshot covers all files (content hash for CSS, metadata for fonts/images). |
| A8 — every refresh `mkpath`ed the themes folder | A refresh never creates the folder; it watches the parent until it returns. `Open Themes Folder` / `Import` still create it. |
| E4 — preview `WebEngineView` allowed local file access | `localContentCanAccessFileUrls: false` (all assets are `data:` URLs). |

## Verification

- Full offscreen regression suite: ****212 passed, 0 failed, 0 skipped** (208 prior + 4 new; one test re-specified), with the two known Qt Material `SplitView` warnings unchanged. The folder test was strengthened to force a refresh after that run and re-verified green on its own.**.
- New tests in [`tests/cycle132-publishing.inc`](../../tests/cycle132-publishing.inc): unrelated-file churn in the themes folder every 300 ms while a PDF renders must still complete with zero theme signals (A1); the selected theme's CSS with an embedded 2 MiB font survives the font's deletion until the watcher reports it, then switches to the fallback (A2 memoization and invalidation); the folder moved aside is not recreated and is re-watched on return (A8); a Web-mode render completes, and a reload returning the same URL after the view navigated elsewhere completes again (A6 — the first `WebEngineView` component test in the suite).
- `publishingComponentTimeoutClearsStalePagesAndRecovers` is re-specified as `publishingComponentTimeoutKeepsLastOutputAndRecovers`: after a timeout the readable render stays, the late result is rejected, retry clears the notice, and another document's failure still blanks (A4).
- Red/green: with the seven source fixes stashed and the tests kept (minus the two spies on the new `publishingCssChanged` signal, which does not exist in the old header), **all five tests fail on the Cycle 131 code** at the exact defect: the churned render never completes, the theme CSS is recomputed and loses its font, Web mode never completes after the view navigated away, the timeout blanks the pages, and a forced refresh recreates the moved-aside folder. With the fixes restored they pass.

## Limits

- Offscreen Qt only; no packaged bundle refreshed; no native `check-publishing-startup` rerun in this cycle (R1 remains open while the installed app is in use).
- HTML generation still assembles the full document (memoized CSS concatenated with the body) on every request before the cache lookup; only the expensive parts are memoized and no work was moved off the GUI thread.
- Fonts and images edited **in place** with size, mtime and birth time all preserved are no longer detected (they have no file watch); atomic replacement and any metadata change are.
- The Chromium page (and therefore renderer process) is still created per PDF job; only the profile is shared.
- `PublishingPreview.qml` keeps its field-based state; the explicit state enum recommended by the review is deferred to Cycle 133's structural pass.
