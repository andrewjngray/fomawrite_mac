# Fomawrite — developer handoff and independent review brief

Prepared **6 October 2026 (Pacific/Auckland)** for Andrew Gray. This is a snapshot of the work through **Cycle 129**, with navigation for a developer reviewing the implementation, tests and improvement opportunities. It does not claim that all user acceptance or competitor parity is complete.

## 1. Start here

- Repository: **[andrewjngray/fomawrite_mac](https://github.com/andrewjngray/fomawrite_mac)**.
- Git clone URL: `https://github.com/andrewjngray/fomawrite_mac.git`; current branch: **`master`**.
- Upstream: [omacom/omawrite](https://github.com/omacom/omawrite), retained as remote `upstream`. This is an MIT-licensed fork, renamed Fomawrite.
- Andrew's current checkout: **`/Users/andrewgray/repo/projects/fomawrite_mac`**. Historical tooling may still mention `omawrite_mac`; that is the obsolete checkout path.
- Current review build: **`0.3.0-dev25`**, macOS version **`0.3.0 (129)`**.
- Cycle129 source/test commit: **`cf0ed37982169f01773441b31d7d0759f5a98e81`**. The following documentation commit records its verified bundle identities. Historical Cycle128 baseline: `0466c3c3cdaad3ae0b0a35bb87607a0c9cd8b50b`.
- Authoritative current evidence: [Cycle129 report](research/cycle-129/README.md). All three local app copies are synchronized and verified; see the [Cycle129 manifest](research/cycle-129/verified-builds.json).

Read [AGENTS.md](AGENTS.md), [README](README.md), this handoff, and the [current acceptance ledger](docs/release-acceptance.md) before changing behavior. The [complete cycle log](docs/build-cycles.md) preserves the detailed history. Older plans contain historical checkpoints; use the current source and latest cycle evidence when they differ.

## 2. Product intent and agreed behavior

Fomawrite is a calm, local-file Markdown writing app, using iA Writer as a reference for structure/readability, Ulysses for writing workflows, and Typora for CSS document presentation. Documents remain plain UTF-8 Markdown. Preserve exact text, Undo/Redo, unsaved-change prompts, recovery and external-change handling.

The pane responsibilities were explicitly agreed after several rounds of confusing controls:

| Area | Responsibility | Controls |
| --- | --- | --- |
| Library and organizer | Navigate folders, files, favorites, recents and queries/tags | Independent navigation visibility and widths |
| Left document pane | Edit canonical Markdown | **Source / Visual Edit**, writing appearance and editor zoom beneath/within this pane |
| Right document pane | Read-only publishing output | **Web / PDF**, output theme and independent publishing zoom |
| Overall document layout | Show one editing pane or editing plus output | Separate **Single / Split** group; preview-only reading remains a separate action |

Source shows **all Markdown syntax**, including emphasis markers. Visual Edit is a bounded formatted editing projection; unsupported structures use explicit Source fallback. The right publishing pane is not another editing surface. Output styling must not change Markdown or the left editor's appearance.

Document header/footer bars fade during editing or scrolling. Hovering the relevant edge reveals a bar; it then **stays visible after the pointer leaves until another edit or scroll**. Open menus and keyboard access keep controls reachable. **View → Title Bar → Fade In/Out / Always Show** and the auto-hide checkbox use the same preference. Studio pairs a soft grey editing surface with a warm cream publishing surround; output page colors also depend on the chosen theme.

## 3. Work completed so far

The repository contains many incremental cycles. This groups the work by capability; individual implementation and acceptance details remain in the linked records.

### Foundation and daily writing

- Established the macOS Qt build, app bundle, native menus/dialogs, Command shortcuts, Finder document opening and platform appearance integration.
- Retained and extended local saving, recovery, external-change handling, unsaved-work protection, previous-save history and guarded application quitting.
- Added library/organizer navigation, favorites/recents, search, tags and saved queries; resizable/collapsible navigation and document panes.
- Developed multi-window/native-tab behavior, launch forwarding and workspace checkpoints/restoration. Some operating-system activation and physical-input scenarios remain open.
- Added formatting commands, focus tools, outline/navigation, Find/Replace, local document/heading links and authorship metadata workflows. Legacy `.omawrite-authors.json` sidecars remain compatible.
- Renamed the product and bundles to Fomawrite, migrated prior preferences/state without deleting originals, and supplied the approved app icons while retaining upstream/font licenses.

### Workspace and editing refinement

- Reworked the workspace geometry, typography, icons, header/footer controls and narrow-window behavior; aligned controls with the panes they affect.
- Added safe source-to-visual mapping for supported text edits, inline code, simple lists/tasks, simple table cells and image alt/caption editing. Expanded keyboard navigation and focus ownership, inline-link editing and table-cell Tab/Shift-Tab navigation.
- Added independent editor/output zoom, optional linking, split-divider balancing and reading-anchor preservation.
- Clarified document identity with filename/icon/Edited presentation and compact file-list rows with filename emphasis, dates and grey excerpts.
- Separated editing mode from layout mode, then established left editing versus right read-only publishing.
- Added sticky edge-revealed bars, explicit visibility preferences, grey editing surfaces and warm publishing surroundings.

### Source appearance and publishing

- Source appearances: **Manuscript, Editorial, Book, Code**. Manuscript uses bundled iA Writer Mono, readable line spacing, hanging list/quote indents and heading markers outside the aligned body column.
- Fenced code has continuous block shading, including empty lines; inline code remains compact.
- Code appearance uses Menlo, line numbers, indentation guides, horizontal scrolling, Tab/Shift-Tab and indented Return, with lightweight syntax colors for supported file/fence languages. It is not an IDE: no language server, completion, linting or debugger.
- Introduced semantic Markdown-to-HTML output using **MD4C 0.5.3**, with shared Chromium HTML/PDF rendering for publishing, export and Export Hub previews. PDF preview displays the generated PDF rather than a separate approximation.
- Added Claude Like and managed CSS publishing themes, local theme/resource imports, folder discovery/reload, font fallback diagnostics and authoritative single-theme selection. Basic font/page presets remain distinct from CSS themes.
- Added **Get More Themes…**, opening [Typora's theme directory](https://theme.typora.io/). This is a gallery link, not an automatic download/installation service.
- Made unavailable-image previews show placeholders/warnings; export and print remain stricter about image availability.
- Repaired blank/stale publishing output and strengthened tests to inspect the document actually displayed. Details below.

### Recent cycle index

| Cycles | Main work | Record |
| --- | --- | --- |
| 60–61 | Fomawrite identity/migration and checkout rename | [Rename design](docs/product-rename.md) |
| 90–108 | Workspace redesign, mapped editing/navigation, responsive controls and bundle checks | [Workspace plan](docs/workspace-ui-redesign-plan.md) |
| 109–116 | Lists, outline, find, pane zoom, formatting focus, links and table navigation | [Cycle log](docs/build-cycles.md) |
| 117–118 | Pane-owned footer controls, filename/file-list clarity, separate Editing/Layout groups | [117](research/cycle-117/README.md), [118](research/cycle-118/README.md) |
| 119–121 | Left editing/right publishing, quiet bars, sticky reveal and visibility preferences | [119](research/cycle-119/README.md), [120](research/cycle-120/README.md), [121](research/cycle-121/README.md) |
| 122–123 | Literal readable Source, Code appearance, hanging headings and block shading | [122](research/cycle-122/README.md), [123](research/cycle-123/README.md) |
| 124–126 | Semantic CSS themes/shared browser output, theme-folder fixes and gallery link | [124](research/cycle-124/README.md), [125](research/cycle-125/README.md), [126](research/cycle-126/README.md) |
| 127 | Missing-image handling, caching and attempted flicker reduction; subsequently reported stale/blank output | [127, historical](research/cycle-127/README.md) |
| 128 | Document identity, compositor-independent replacement, recovery and actual-pane verification | [128, historical](research/cycle-128/README.md) |
| 129 | Theme watcher feedback loop and normal cold/restored startup coverage | [129, current](research/cycle-129/README.md) |

## 4. Latest incident: what failed and what changed

Andrew still saw blank publishing panes in all app copies after Cycle128. The earlier repair addressed real document-identity and screenshot-gated replacement defects, but the verification missed a separate startup failure with his populated themes folder. Reporting the preview incident as repaired on that evidence was too broad.

`watchPublishingThemes` removed and re-added every `QFileSystemWatcher` path on each refresh. On macOS with the populated theme folder, that replayed directory notifications about every 200 ms. Each theme notification restarted the publishing pane's 220 ms debounce, so generation never began. The old normal-startup reproduction recorded 67 theme signals per backend in 15 seconds and no output. Shared themes explain why ordinary and Dev copies could both fail despite different bundle identities/workspace paths.

Cycle129 updates watcher paths only when membership changes and compares cached content snapshots before announcing actual theme changes. File events invalidate cached content so same-size edits with preserved modification times are detected; atomic replacements re-arm their watches. Explicit Reload Themes still forces refresh. An empty preview URL now shows loading status when generation is pending.

The new [normal-startup harness](bin/check-publishing-startup) uses the normal Main startup and persisted workspace path, enabled QML cache and guarded normal quit. It fails the old implementation with a copied user themes folder and passes the corrected implementation on cold and restored launches. It does not force frames. The added offscreen watcher tests **also pass the old implementation**: their passing count is not red/green evidence for this macOS failure. The earlier Cycle128 component tests remain evidence for their own defects.

A read-only check of Andrew's actual Dev saved workspace showed matching PDF output, with an OS screenshot visually inspected. Private document text, themes and captures are excluded from Git. Physical input, full UI acceptance and long-running use remain separate review work.

## 5. Architecture and source map

Stack: **C++17, Qt Quick/QML, Qt Widgets/native integration, Qt WebEngine and Qt PDF**; qmake project [fomawrite.pro](fomawrite.pro). Recent verification used Qt **6.11.2** on Apple Silicon/macOS **27.0.1**. The configured macOS deployment target is 14.0; that does not establish acceptance on every supported OS.

Canonical Markdown belongs to the C++ document backend. QML controls UI state and editing interactions. The visual editor maps supported changes back to source. Publishing independently converts source to semantic HTML, applies bounded theme resources, and renders Web or PDF output asynchronously.

| Concern | Primary files |
| --- | --- |
| App/window lifecycle, restore, launch routing | [main.cpp](src/main.cpp), [workspace.cpp](src/workspace.cpp), [workspace.h](src/workspace.h) |
| Canonical document, I/O, history, recovery and preferences | [backend.h](src/backend.h), [backend.cpp](src/backend.cpp) |
| Publishing requests, cache, identity, export/print integration | [backendpublishing.inc](src/backendpublishing.inc), included by the backend |
| Top-level UI and commands | [Main.qml](src/Main.qml), [WorkspaceCommands.qml](src/WorkspaceCommands.qml) |
| Pane geometry, headers/footers, visibility | [WorkspaceLayout.qml](src/WorkspaceLayout.qml), [WorkspaceHeader.qml](src/WorkspaceHeader.qml), [DocumentFooter.qml](src/DocumentFooter.qml), [WorkspaceFooter.qml](src/WorkspaceFooter.qml) |
| Right publishing pane and asynchronous state | [PublishingPreview.qml](src/PublishingPreview.qml) |
| Left Visual Edit projection | [PreviewPane.qml](src/PreviewPane.qml), [sourcevisualmapping.cpp](src/sourcevisualmapping.cpp), [visualtexthighlighter.cpp](src/visualtexthighlighter.cpp) |
| Source syntax/structure | [markdownhighlighter.cpp](src/markdownhighlighter.cpp), [markdownextensions.cpp](src/markdownextensions.cpp), [EditorMutations.js](src/EditorMutations.js) |
| Semantic HTML / browser PDF | [publishinghtml.cpp](src/publishinghtml.cpp), [publishingpdf.cpp](src/publishingpdf.cpp), [MD4C](src/vendor/md4c/README.md) |
| CSS catalog, imports/assets/fonts; basic settings | [publishingthemes.cpp](src/publishingthemes.cpp), [outputcss.cpp](src/outputcss.cpp), [Claude Like CSS](src/themes/claude-like.css) |
| Export UI | [ExportHub.qml](src/ExportHub.qml) |
| Library/organizer | [LibraryPane.qml](src/LibraryPane.qml), [OrganizerPane.qml](src/OrganizerPane.qml), [filelibrary.cpp](src/filelibrary.cpp) |
| Zoom/find/outline/links | [PaneZoomState.qml](src/PaneZoomState.qml), [DocumentFindBar.qml](src/DocumentFindBar.qml), [DocumentOutline.qml](src/DocumentOutline.qml), [LinkEditor.qml](src/LinkEditor.qml), [LinkSyntax.js](src/LinkSyntax.js) |
| macOS behavior and bundle metadata | [windowchrome_mac.mm](src/windowchrome_mac.mm), [systemtheme_mac.cpp](src/systemtheme_mac.cpp), [Info.plist](macos/Info.plist) |
| Embedded QML/assets/fonts | [resources.qrc](src/resources.qrc), `fonts/`, `macos/` |
| Tests | [tests.pro](tests/tests.pro), [tst_fomawrite.cpp](tests/tst_fomawrite.cpp), `tests/cycle*.inc` |
| Normal startup/restoration | [check-publishing-startup](bin/check-publishing-startup), [startup-preview-check.inc](tests/startup-preview-check.inc), [startup-preview-setup.inc](tests/startup-preview-setup.inc) |
| Actual-bundle verification | [check-document-views](bin/check-document-views), [documentviewcheck.cpp](src/documentviewcheck.cpp), [publishingpreviewacceptancecheck.inc](src/publishingpreviewacceptancecheck.inc) |

**Naming trap:** `PreviewPane.qml` serves the left Visual Edit surface; `PublishingPreview.qml` is the right read-only output. Trace the current bindings rather than inferring ownership from the older filename.

## 6. Files, builds, themes and state on Andrew's Mac

Paths below are local to Andrew's machine. Relative repository links elsewhere in this handoff work in another clone.

| Location | Contents / use |
| --- | --- |
| `/Users/andrewgray/repo/projects/fomawrite_mac` | Current Git checkout |
| `/Users/andrewgray/repo/projects/fomawrite_mac/build/Fomawrite.app` | Intermediate compiled app; not the distributable reference |
| `/Users/andrewgray/repo/projects/fomawrite_mac/dist/Fomawrite Dev.app` | Stable QA app; ID `io.github.andrewjngray.fomawrite.dev`; depends on the local development Qt installation |
| `/Users/andrewgray/repo/projects/fomawrite_mac/dist/Fomawrite.app` | Packaged ordinary app with deployed Qt dependencies; ID `io.github.andrewjngray.fomawrite` |
| `/Applications/Fomawrite.app` | Installed ordinary copy |
| `/Users/andrewgray/repo/projects/fomawrite_mac/build-tests/tst_fomawrite` | Built Qt regression executable |
| `/Users/andrewgray/Library/Application Support/AndrewGray/fomawrite/` | Application data: workspace checkpoints, recovery files/locks, migration marker, basic output settings and managed themes |
| `/Users/andrewgray/Library/Application Support/AndrewGray/fomawrite/publishing-themes/` | Managed CSS themes and their local resource folders |
| `/Users/andrewgray/Library/Application Support/AndrewGray/fomawrite/output-user-styles.json` | Basic output settings; separate from CSS theme files |
| `/Users/andrewgray/repo/projects/fomawrite_mac/research/cycle-129/` | Committed summary/manifest plus ignored local verification reports and captures |

All three local app copies are synchronized to build129 and pass strict signatures and focused preview checks. Ordinary and installed executable hashes match; Dev differs because of packaging/identity. See the [Cycle129 manifest](research/cycle-129/verified-builds.json). The previous installed copy is preserved at `/private/tmp/Fomawrite-before129-1791225132.app`.

The Finder modification date of an outer `.app` folder is not a reliable build identifier. Use **About Fomawrite**, `Contents/Info.plist`, and the verification manifest. No new public release was created; the older Mac 0.2.0 RC1 download predates this work.

Workspace filenames include an identity derived from the executable path, so copies at different paths can restore different workspaces. Do not assume distinct bundle IDs mean every app-data resource is isolated. Themes use the shared application-data location. Preferences use Qt settings under the app's organization/domain; diagnostic checks use disposable state.

For themes, put top-level `.css` files and their matching resource folders together in `publishing-themes`. **Open Themes Folder** opens that location; **Import Theme** feeds the same catalog; **Reload Themes** refreshes it. Local supported fonts/assets must accompany the CSS. A theme naming a font does not install that font. Unsupported/missing resources can produce fallback/advisories. Do not confuse theme CSS with workspace/recovery JSON files in the parent folder.

`build/`, `build-tests/`, `dist/`, private writing, imported personal themes and raw local captures are not shipped in Git. The committed manifest points to evidence recorded locally; another developer should rerun checks instead of expecting all raw captures in a fresh clone. Do not delete recovery files or overwrite a running app to troubleshoot preview state.

## 7. Build and run checks

On macOS, use Apple's Command Line Tools and Qt 6 including WebEngine. The repository README specifies Homebrew `qtbase qtdeclarative qttools qtwebengine`. `bin/qt-env` discovers qmake; `QMAKE=/path/to/qmake` overrides it. Its older missing-Qt hint omits WebEngine, which the current app requires.

From a clone:

```sh
git clone https://github.com/andrewjngray/fomawrite_mac.git
cd fomawrite_mac
./bin/build
./bin/test
./bin/run examples/Welcome.md
```

Build the app copies after normally closing any copy being replaced, preserving unsaved work:

```sh
./bin/package-mac
./bin/prepare-dev-app
```

Compilation alone does not refresh `dist` or `/Applications`. Packaging and Dev preparation are separate. Local bundles are **ad-hoc signed, not notarized**. Keep the stable Dev path/identity; see [development-app approvals](docs/development-app-approvals.md). Do not force-quit Andrew's normal editor or replace its live bundle.

Run native checks in a logged-in macOS desktop session. Run normal cold/restored startup with a fresh output directory, then the focused bundle checks:

```sh
./bin/check-publishing-startup /tmp/fomawrite-review-startup
./bin/check-document-views /tmp/fomawrite-review-preview --preview-only --app "dist/Fomawrite Dev.app"
./bin/check-document-views /tmp/fomawrite-review-full --app "dist/Fomawrite Dev.app"
./bin/check-document-views /tmp/fomawrite-review-ordinary --preview-only --app "dist/Fomawrite.app"
```

The checker creates disposable documents/settings, captures synthetic fixtures and verifies the actual executable, embedded QML, bundle version and signature. Keep its window available for native presentation. `./bin/test` uses **offscreen** Qt; it cannot replace native bundle verification. `./bin/test <QtTest-function-name>` runs a focused regression. [bin/test-window-routing](bin/test-window-routing) is a separate lifecycle/activation harness; it was not recertified by the Cycle 128 preview checks.

Do not publish raw screenshots of private documents. Use synthetic A/B/C documents for reproducible failures, then perform personal-workspace acceptance with Andrew's preserved data.

## 8. Test evidence and what the numbers mean

[Cycle129](research/cycle-129/README.md) records the current candidate:

| Layer | Result | Limits |
| --- | --- | --- |
| Final Qt regression suite | **204 passed, 0 failed, 0 skipped** | Offscreen; totals include setup/cleanup; two Qt Material SplitView warnings |
| Expanded final watcher fixture | **4 passed, 0 failed, 0 skipped** | Two cases plus setup/cleanup; added watcher tests also pass old code |
| Old native normal startup | **FAIL** with copied populated user themes | 67 theme signals/backend in 15 seconds; no publishing output |
| Corrected native normal startup | Cold **3327 ms**, restored **4260 ms**, both **PASS** | Compile-only harness; normal Main/cache/guarded quit; zero QML warnings |
| Actual Dev saved workspace | Matching PDF output observed | Read-only private check; OS screenshot inspected, excluded from Git |
| Dev, packaged ordinary and installed focused checks | Each **4 groups / 8 pane paints pass** | Zero QML warnings; strict signatures; focused preview scope, not full UI |

The [startup checker](bin/check-publishing-startup) defaults to a synthetic populated theme/resource fixture. `FOMAWRITE_STARTUP_PREVIEW_THEMES` optionally copies a theme folder into disposable state; `FOMAWRITE_STARTUP_PREVIEW_SOURCE` optionally copies a document. Treat those reports as private when personal input is used. Leave the output path fresh: the checker requires no existing disposable settings/checkpoint. A cold run saves a workspace by normal guarded quit; the restored run launches without document arguments. QML caching remains enabled, and readiness does not force frames.

The default synthetic populated-theme fixture also passes cold (3320 ms) and restored (3135 ms) startup. Dev was closed normally and relaunched through `open` without diagnostic injection. The final full suite emits two Qt Material `SplitView.qml` null-parent TypeErrors in `savesAndOpensFromFooterMenu`; native startup and focused bundle checks report zero QML warnings. The old failure and corrected cold/restored pass establish the relevant red/green startup evidence. Component/offscreen pass counts alone do not. Final local reports are under `/private/tmp/fomawrite-cycle129-final-startup`; the old failure is under `/private/tmp/fomawrite-startup-theme-loop-red-128`. Commit only sanitized summaries/manifests, never private raw logs/screenshots. [Optional user review](research/usability/cycle-129.md).

## 9. Independent review priorities and remaining work

Prioritize correctness and reproducible acceptance before further presentation features:

1. **Document identity and asynchronous preview lifecycle.** Inspect cancellation, stale callbacks, source/settings tokens, cached URLs, hidden/show, rapid A→B→C selection and simultaneous export/pane jobs. Assert what is visible, not just what was generated. Review error/timeout/retry paths and whether optional Web geometry/JavaScript callbacks can leave pending state or block recovery.
2. **Real application startup and persistence.** Reproduce the originally reported workflow, quit normally and relaunch a saved multi-window workspace. Check title, canonical editor text and publishing content agree throughout. Use the Cycle129 normal startup harness with populated themes; component tests alone missed this failure. Expand physical and sustained-workspace acceptance beyond its bounded cold/restored checks.
3. **Data integrity.** Exercise Unicode, unsaved edits, Undo/Redo, source/visual switching, file changes, recovery and Save As. Appearance/highlighting refresh must not create unintended text edits or destroy Redo. Keep unsupported visual edits explicitly bounded.
4. **Theme compatibility and resource handling.** Check imports, same-name themes, folder changes, selection persistence, local fonts/imports, missing assets and error feedback. Review resource containment, size limits, raw HTML handling and blocked remote assets. Compatibility with arbitrary Typora CSS is not established.
5. **Rendering and sustained use.** Stress large documents, repeated edits, theme/zoom/divider changes, missing images, many windows and extended sessions. Measure Chromium/PDF job count, cache/temp-file lifetime, memory and responsiveness. Verify PDF page breaks/tables/code and physical printing separately.
6. **Input and accessibility.** Physical keyboard/mouse/trackpad, IME, VoiceOver, focus/F6 recovery, inactive windows, fullscreen, tab detach, multiple displays and scaling remain acceptance work. Historical activation-blocked workflows must not be silently counted as passing.
7. **Maintainability and test quality.** Review coupling in the large backend/Main.qml and cycle-based `.inc` organization. Suggest focused abstractions only where they simplify ownership or make failures testable. Look for tests that mirror implementation, force helpful rendering, or assert counts/state without user-visible behavior.
8. **Packaging and release readiness.** Verify fresh-machine installation and dependency deployment; consider reproducible CI and a compact release gate. Developer ID signing/notarization, public release packaging and full competitor parity remain separate unfinished tracks.

Known scope limits: complex/nested Visual Edit operations still fall back to Source; Code has lightweight lexers rather than language services; math is not typeset; arbitrary raw HTML is escaped; Typora editor/UI selectors are not reproduced; remote theme assets are blocked. Saved PDF export is vector, while native rendered printing uses 300 dpi page images. These are explicit design/implementation limits, not promises of complete parity.

A minimum manual preview matrix should include two unmistakably different documents in the same folder; rapid A/B/C selection while a PDF is pending; Web→PDF→Web and same-cached-PDF return; theme changes; hide/show or minimize/restore; editing while rendering; timeout/retry; missing images; and normal quit/relaunch. Record app path, version, theme, mode, selected file, visible heading and exact reproduction steps.

## 10. Suggested brief to send with this file

> Please independently review Fomawrite at the repository and baseline above. Start with the latest publishing-preview regressions and their tests, then assess document integrity, theme handling, native workflows and maintainability. Build and run the applicable checks; distinguish recorded historical results from your own observations. Use disposable fixtures and preserve existing user files/settings. For each finding, give severity, source file/line, reproduction, expected versus actual behavior, proposed fix and a regression test that would catch it. Separate confirmed defects from hypotheses and optional improvements. Recommend a short prioritized next cycle rather than a broad rewrite.

Further references: [iA Writer inventory](docs/ia-writer-inventory.md), [menu audit](docs/ia-menu-audit-2026-09-23.md), [editor redesign](docs/editor-surface-redesign.md), [roadmap](docs/roadmap.md), [acceptance ledger](docs/release-acceptance.md), and [cycle log](docs/build-cycles.md).

Licenses to preserve: [MIT](LICENSE), [iA Writer Mono SIL OFL](fonts/OFL.txt), and [MD4C MIT](src/vendor/md4c/LICENSE.md). Personal reference documents and themes should not be added to the public repository merely to support a review.
