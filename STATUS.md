# Fomawrite — status

Updated 22 November 2026. This is the authoritative current-build record. Direction: [MISSION.md](MISSION.md). History: [CHANGELOG.md](CHANGELOG.md). Source map: [ARCHITECTURE.md](ARCHITECTURE.md).

| Item | Current state |
| --- | --- |
| **Current review build** | **`0.3.0-dev66` / macOS `0.3.0 (171)`** in signed `build/Fomawrite.app` and `dist/Fomawrite Dev.app`. Distribution and Applications are on 166. |
| **In review** | 171 — Wider Mac PDF Split sidebars ([record](research/cycle-171/README.md)). 170 — Split view pane resizing ([record](research/cycle-170/README.md)). 169 — Mac Source Find and PDF note cards ([record](research/cycle-169/README.md)). 168 — Mac toolbar rotation, active markup tools, Fit Page ([record](research/cycle-168/README.md)). 167 — Mac PDF note popover ([record](research/cycle-167/README.md)). 166 — PDF page controls, Save prompts and undo history ([record](research/cycle-166/README.md)). ARM64 and x64 Windows test packages passed [Windows CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/44830250294). |
| **Last CI-verified cycle** | 171 — [Mac CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/44946120769) and [Windows ARM64/x64 CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/44946256820) passed. 165 — page edits with qpdf, safe saving, notes renumbered at save; no interface yet ([record](research/cycle-165/README.md)); 164 — notes pinned on the page, page/notes navigation, unplaced list and marker chips in Live ([record](research/cycle-164/README.md)); 163 — highlights, underline and strikethrough stored in the PDF's notes file ([record](research/cycle-163/README.md)); 162 — PDF links and integration, completing Phase 1 of the PDF programme ([record](research/cycle-162/README.md)); 161 — the PDF pane: toolbar, zoom, selection, find, thumbnails, Inspector, PDF menu ([record](research/cycle-161/README.md)); 160 — PDF core: a PDF opens in the preview pane's place with its notes in the editor ([record](research/cycle-160/README.md)); 159 — PDF plan Phase 0: decisions, spikes and the reviewed programme, no build ([record](research/cycle-159/README.md), [programme](docs/pdf-plan.md)); 158 — licence notices in the app and no unused or GPL-only Qt libraries bundled ([record](research/cycle-158/README.md)); 157 — the dark runtime icon bundled as a PNG for Codex's Open menu ([record](research/cycle-157/README.md)); 156 — a new document takes the keyboard at its first line ([record](research/cycle-156/README.md)); 155 — toolbar formatting fixed in Live, Word-like Format Bar, formatting marks (¶ / ⌘8) in Live and Source, explorer "+ ▾" ([record](research/cycle-155/README.md)); 154 — Fomawrite as the default app for Markdown and plain text; the installer sets it ([record](research/cycle-154/README.md)); 153 — stability: revision-neutral spelling refresh, native Live keyboard/save/history checks, embedded editor/theme identity verification, editor CI, and reconciled documentation. [Evidence](research/cycle-153/README.md). |
| **Repository** | [andrewjngray/fomawrite_mac](https://github.com/andrewjngray/fomawrite_mac), `master`; local checkout `/Users/andrewgray/repo/projects/fomawrite_mac`. The former `omawrite_mac` directory name is obsolete. |
| **Local regression suite** | **431 passed, 0 failed, 0 skipped** (`./bin/test`, Qt 6.11.2, Apple Silicon, cycle 171); **334 editor tests passed**, TypeScript typecheck passed, editor assets rebuilt identically. |
| **Native app acceptance** | Signed Dev build 171 passed the document-view harness and the new sidebar ranges passed in Cocoa ([record](research/cycle-171/README.md)). Signed Dev build 170 passed the native document-view harness and the affected drag workflows passed in Cocoa ([record](research/cycle-170/README.md)). Signed build 169 passed the native document-view checks and the new Find and note workflows passed in Cocoa ([record](research/cycle-169/README.md)). Dev build 168 passed the native document-view checks and the toolbar and note workflows passed the native Cocoa regressions ([record](research/cycle-168/README.md)). Dev, distribution and installed build 153 all pass: 124 footer states, 5 daily-writing, 5 zoom, 7 editing, 3 pane-chrome, 5 editing-layout and 12 publishing-theme groups. Tests check actual Web/PDF pixels and current document identity. [Bundle identities](research/cycle-153/verified-builds.json). |
| **Startup** | **Cold and restored passed**, two document windows, correct PDF content and visible page paint, normal quit. Separate instrumented startup binary from the same source. |
| **Local bundles** | The signed ARM64 Mac package was installed in `/Applications/Fomawrite.app` on 21 November. The package and installed executable match, and the signature and licence checks passed. The regular Windows launcher selects build 171 for ARM64 and x64. Both architectures passed [Windows CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/44946256820). |
| **CI** | [Workflow](.github/workflows/ci.yml): macOS Qt 6.11.3 build/test; independent Ubuntu Node 22.16.0 editor lockfile install, pinned Harper download, tests, typecheck, build, and committed-asset freshness check. [Latest runs](https://github.com/andrewjngray/fomawrite_mac/actions/workflows/ci.yml). |
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

1. Use the installed build for ordinary writing and record reproducible friction in [docs/live-friction.md](docs/live-friction.md), particularly physical selection/format/link interactions, rapid document changes, and quit/reopen.
2. Keep both CI jobs green and require native current-document/paint checks before distributing preview or editor changes. `bin/test` regenerates tracked gallery images; review or restore only those generated images after a run.
3. Prioritize demonstrated correctness and usability problems before adding features or restructuring the backend.

Known limits: no Mermaid; images need a saved document; images and footnote references in Live table cells remain text; authorship marks are invisible in Live; unknown/plaintext fences intentionally have no syntax colors. Theme-supplied colors are not contrast-checked, and exact-theme following is global. Fast interactions involving selection, history and link editing warrant physical-input follow-through even though formatting requests now fetch the current selection.

Optional later work: smaller language bundle, memory profiling, remaining Backend extractions, a test/static-library split and an explicit publishing-preview state model. The Grammarly experiment was superseded by native checking plus Harper. Roam/Teams work belongs in its separate discussion, not this project's active backlog.

Refresh bundles only with the app closed: `./bin/package-mac && ./bin/prepare-dev-app && ./bin/install-mac`. No public release or website PR was published in this cycle.

## Cycle 157 packaging verification

- App build and the new external-PNG regression check passed. The check fails
  on the pre-change bundle, then passes with byte-identical dark artwork.
- Distribution packaging, strict deep signature verification and the icon check
  passed on `dist/Fomawrite.app` (157). Full local regression suite passed: 323 tests, zero failures or skips.
- [CI for the implementation commit](https://github.com/andrewjngray/fomawrite_mac/actions/runs/42710098152):
  editor tests, typecheck and asset freshness passed; macOS job queued for a
  runner with no steps started. Master's macOS job was cancelled the same way
  on 9 November (and three times on 5 November): no runner, no steps.
- The same macOS build and tests, including `bin/check-codex-icon`, passed in
  CI on 15 November on `windows/port` `e928612`, which merges `8ed84e4`
  ([run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/43710538665)).
  Cycle 157 is complete on that evidence; master's own macOS job still needs
  a runner.
- Installed as `/Applications/Fomawrite.app` (157) on 15 November. The Codex
  icon configuration was visually confirmed by Andrew on 9 November. See [setup](docs/codex-open-with.md).
