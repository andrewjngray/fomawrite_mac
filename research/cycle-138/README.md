# Cycle 138 — native theme-folder checks and the Live selection race (overnight, 8 October 2026)

**Build:** `0.3.0-dev34` / macOS `0.3.0 (138)`. **Plan:** [research/overnight-2026-10-08/PLAN.md](../overnight-2026-10-08/PLAN.md). Builder: one Qt+page agent (Sonnet); reviewer: separate agent; planning: Fable.

## What changed

| Finding | Root cause | Change |
| --- | --- | --- |
| Three `bin/check-document-views` publishing-theme checks failed since Cycle 132 ("Cannot create disposable folder themes", "Cannot restore folder fixture theme", "Theme folder menu did not open for previewThemeImport") | Cycle 132 (commit `41d1ba1`, carried into `Publisher::watchThemes`) deliberately stopped creating `<AppData>/publishing-themes` on startup so a writer who moved the folder aside never finds an empty replacement; the harness runs in an empty isolated app-data directory, and the fixture's `writeTheme` wrote into the folder without creating it. | The fixture now creates the folder first (`QDir().mkpath`), as every Qt test that uses the folder already did. The app is unchanged; its behaviour is deliberate and tested. |
| Live selection race: the page reports its selection with a 30 ms debounce, so a Format command issued within that window acted on the previous selection | Host operations read `EditorBridge::lastSelectionStart()/End()` | New bridge pair `requestSelection(token)` (host→page) / `selectionReply(token, anchor, head)` (page→host, read from the editor state at the moment of the request). `Backend::liveWrapSelection`, `liveReplaceSelection`, `liveEditMarkdown` and `liveCopySelection` queue the command (FIFO, one request in flight), apply it in the reply, fall back to the last reported selection if no reply arrives within one second, and keep the synchronous path when the page is not ready. Public signatures unchanged; the return value now means "accepted". |

## Verification

- Red/green: with the old `backend.cpp` and the new tests, `liveFormatCommandUsesTheSelectionMadeJustBefore` produced `****one two three` instead of `one **two** three`, and `liveSelectionRequestFallsBackWhenThePageDoesNotAnswer` saw no request sent; both pass with the fix (the race test passed 8 of 8 repeated runs).
- Qt suite: **212 passed, 0 failed, 0 skipped**; page: **248 node tests** (`test/selection.test.mjs` added).
- Native harness: the app's own report passes (`passed: true`, 124 footer states, 12 publishing-theme checks). The wrapper script `bin/check-document-views` still exits 1 on four stale checks of its own: thresholds of ≥7 daily-writing and ≥6 editor acceptance checks (left over from before Cycle 135 retired Visual Edit's checks), a missing embedded hash for `NativeMenuBar.qml` in `documentviewcheck.cpp`'s resource list, and the unsigned `build/` app. Fixed in this cycle: thresholds lowered to the five checks each category has had since Visual Edit was retired, `NativeMenuBar.qml` added to the embedded-hash list; the signature check is expected to fail for the unsigned `build/` app and passes for `dist/`.
- Review: verdict ship with fixes. Medium: queued commands were never invalidated when the document was swapped or the page reloaded; low: a late reply overwrote the last-known selection after the fallback had applied the edit. Both fixed: each queued operation carries the document generation (bumped by `syncLiveEditor` and on load) and is dropped if it differs; the bridge accepts a `selectionReply` only for the request still in flight and the fallback cancels it. Test `liveQueuedCommandsDropOnDocumentSwapAndIgnoreLateReplies`. Lows left open and documented: commands sent through other paths (undo, `runCommand`) are not queued behind a pending request; a mode switch within milliseconds of a Format command may not carry the caret.
- Bundles: `dist/Fomawrite.app`, `dist/Fomawrite Dev.app` and `/Applications/Fomawrite.app` all at build 138 ([verified-builds.json](../cycle-138/verified-builds.json)), installed 8 October 22:40 with the app closed; `bin/check-document-views` passes in full against the release bundle.

## Limits

- The one-second fallback is a judgement call: a page that is alive but slow would see the command applied at the last reported selection.
- `liveCopySelection` with a collapsed selection now returns true ("accepted") and copies nothing.
