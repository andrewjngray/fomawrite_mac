# Fomawrite — status

This is the only place the current review build is recorded. Update it at the end of every cycle (see [AGENTS.md](AGENTS.md)). What the build is for, and the current measure of progress, are in [MISSION.md](MISSION.md). History: [CHANGELOG.md](CHANGELOG.md). Source map: [ARCHITECTURE.md](ARCHITECTURE.md).

| | |
| --- | --- |
| **Current review build** | `0.3.0-dev31` / macOS `0.3.0 (135)` |
| **Last completed cycle** | 135 — Visual Edit retired, builds aligned ([record](research/cycle-135/README.md)), 7 October 2026 |
| **Regression suite** | **207 passed, 0 failed, 0 skipped** (`./bin/test`, offscreen Qt 6.11.2, Apple Silicon; down from 240 because the Visual Edit and mapper suites left with the feature — see the record); editor page **176 passed** (`cd src/editor && npm test`). Two known Qt Material `SplitView` null-parent warnings in `savesAndOpensFromFooterMenu`. |
| **Packaged bundles** | `dist/Fomawrite.app`, `dist/Fomawrite Dev.app` and `/Applications/Fomawrite.app` are all at build **135** ([manifest](research/cycle-135/verified-builds.json)); the stale Omawrite bundles were removed from `dist/`. **Refresh with the app closed:** `./bin/package-mac && ./bin/prepare-dev-app && ./bin/install-mac`. |
| **CI** | [`.github/workflows/ci.yml`](.github/workflows/ci.yml): macOS, pinned Qt 6.11.3, `./bin/build && ./bin/test`. First run proved the pipeline but was red on four runner-only UI-click tests; Cycle 134 root-caused one (a viewport-transition timer stealing focus) and hardened the click helper for the other three — see the Actions tab for the run after `e07f727`. |
| **Baseline reviewed** | [INDEPENDENT_REVIEW.md](INDEPENDENT_REVIEW.md) (6 October 2026). C1–C6 closed by Cycle 131; A1–A8/E4 by Cycle 132; P1 and part of §4 by Cycle 133; the §6 direction question answered by Cycle 134. |

## What the Live editor is (Cycle 134)

One CodeMirror 6 editor page (`src/editor/`, TypeScript, built to the committed `dist/editor.js`, 1.4 MB) hosted in the app's Chromium `WebEngineView` and connected to the C++ document over QWebChannel (`EditorBridge`, `LiveEditorPane.qml`). The Markdown text stays canonical in C++: page edits mirror into the `QTextDocument` byte-exactly and C++-side edits mirror back. Presentation modes over the same text:

- **Live** — markers hide until the caret enters them; images (resolved by the host), task checkboxes, rules, syntax-highlighted fences (curated language set), table grids, KaTeX math, footnotes, `[toc]`, front matter; styled by the same `#write` theme CSS as published output (Typora-style themes apply).
- **Source** — Manuscript (bundled iA Writer Mono, hanging markers), Editorial, Book; **Code** (line numbers, indent guides, no wrap); focus and typewriter modes.
- App integration: footer capsule **Source / Live** and View menu; Format menu/toolbar/shortcuts act on the page while it has focus; Undo/Redo and Find route to the page; scroll sync with the publishing pane both ways; caret carried across mode switches; outline jumps land in the page; pasted/dropped images saved beside the document; an in-page link editor (Ctrl+K); the whole Edit menu (Cut/Copy/Paste/Select All, Copy Formatted/HTML/Markdown, Paste As) acts on the page; theme changes re-style without reloading; the page is created on first use and kept.

Measured: 1 MiB document loads in ~55 ms; one keystroke on it mirrors in ~75 ms (two channel hops); first page ready in ~360 ms. Screenshots: [research/cycle-134/captures](research/cycle-134/captures/).

## Verified

- **Live editor** (Cycle 134): byte-exact mirror both ways including ZWJ emoji, combining marks, CJK; undo mirrored; mode switches keep one text; Cycle 131 persistence fixtures (CRLF, BOM, NBSP, graphemes) edited in the page and saved by C++ keep their bytes; host edits reach the page while format-only changes do not; Format commands apply at the page selection; host commands (find/selectAll) and cursor placement; theme push without reload; outline navigation into Live and back; images pasted or dropped into the page are saved beside the document and inserted as Markdown (whole-stack test), refusals reach the notice banner; Insert Link fills the page's link panel and lands in the canonical text with one-step undo; inline math renders inside table cells (capture refreshed); a host change that disagrees with the mirror (Qt's empty-document quirk) reloads the page text instead of being refused (red/green). Editor page: changes, decorations, modes, appearance, blocks, tables (incl. math in cells), math, extras, languages, images, links — 176 node tests.
- **Byte-exact persistence** (Cycle 131, red/green) and **publishing robustness** (Cycle 132, red/green) as before; **structure** (Cycle 133): `Publisher` behind `PublishingSource`, dead code removed, `macbridge.h`, `NativeMenuBar.qml`, CI, docs. (The mapper and its fuzz suite were removed with Visual Edit in Cycle 135.)
- **Test isolation** (Cycle 134): per-process app-data directories (`FomawriteTests-<pid>`), recovery-write waits, motion-settling click helper, viewport-settle before auxiliary focus.

## Open

- **Native acceptance**: `bin/check-document-views` / `bin/check-publishing-startup` have not been rerun since build 130; their Visual Edit steps now drive Live instead and have never run natively. The Live editor has never been driven by physical input: build 135 is installed, try it.
- **Live editor limits**: no Mermaid; pasted images go to `images/` beside a *saved* document (an untitled document refuses with a notice); authorship marks invisible in Live; Live links use the page's own panel (label/URL/title, edit/remove), not the Source dialog; images and footnote references inside table cells show as text; `plaintext`/unknown fences unhighlighted by design; the page's selection is reported with a 30 ms debounce, so a Format command issued within 30 ms of a selection change may use the previous selection.
- **App bug found by the CI work**: a viewport-transition timer can steal focus from an auxiliary field (e.g. the link editor label) right after it opens — fixed in the test's sequencing, now fixed in the app as well (`viewportTransitionKeepsAuxiliaryFocus`, red/green).
- **Decisions for Andrew**: Mermaid (+2.5 MB)? Drop C/C++/PHP/Rust grammars (−280 KB)? Spell check in Live (WebEngine dictionaries)?
- **Structure still open** from the review: remaining `Backend` extractions, static library/test split, `PublishingPreview.qml` state enum, acceptance code in the product (deliberate).
