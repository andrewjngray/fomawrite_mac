# Fomawrite — architecture

Living document. Keep it current when a cycle moves responsibilities between files, adds a module or changes one of the models below (see [AGENTS.md](AGENTS.md)). It replaces §5 and §7 of the dated [DEVELOPER_HANDOFF.md](DEVELOPER_HANDOFF.md) and incorporates the corrections from [INDEPENDENT_REVIEW.md](INDEPENDENT_REVIEW.md) §4. Current build and open items: [STATUS.md](STATUS.md).

## 1. Stack and shape

C++17, Qt Quick/QML, Qt Widgets/native integration, Qt WebEngine and Qt PDF; qmake project [fomawrite.pro](fomawrite.pro) (app) and [tests/tests.pro](tests/tests.pro) (QtTest executable, which compiles the same sources except `main.cpp`, `documentviewcheck.cpp` and `systemtheme_mac.cpp`). Development uses Homebrew Qt 6.11.2 on Apple Silicon; CI pins Qt 6.11.3. The macOS deployment target is 14.0. The Linux path (`systemtheme.cpp`, D-Bus portal) is retained in the `.pro` but nothing exercises it since the WebEngine/PDF work; assume it does not compile until proven otherwise.

Canonical Markdown belongs to the C++ document backend. QML owns UI state and editing interaction. The left pane edits (Source, or the Live editor page); the right pane is read-only publishing output (Web or PDF) generated independently from the source. Output styling never changes the Markdown or the left editor's appearance.

What Fomawrite is, in one line: an iA-Writer-style Source editor plus a Typora-style Live editor (one CodeMirror 6 page with presentation modes) plus a Marked-2-style publishing pane that accepts Typora CSS themes. The bounded Visual Edit projection and its Source↔Visual mapper were retired in Cycle 135; Live covers that ground with the Markdown text itself as the editing model.

## 2. Source map

