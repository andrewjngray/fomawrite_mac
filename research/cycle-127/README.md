# Cycle 127 — publishing preview continuity

Review build **0.3.0-dev23 / macOS 0.3.0 (127)** responds to Andrew's blank publishing pane, the readable-local-image error and flashing during refresh.

Preview now renders the document around missing, unreadable, damaged, remote or oversized images, substituting escaped alt-text placeholders and a warning. Valid local PNG/JPEG/GIF/WebP assets embed within 5 MiB per image and 20 MiB total. Export and print remain strict so an incomplete preview does not silently become a successful export.

Successful results cache per consumer using generated HTML, format, document base URL and warning. Embedded image bytes and print-layout CSS invalidate the cache when assets or paper settings change. Unchanged output reuses its URL; identical pending PDF work reuses its request ID. Changed requests, cache hits and errors cancel stale PDF work before it can apply. Web-only and PDF consumers release pinned files and cache state on destruction. The publishing surface retains a painted snapshot during replacement loading and skips navigation for an unchanged URL. These paths do not modify canonical Markdown, saved source files or writing history.

## Verification

Build and packaging pass. All **197 regressions pass**, with **0 failures or skips**. Dev and the installed app each pass **159 native checks**, including repeated unchanged refreshes, readable missing-image Web/PDF output, painted-frame retention and source/Undo preservation. Both have **zero unexpected QML warnings**. Six deliberately triggered missing-image diagnostics from the left Visual Edit resource loader are recorded separately in each native report. Existing Qt timer/compositor teardown diagnostics remain in local logs.

Inspected native captures of Web and PDF during replacement and the missing-image Web document. All three app copies are build127 with valid strict signatures: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app` and `/Applications/Fomawrite.app`. The packaged executable matches the installed copy; it did not receive an independent native run. See [verified-builds.json](verified-builds.json) for source and binary identities.

Andrew's physical blank-pane/flashing report is **not yet proven resolved**. Repeat the original workflow after verified installation; use the [optional exercise](../usability/cycle-127.md). Synthetic native input cannot replace that review. Physical input/IME, VoiceOver, display behavior, printer output and broader CSS/Markdown compatibility remain separate acceptance work. No public release or notarization is included.

Generated logs, screenshots, private documents and raw output stay local; publish this handoff and reviewed build identities only.
