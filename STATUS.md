# Fomawrite — status

Updated 22 October 2026. This is the authoritative current-build record. Direction: [MISSION.md](MISSION.md). History: [CHANGELOG.md](CHANGELOG.md). Source map: [ARCHITECTURE.md](ARCHITECTURE.md).

| Item | Current state |
| --- | --- |
| **Current review build** | **`0.3.0-dev49` / macOS `0.3.0 (153)`** |
| **Last completed cycle** | 153 — stability: revision-neutral spelling refresh, native Live keyboard/save/history checks, embedded editor/theme identity verification, editor CI, and reconciled documentation. [Evidence](research/cycle-153/README.md). |
| **Repository** | [andrewjngray/fomawrite_mac](https://github.com/andrewjngray/fomawrite_mac), `master`; local checkout `/Users/andrewgray/repo/projects/fomawrite_mac`. The former `omawrite_mac` directory name is obsolete. |
| **Local regression suite** | **299 passed, 0 failed, 0 skipped** (`./bin/test`, Qt 6.11.2, Apple Silicon); **321 editor tests passed**, TypeScript typecheck passed, editor assets rebuilt identically. |
| **Native app acceptance** | Dev, distribution and installed build 153 all pass: 124 footer states, 5 daily-writing, 5 zoom, 7 editing, 3 pane-chrome, 5 editing-layout and 12 publishing-theme groups. Tests check actual Web/PDF pixels and current document identity. [Bundle identities](research/cycle-153/verified-builds.json). |
| **Startup** | **Cold and restored passed**, two document windows, correct PDF content and visible page paint, normal quit. Separate instrumented startup binary from the same source. |
| **Synchronized bundles** | `/Applications/Fomawrite.app`, `dist/Fomawrite.app`, `dist/Fomawrite Dev.app`. All build 153, with verified signatures and matching embedded resources. Installed and distribution release executables are byte-identical; Dev has its own signed identity. |
| **CI** | [Workflow](.github/workflows/ci.yml): macOS Qt 6.11.3 build/test; independent Ubuntu Node 22.16.0 editor lockfile install, tests, typecheck, build, and committed-asset freshness check. [Latest runs](https://github.com/andrewjngray/fomawrite_mac/actions/workflows/ci.yml). |
| **Review baseline** | [INDEPENDENT_REVIEW.md](INDEPENDENT_REVIEW.md), 6 October. Cycles 131–134 closed the listed correctness findings and established the current Live-editor direction. |

## What is shipped

- **One canonical Markdown document**, edited through Source or a CodeMirror 6 Live page hosted in Qt WebEngine. Live edits mirror into C++; saving remains the host's responsibility.
- **Source**: Manuscript with hanging Markdown markers, Editorial, Book and Code appearances; Code includes line numbers, indentation guides and no wrap. Focus and typewriter modes remain available.
- **Live**: hidden inactive markers, images, tasks, rules, highlighted fences, table grids, KaTeX math, footnotes, table of contents and front matter. Format/Edit commands, Find, Undo/Redo, link editing, outline navigation, image insertion and Source/Live caret transfer are integrated.
- **Publishing**: a separate read-only Web/PDF pane, CSS output themes including imported Typora-style themes, local theme assets, and basic font/page presets. Live follows the output theme with layout filtering by default; exact-theme following is optional.
- **Writing assistance**: native spelling/grammar plus the pinned Harper engine, including Live marks and suggestions. Cycles 143–147 implemented this; spelling in Live is no longer an undecided feature.
- **Navigation and appearance**: document-folder navigation, current-folder chooser, middle-pane back/forward and enclosing-folder shortcut (148–151); Omawrite Source and Output appearances (152).

## Verification and its limits

Cycle 153 found a real product defect: checker readiness used full syntax rehighlighting, advancing the document revision despite unchanged Markdown. The fix uses layout-only spelling restyling. Its regression fails on the previous code and passes now. Native checks also had obsolete Claude Like appearance expectations; those were corrected to the already-shipped near-white sans-serif theme, retaining content, pagination and paint assertions.

The native acceptance harness launches the actual Cocoa app bundles with disposable files/settings and sends synthetic Qt pointer/key events. The new Live checks verify page focus and caret before typing, canonical/page agreement, exact UTF-8 save/reopen, mode switching and Undo/Redo. This is stronger integration coverage, but is not a human physical-input, VoiceOver or multi-display certification. See the [daily-writing follow-through](research/usability/cycle-153.md).

Earlier byte-preservation and preview robustness regressions remain in the full suite. Historical performance measurements, not remeasured this cycle: about 450 MB resident per window with the hidden Live page and Harper; 1 MiB load about 55 ms and a mirrored keystroke about 75 ms. Runner-dependent macOS grammar tests can skip in GitHub CI; the local run skipped none.

## Next steps

1. Use build 153 for ordinary writing and record reproducible friction in [docs/live-friction.md](docs/live-friction.md), particularly physical selection/format/link interactions, rapid document changes, and quit/reopen.
2. Keep both CI jobs green and require native current-document/paint checks before distributing preview or editor changes. `bin/test` regenerates tracked gallery images; review or restore only those generated images after a run.
3. Prioritize demonstrated correctness and usability problems before adding features or restructuring the backend.

Known limits: no Mermaid; images need a saved document; images and footnote references in Live table cells remain text; authorship marks are invisible in Live; unknown/plaintext fences intentionally have no syntax colors. Theme-supplied colors are not contrast-checked, and exact-theme following is global. Fast interactions involving selection, history and link editing warrant physical-input follow-through even though formatting requests now fetch the current selection.

Optional later work: smaller language bundle, memory profiling, remaining Backend extractions, a test/static-library split and an explicit publishing-preview state model. The Grammarly experiment was superseded by native checking plus Harper. Roam/Teams work belongs in its separate discussion, not this project's active backlog.

Refresh bundles only with the app closed: `./bin/package-mac && ./bin/prepare-dev-app && ./bin/install-mac`. No public release or website PR was published in this cycle.