| Concern | Primary files | Notes |
| --- | --- | --- |
| App/window lifecycle, restore, launch routing | [main.cpp](src/main.cpp), [workspace.cpp](src/workspace.cpp) / [.h](src/workspace.h) | One process; one `Backend` + QML engine per window; local-socket launch forwarding; workspace checkpoints keyed by executable path. `main()` also splices test harness code under `#ifdef` (see §6). |
| Canonical document, I/O, history, recovery, preferences, output styles and ~15 other responsibilities | [backend.h](src/backend.h), [backend.cpp](src/backend.cpp) (~4.1k lines) | Still the god object the review scores against, minus publishing (extracted in Cycle 133) and the dead word-count/close-after-save paths. `FileLibrary`, `WorkspaceCommands` and now `Publisher` are the pattern to follow for the remaining extractions (output styles, document analysis, Markdown editing, authorship, writing review). |
| Publishing: theme catalog and folder watching, memoized sanitized CSS, HTML/PDF generation, per-consumer async preview protocol and cache | [publisher.h](src/publisher.h), [publisher.cpp](src/publisher.cpp) | A real module since Cycle 133 (it replaced the `#include`d `backendpublishing.inc`). `Publisher` reads the document and output settings only through the `PublishingSource` interface that `Backend` implements, so publishing state cannot reach editing state. `Backend` keeps one-line forwarders (`backend.requestPublishingPreview`, `backend.publishingThemeId`, …) and relays `Publisher`'s signals under their old names; it also exposes `backend.publisher` for new callers. |
| Top-level UI and commands | [Main.qml](src/Main.qml) (~3.2k lines), [NativeMenuBar.qml](src/NativeMenuBar.qml) (~0.5k), [WorkspaceCommands.qml](src/WorkspaceCommands.qml) | The native macOS menu bar now lives in `NativeMenuBar.qml` (Cycle 133); it is instantiated from `Main.qml`'s `Loader`, so it still resolves `win`, `backend`, the dialogs and panes through that context. `Main.qml` still holds the source editor (~800 lines), the dialogs, timers, and Markdown logic that belongs in C++ (`smartReturn`, `visualDiff`, link escaping; the [LinkSyntax.js](src/LinkSyntax.js) grammar is duplicated by a C++ regex kept in sync by hand). Remaining planned extractions: `SourceEditor.qml`, `WorkspaceSettings.qml`, one file per dialog. |
| Pane geometry, headers/footers, visibility | [WorkspaceLayout.qml](src/WorkspaceLayout.qml), [WorkspaceHeader.qml](src/WorkspaceHeader.qml), [DocumentFooter.qml](src/DocumentFooter.qml), [WorkspaceFooter.qml](src/WorkspaceFooter.qml) | Left footer: Source / Live + writing appearance + editor zoom. Right footer: Web / PDF + Output Style + publishing zoom. Single / Split is a separate group. |
| **Live** editing surface (Cycle 134) | [LiveEditorPane.qml](src/LiveEditorPane.qml), [editorbridge.h](src/editorbridge.h)/[.cpp](src/editorbridge.cpp), [src/editor/](src/editor/README.md) (TypeScript, CodeMirror 6, esbuild → committed `dist/editor.js`) | A CodeMirror 6 Markdown editor page hosted in a `WebEngineView` (`qrc:/editor/index.html`, strict CSP) and connected over `QWebChannel` through `EditorBridge`. While Live is active the page's document is canonical and `Backend::applyLiveChanges` mirrors its `{from,to,insert}` change lists into the `QTextDocument` byte-exactly; `Backend::forwardLiveChange` mirrors C++-side edits back. Presentation modes: Source (Manuscript/Editorial/Book/Code appearances, focus, typewriter) and Live (markers hidden until the caret enters; images via the host, task checkboxes, rules, highlighted fences, table grids, KaTeX math), themed by the same `#write` CSS as published output. Created lazily on first use via a `Loader`, then kept. Selected by `workspaceLayout.liveEditEnabled` / command `liveEditing`. |
| **Right** publishing pane (read-only output) | [PublishingPreview.qml](src/PublishingPreview.qml) | Instantiated in `Main.qml` as **`previewPane`**; in `ExportHub.qml` as `exportHubPreviewPane`. See §5. |
| Source syntax/structure | [markdownhighlighter.cpp](src/markdownhighlighter.cpp), [markdownextensions.cpp](src/markdownextensions.cpp), [EditorMutations.js](src/EditorMutations.js) | Source always shows all syntax; four appearances (Manuscript, Editorial, Book, Code). |
| Semantic HTML / browser PDF | [publishinghtml.cpp](src/publishinghtml.cpp), [publishingpdf.cpp](src/publishingpdf.cpp), [MD4C 0.5.3](src/vendor/md4c/README.md) | MD4C → HTML; raw HTML escaped; `javascript:`/`data:` hrefs rejected; images inlined as `data:` URLs through `PublishingHtml::ImageCache`; Chromium renders Web and PDF for the pane, Export Hub and files. |
| CSS catalog, imports/assets/fonts; basic settings | [publishingthemes.cpp](src/publishingthemes.cpp), [outputcss.cpp](src/outputcss.cpp), [claude-like.css](src/themes/claude-like.css) | Folder-discovered `.css` themes with local resources; CSS sanitiser decodes escapes and rejects `\`, `<`, remote `url()` and `@import`. Basic font/page presets (`OutputCss`) are separate from CSS themes. |
| Export UI | [ExportHub.qml](src/ExportHub.qml) | Reuses `PublishingPreview`. |
| Library/organizer | [LibraryPane.qml](src/LibraryPane.qml), [OrganizerPane.qml](src/OrganizerPane.qml), [filelibrary.cpp](src/filelibrary.cpp) | `FileLibrary` is the clean-module reference. |
| Zoom/find/outline/links | [PaneZoomState.qml](src/PaneZoomState.qml), [DocumentFindBar.qml](src/DocumentFindBar.qml), [DocumentOutline.qml](src/DocumentOutline.qml), [LinkEditor.qml](src/LinkEditor.qml), [LinkSyntax.js](src/LinkSyntax.js) | |
| macOS behaviour and bundle metadata | [macbridge.h](src/macbridge.h), [windowchrome_mac.mm](src/windowchrome_mac.mm), [systemtheme_mac.cpp](src/systemtheme_mac.cpp), [Info.plist](macos/Info.plist) | Every native function is declared once in `macbridge.h` (Cycle 133), included by callers and by the `.mm` file itself, so a signature change is a compile error rather than a link or runtime error. |
| Embedded QML/assets/fonts | [resources.qrc](src/resources.qrc), `fonts/`, `macos/` | The acceptance checker compares the bundle's embedded QML hashes with the checkout. |
| Tests | [tests.pro](tests/tests.pro), [tst_fomawrite.cpp](tests/tst_fomawrite.cpp), `tests/*.inc` | One translation unit; see §6. |
| Normal-startup / restoration harness | [bin/check-publishing-startup](bin/check-publishing-startup), [startup-preview-check.inc](tests/startup-preview-check.inc), [startup-preview-setup.inc](tests/startup-preview-setup.inc) | Rebuilds the app with `-DFOMAWRITE_STARTUP_PREVIEW_CHECK`. |
| Actual-bundle verification | [bin/check-document-views](bin/check-document-views), [documentviewcheck.cpp](src/documentviewcheck.cpp), `src/*acceptancecheck.inc` | Ships inside the product binary; see §6. |

Coupling to know about: C++→QML goes mostly through `findChild<>("objectName")` (hundreds of sites) and `invokeMethod`; QML binds widely to `backend.palette` (a `QVariantMap` rebuilt on theme change); `QSettings` keys are written at ~50 call sites with no registry; output styles are compared as magic integers in places.

## 3. Persistence model (since Cycle 131)

Bytes in and out are preserved exactly unless the user is told otherwise.

- **Open** reads raw bytes (no `QIODevice::Text`). `Backend::decodeDocumentBytes` validates UTF-8 with `QStringDecoder` and recognises UTF-16 byte-order marks. The dominant line ending is recorded in `m_lineEnding`, a UTF-8 BOM in `m_hadByteOrderMark`, and a lossy decode in `m_lossyDecode`.
- **In-memory text** is always LF. `currentDocumentText()` derives from `QTextDocument::toRawText()` and replaces only paragraph separators with `\n`, so no-break spaces and U+2028 survive; offsets equal `toPlainText()`'s, so the Source↔Visual mapping is unaffected. Both sides of the modified-state comparison use this canonical text.
- **Save / duplicate** go through `encodeDocumentText`, which re-applies the file's BOM and line-ending style, then `QSaveFile` (atomic). The disk baseline holds the real bytes, so autosave, Rename, Move and the external-change watcher compare like for like.
- **Lossy input** (Latin-1, UTF-16) is displayed but Save, autosave and quit-save refuse to overwrite the original; the status says why and points at Save As, which writes UTF-8 and clears the flag.
- **Every same-document save** (`saveTo`, not just autosave) re-reads the on-disk bytes and compares them with the baseline. A mismatch cancels the staged write and raises the **File changed** dialog, which has a **Save Anyway** option. Recovery snapshots carry the line-ending, BOM and lossy flags so a relaunch restores the same conventions; `restoreVersion` normalises historical line endings before insertion.
- Known limits: U+2029 in a document becomes a paragraph break; mixed endings are normalised to the dominant style on save; UTF-16 is never written back; the mapper's CRLF branches in `sourcevisualmapping.cpp` are unreachable from the live app.

Regression fixtures: [tests/cycle131-persistence.inc](tests/cycle131-persistence.inc) (byte round-trips, lossy refusal, CRLF workflow, manual-save baseline) plus the extended `recoverySnapshotsSurviveProcessExit`.

## 4. Publishing model (since Cycle 132)

Request flow: debounce in `PublishingPreview.qml` → `Backend::publishingHtml` (body + memoized CSS + custom/print CSS + inlined images, returning a **fingerprint**) → cache lookup → `PublishingPdf` (Chromium) or direct Web load → result bound to the requesting document/settings generation → displayed or rejected as stale.

- **Two signals, two meanings.** `publishingThemesChanged` means the *catalog* (menu listing, selected id/name/error) changed; menus rebuild, nothing is cancelled. `publishingCssChanged` means the *selected theme's sanitized CSS* (or its error) changed; it is the only path that bumps the settings generation, cancels in-flight renders and reloads the preview. `refreshPublishingThemes` compares the catalog listing and the selected CSS, not a folder snapshot, so unrelated files in the themes folder (`.DS_Store`, editor swap files, edits to an unselected theme) emit nothing. Explicit Reload Themes forces both.
- **Memoized CSS.** `updatePublishingCss` / `selectedPublishingCss` compute the selected theme's sanitized CSS (fonts base64-embedded, up to 64 MiB) once per folder state and keep its hash; the watcher invalidates it. Decoded images are cached per path in `PublishingHtml::ImageCache` while size and mtime are unchanged.
- **Cache key.** The request cache key is a SHA-256 over a small fingerprint of the inputs (body, CSS hash, custom CSS, print CSS, title, image stat signature, warning), computed in `publishingHtml`, not over the megabytes of generated HTML. Identical HTML from two different sources still gets distinct identity through the document token.
- **Watcher.** Entries are sorted before the 511-path cap; watches cover directories and top-level `.css` files; the content snapshot hashes CSS and records metadata for fonts/images. A refresh never creates the themes folder; if it is moved aside the parent is watched until it returns (Open Themes Folder / Import still create it). Fonts/images edited strictly in place with size, mtime and birth time preserved are not detected.
- **Chromium.** One off-the-record profile per `PublishingPdf` (no cache, cookies or storage); still one page — and therefore one renderer process — per PDF job so cancellation is real. The preview `WebEngineView` has `localContentCanAccessFileUrls: false` (all assets are `data:` URLs).
- **Failure keeps output.** `failRefresh` keeps the last output that belongs to the *current* document behind the notice; output from another document is still cleared. The QML deadline (35 s) is longer than the renderer's own 30 s so the renderer's error is the one reported. `onRenderProcessTerminated` reports and reloads. Web completion is driven by load success; `webOutputUrl` sets `web.url` imperatively and an unchanged URL forces `reload()`.
- Still open: `PublishingPreview.qml` keeps ~15 mutable fields and callback guards instead of an explicit `Idle | Debouncing | Requested | Loading | Displayed | Failed` state machine; HTML assembly and fingerprinting run on the GUI thread; Web mode has one component test.

Regression fixtures: [tests/cycle132-publishing.inc](tests/cycle132-publishing.inc), [cycle128-publishing.inc](tests/cycle128-publishing.inc), [cycle124-publishing.inc](tests/cycle124-publishing.inc), [cycle125-theme-folder.inc](tests/cycle125-theme-folder.inc).

## 5. The two editing surfaces and the publishing pane

| | Left: Source | Left: Live | Right: publishing output |
| --- | --- | --- | --- |
| File | `Main.qml` (`editorPane` / `sourceEditor`) | `src/LiveEditorPane.qml` | `src/PublishingPreview.qml` |
| Instance / `objectName` | `editor` / `"sourceEditor"` | `liveEditorLoader.item` / `"liveEditorPane"` (view `"liveEditorView"`) | `previewPane` / `"previewPane"` (`"exportHubPreviewPane"` in Export Hub) |
| Edits the document | Yes, directly | Yes: page change lists mirrored byte-exactly through `EditorBridge` / `Backend::applyLiveChanges` | Never |
| Content | The Markdown text, highlighted | The same text in a CodeMirror 6 page with markers hidden until the caret enters | Chromium Web page or PDF pages generated from the Markdown |

The retired Visual Edit projection (`VisualEditPane.qml`, `sourcevisualmapping.*`, `visualtexthighlighter.*`) lived here until Cycle 135; a saved workspace that still asks for Visual Edit is restored as Live.

## 6. The three `.inc` conventions

None of these is a header. All three are raw text splices and all three are candidates for the structural pass.

1. **Product code split by `#include`** — *gone since Cycle 133*: `src/backendpublishing.inc` became the `Publisher` class in `src/publisher.{h,cpp}`.
2. **Acceptance-check bodies inside the product binary** — `src/editoracceptancecheck.inc`, `panechromeacceptancecheck.inc`, `editinglayoutacceptancecheck.inc`, `publishingthemeacceptancecheck.inc` (which itself includes `publishingfolderacceptancecheck.inc` and `publishingpreviewacceptancecheck.inc`) are `#include`d *inside function bodies* of `src/documentviewcheck.cpp`, which is in the app's `SOURCES` and ships in every bundle. `bin/check-document-views` drives it against an actual `.app`. Target: a separate `fomawrite-viewcheck` executable.
3. **Test slots and harnesses** — `tests/cycle*.inc` are QtTest slot bodies `#include`d into the single translation unit `tests/tst_fomawrite.cpp` (every test links WebEngine; files are named by the cycle that added them, not by feature). Separately, `tests/startup-preview-setup.inc`, `startup-preview-check.inc`, `window-routing-smoke.inc`, `cycle103-migration-seed.inc` and `cycle103-migration.inc` are `#include`d into `src/main.cpp` under `#ifdef FOMAWRITE_STARTUP_PREVIEW_CHECK` / `FOMAWRITE_CONTEXT_SMOKE`; `bin/check-publishing-startup` and `bin/test-window-routing` rebuild the whole app into a temporary directory with those defines.

The editor page has its own test suite (`cd src/editor && npm test`, node:test, DOM-free) beside the QtTest suite.

Planned end state (review §8): `src/` as a static library; tests split by module into fast-core and slow-QML executables; `cycle*.inc` renamed by feature. **Deliberately not done in Cycle 133:** moving `documentviewcheck.cpp` out of the product. `bin/check-document-views` runs `--check-document-views` *inside the shipped bundles* (Dev, packaged and installed) precisely to verify the real embedded QML and signatures; a separate executable would verify a different binary. The acceptance code therefore stays in the app until that verification model is redesigned. The static-library/test-split restructure was also deferred so the first CI runs exercise an unchanged build.

## 7. Build, test, packaging and checks

Requirements on macOS: Apple Command Line Tools and Qt 6 with WebEngine (`brew install qtbase qtdeclarative qttools qtwebengine`; `qtwebengine` supplies QtPdf). [bin/qt-env](bin/qt-env) discovers `qmake6`/`qmake`; `QMAKE=/path/to/qmake` overrides it; `JOBS` sets parallelism.

```sh
./bin/build                         # qmake + make into build/Fomawrite.app (regenerates Info.plist, copies licences)
./bin/test                          # builds build-tests/tst_fomawrite and runs it with QT_QPA_PLATFORM=offscreen
./bin/test <QtTestFunctionName>     # one focused regression
./bin/run examples/Welcome.md       # run the built app on a document
```

Packaging is separate from building; close any copy being replaced first (preserving unsaved work) and never replace the running editor:

```sh
./bin/package-mac                   # dist/Fomawrite.app, deployed Qt, ad-hoc signed (hand-rolled otool/install_name_tool loop)
./bin/prepare-dev-app               # dist/Fomawrite Dev.app, stable QA identity io.github.andrewjngray.fomawrite.dev
./bin/install                       # copy the packaged app to /Applications
./bin/make-app-icon                 # regenerate .icns/PNGs from macos/*.svg (needs QtSvg and iconutil)
./bin/archive-mac-release           # zip + manifest + SHA-256 for a release candidate
```

Native checks need a logged-in desktop session and a fresh output directory:

```sh
./bin/check-publishing-startup /tmp/fw-startup                                        # cold + restored normal startup with populated themes
./bin/check-document-views /tmp/fw-preview --preview-only --app "dist/Fomawrite Dev.app"
./bin/check-document-views /tmp/fw-full --app "dist/Fomawrite Dev.app"
./bin/test-window-routing                                                             # lifecycle/activation harness (not recertified since 128)
```

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) runs `./bin/build` and `./bin/test` on a macOS runner with pinned Qt; it cannot run the native checks. Bundles are ad-hoc signed, not notarized; the version string lives in both `src/main.cpp` (`setApplicationVersion`) and `macos/Info.plist`.

## 8. Data on disk (macOS)

| Location | Contents |
| --- | --- |
| `~/Library/Application Support/AndrewGray/fomawrite/` | Workspace checkpoints, recovery files and locks, migration marker, `output-user-styles.json` (basic output settings) |
| `…/fomawrite/publishing-themes/` | Managed CSS themes and their resource folders (shared by every app copy, including Dev) |
| Qt settings under the app's organization/domain | Preferences; diagnostic checks use disposable settings |
| `.<filename>.omawrite-authors.json` beside a document | Authorship sidecar (legacy name kept for compatibility) |

Workspace filenames include an identity derived from the executable path, so copies at different paths restore different workspaces, but themes are shared. Do not delete recovery files or overwrite a running app to troubleshoot preview state.
