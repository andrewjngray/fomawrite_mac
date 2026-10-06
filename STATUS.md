# Fomawrite — status

This is the only place the current review build is recorded. Update it at the end of every cycle (see [AGENTS.md](AGENTS.md)). History: [CHANGELOG.md](CHANGELOG.md). Source map: [ARCHITECTURE.md](ARCHITECTURE.md).

| | |
| --- | --- |
| **Current review build** | `0.3.0-dev28` / macOS `0.3.0 (132)` |
| **Last completed cycle** | 132 — publishing robustness ([record](research/cycle-132/README.md)), 6 October 2026 |
| **Regression suite** | **212 passed, 0 failed, 0 skipped** (`./bin/test`, offscreen Qt 6.11.2, Apple Silicon). Two known Qt Material `SplitView` null-parent warnings in `savesAndOpensFromFooterMenu`. |
| **Packaged bundles** | Not refreshed since build **130**. `dist/Fomawrite.app`, `dist/Fomawrite Dev.app` and `/Applications/Fomawrite.app` are at build 130 ([manifest](research/cycle-130/verified-builds.json)); Cycles 131 and 132 were source/tests only. |
| **CI** | [`.github/workflows/ci.yml`](.github/workflows/ci.yml): macOS, pinned Qt 6.11.3, `./bin/build && ./bin/test`. Added in Cycle 133; no green run recorded yet. |
| **Baseline reviewed** | [INDEPENDENT_REVIEW.md](INDEPENDENT_REVIEW.md) (6 October 2026, `master` @ `559a1cd`, Cycle 130). Findings C1–C6 closed by Cycle 131; A1–A8 and E4 closed by Cycle 132. |

## Verified

Offscreen regression evidence; "red/green" means the test fails on the previous cycle's code.

- **Byte-exact persistence** (Cycle 131, red/green): LF, CRLF, CR, BOM, BOM+CRLF, NBSP/guillemets, U+2028, no-final-newline and trailing-whitespace fixtures round-trip byte-for-byte; edits keep the file's own BOM and line-ending style; Latin-1 and UTF-16 input is readable but in-place Save/autosave is refused and Save As writes UTF-8; CRLF files autosave, rename and move; a manual Save that would overwrite a newer on-disk version raises **File changed / Save Anyway**; crash recovery no longer overwrites an external writer's version.
- **Publishing robustness** (Cycle 132, red/green): unrelated file churn in the themes folder every 300 ms no longer cancels a render (zero theme signals); the selected theme's CSS is memoized and invalidated by the watcher; a timeout keeps the current document's last readable output and a late result is rejected; Web mode completes and re-completes on an unchanged URL; a themes folder moved aside is not recreated. First `WebEngineView` component test in the suite.
- **Earlier core**, confirmed by the review: atomic `QSaveFile` saves, lock-slot crash recovery, single undo stack, bounded Source↔Visual mapper, escaped raw HTML, rejected `javascript:`/`data:` hrefs, CSS sanitiser, CSP + interceptor on the PDF path.
- **Native bundle checks at build 130**: menu checks and focused preview checks passed on Dev, packaged and installed copies ([record](research/cycle-130/README.md)); Cycle 129's cold/restored normal-startup harness passed with populated themes.

## Open

- **Structure (Cycle 133 scope)**: `Backend` (~4.0k lines + `backendpublishing.inc`) and `Main.qml` (~3.7k lines) god objects; test and acceptance code compiled into the product (`documentviewcheck.cpp`, `#include`d `tests/*.inc` in `main.cpp`); one ~10k-line test TU; `PublishingPreview.qml` still field-based rather than an explicit state machine; HTML generation still on the GUI thread; one Chromium page per PDF job. Gate: CI green, test count ≥ 212, `backend.cpp` < 2,000 lines, no test code in the product binary.
- **Native acceptance**: no `bin/check-publishing-startup` / `bin/check-document-views` rerun since build 130 (the installed app was in use); physical keyboard/trackpad, IME, VoiceOver, multi-display, sustained-use and `bin/test-window-routing` remain Andrew's acceptance work.
- **Documented behaviour limits**: a document containing U+2029 becomes a paragraph break; mixed line endings are normalised to the dominant style on save; UTF-16 files are displayed but written back as UTF-8 via Save As; fonts/images edited in place with size, mtime and birth time all preserved are not detected; the mapper's CRLF branches are unreachable from the live app.
- **Distribution**: bundles are ad-hoc signed, not notarized; no Developer ID path; the Linux build is probably broken (nothing exercises it since the WebEngine/PDF work).
- **Product direction** (Cycle 134): decide between the iA-Writer-plus-publishing identity, a CodeMirror live-preview editor inside the shipped Chromium, or a separate editor — see review §6.
