# Fomawrite — architecture

Living document. Keep it current when a cycle moves responsibilities between files, adds a module or changes one of the models below (see [AGENTS.md](AGENTS.md)). It replaces §5 and §7 of the dated [DEVELOPER_HANDOFF.md](DEVELOPER_HANDOFF.md) and incorporates the corrections from [INDEPENDENT_REVIEW.md](INDEPENDENT_REVIEW.md) §4. Current build and open items: [STATUS.md](STATUS.md).

## 1. Stack and shape

C++17, Qt Quick/QML, Qt Widgets/native integration, Qt WebEngine and Qt PDF; qmake project [fomawrite.pro](fomawrite.pro) (app) and [tests/tests.pro](tests/tests.pro) (QtTest executable, which compiles the same sources except `main.cpp`, `documentviewcheck.cpp` and `systemtheme_mac.cpp`). Development uses Homebrew Qt 6.11.2 on Apple Silicon; CI pins Qt 6.11.3. The macOS deployment target is 14.0. The Linux path (`systemtheme.cpp`, D-Bus portal) is retained in the `.pro` but nothing exercises it since the WebEngine/PDF work; assume it does not compile until proven otherwise.

Canonical Markdown belongs to the C++ document backend. QML owns UI state and editing interaction. The left pane edits (Source, or the bounded Visual Edit projection); the right pane is read-only publishing output (Web or PDF) generated independently from the source. Output styling never changes the Markdown or the left editor's appearance.

What Fomawrite is, in one line: an iA-Writer-style Source editor plus a bounded Visual Edit projection plus a Marked-2-style publishing pane that accepts Typora CSS themes. It is not an inline-WYSIWYG editor; the mapper is "not a Markdown serializer" by design ([sourcevisualmapping.h](src/sourcevisualmapping.h)).

## 2. Source map

| Concern | Primary files | Notes |
| --- | --- | --- |
| App/window lifecycle, restore, launch routing | [main.cpp](src/main.cpp), [workspace.cpp](src/workspace.cpp) / [.h](src/workspace.h) | One process; one `Backend` + QML engine per window; local-socket launch forwarding; workspace checkpoints keyed by executable path. `main()` also splices test harness code under `#ifdef` (see §6). |
| Canonical document, I/O, history, recovery, preferences and ~18 other responsibilities | [backend.h](src/backend.h), [backend.cpp](src/backend.cpp) (~4.0k lines) + [backendpublishing.inc](src/backendpublishing.inc) (~0.5k) | The god object the review scores against: 27 `Q_PROPERTY`, 28 signals, ~110 members. `FileLibrary` and `WorkspaceCommands` are the correctly-factored pattern to follow when extracting from it (`backend.publisher` is the first planned extraction). |
| Publishing requests, cache, identity, export/print integration | [backendpublishing.inc](src/backendpublishing.inc) | **Not a module.** It is `#include`d near the end of `backend.cpp` as a file-length split: it defines `Backend::` members, shares all of `Backend`'s private state, and its anonymous-namespace helpers cannot be unit-tested on their own. It is listed in `DISTFILES`, not `SOURCES`. |
| Top-level UI and commands | [Main.qml](src/Main.qml) (~3.7k lines), [WorkspaceCommands.qml](src/WorkspaceCommands.qml) | `Main.qml` holds the native menu bar (~500 lines, no logic), the source editor (~800 lines), 24 dialogs, 11 timers, and Markdown logic that belongs in C++ (`smartReturn`, `visualDiff`, link escaping; the [LinkSyntax.js](src/LinkSyntax.js) grammar is duplicated by a C++ regex kept in sync by hand). Planned mechanical extractions: `NativeMenuBar.qml`, `SourceEditor.qml`, `WorkspaceSettings.qml`, one file per dialog. |
| Pane geometry, headers/footers, visibility | [WorkspaceLayout.qml](src/WorkspaceLayout.qml), [WorkspaceHeader.qml](src/WorkspaceHeader.qml), [DocumentFooter.qml](src/DocumentFooter.qml), [WorkspaceFooter.qml](src/WorkspaceFooter.qml) | Left footer: Source / Visual Edit + writing appearance + editor zoom. Right footer: Web / PDF + Output Style + publishing zoom. Single / Split is a separate group. |
| **Right** publishing pane (read-only output) | [PublishingPreview.qml](src/PublishingPreview.qml) | Instantiated in `Main.qml` as **`previewPane`**; in `ExportHub.qml` as `exportHubPreviewPane`. See §5. |
| **Left** Visual Edit projection | [PreviewPane.qml](src/PreviewPane.qml), [sourcevisualmapping.cpp](src/sourcevisualmapping.cpp), [visualtexthighlighter.cpp](src/visualtexthighlighter.cpp) | Instantiated in `Main.qml` as **`visualEditorPane`**. **Naming trap:** despite its filename, `PreviewPane.qml` is the *editor*, and both it and `PublishingPreview.qml` declare `objectName: "previewPane"`. Trace the bindings; do not infer ownership from the filename. Planned rename: `PreviewPane.qml → VisualEditPane.qml`. |
| Source syntax/structure | [markdownhighlighter.cpp](src/markdownhighlighter.cpp), [markdownextensions.cpp](src/markdownextensions.cpp), [EditorMutations.js](src/EditorMutations.js) | Source always shows all syntax; four appearances (Manuscript, Editorial, Book, Code). |
| Semantic HTML / browser PDF | [publishinghtml.cpp](src/publishinghtml.cpp), [publishingpdf.cpp](src/publishingpdf.cpp), [MD4C 0.5.3](src/vendor/md4c/README.md) | MD4C → HTML; raw HTML escaped; `javascript:`/`data:` hrefs rejected; images inlined as `data:` URLs through `PublishingHtml::ImageCache`; Chromium renders Web and PDF for the pane, Export Hub and files. |
| CSS catalog, imports/assets/fonts; basic settings | [publishingthemes.cpp](src/publishingthemes.cpp), [outputcss.cpp](src/outputcss.cpp), [claude-like.css](src/themes/claude-like.css) | Folder-discovered `.css` themes with local resources; CSS sanitiser decodes escapes and rejects `\`, `<`, remote `url()` and `@import`. Basic font/page presets (`OutputCss`) are separate from CSS themes. |
| Export UI | [ExportHub.qml](src/ExportHub.qml) | Reuses `PublishingPreview`. |
| Library/organizer | [LibraryPane.qml](src/LibraryPane.qml), [OrganizerPane.qml](src/OrganizerPane.qml), [filelibrary.cpp](src/filelibrary.cpp) | `FileLibrary` is the clean-module reference. |
| Zoom/find/outline/links | [PaneZoomState.qml](src/PaneZoomState.qml), [DocumentFindBar.qml](src/DocumentFindBar.qml), [DocumentOutline.qml](src/DocumentOutline.qml), [LinkEditor.qml](src/LinkEditor.qml), [LinkSyntax.js](src/LinkSyntax.js) | |
| macOS behaviour and bundle metadata | [windowchrome_mac.mm](src/windowchrome_mac.mm), [systemtheme_mac.cpp](src/systemtheme_mac.cpp), [Info.plist](macos/Info.plist) | The C++↔Objective-C++ bridge is reached through function-local `extern` declarations scattered across files (no `macbridge.h` yet). |
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

## 5. The two "preview" panes

| | Left: Visual Edit | Right: publishing output |
| --- | --- | --- |
| File | `src/PreviewPane.qml` | `src/PublishingPreview.qml` |
| Instance id in `Main.qml` | `visualEditorPane` | `previewPane` |
| `objectName` | `"previewPane"` | `"previewPane"` (`"exportHubPreviewPane"` in Export Hub) |
| Edits the document | Yes, through `SourceVisualMapping` (paragraph, heading, list item, image alt, table cell; everything else is `SourceOnly`) | Never |
| Content | `QTextDocument` projection of the Markdown | Chromium Web page or PDF pages generated from the Markdown |

## 6. The three `.inc` conventions

None of these is a header. All three are raw text splices and all three are candidates for the structural pass.

1. **Product code split by `#include`** — `src/backendpublishing.inc` into `backend.cpp`. One class, two files; the include is a length workaround, not a boundary.
2. **Acceptance-check bodies inside the product binary** — `src/editoracceptancecheck.inc`, `panechromeacceptancecheck.inc`, `editinglayoutacceptancecheck.inc`, `publishingthemeacceptancecheck.inc` (which itself includes `publishingfolderacceptancecheck.inc` and `publishingpreviewacceptancecheck.inc`) are `#include`d *inside function bodies* of `src/documentviewcheck.cpp`, which is in the app's `SOURCES` and ships in every bundle. `bin/check-document-views` drives it against an actual `.app`. Target: a separate `fomawrite-viewcheck` executable.
3. **Test slots and harnesses** — `tests/cycle*.inc`, `tests/sourcevisualmapping-cycle99.inc` are QtTest slot bodies `#include`d into the single translation unit `tests/tst_fomawrite.cpp` (every test links WebEngine; files are named by the cycle that added them, not by feature). Separately, `tests/startup-preview-setup.inc`, `startup-preview-check.inc`, `window-routing-smoke.inc`, `cycle103-migration-seed.inc` and `cycle103-migration.inc` are `#include`d into `src/main.cpp` under `#ifdef FOMAWRITE_STARTUP_PREVIEW_CHECK` / `FOMAWRITE_CONTEXT_SMOKE`; `bin/check-publishing-startup` and `bin/test-window-routing` rebuild the whole app into a temporary directory with those defines.

Planned end state (review §8, Cycle 133): `src/` as a static library; tests split by module into fast-core and slow-QML executables; `cycle*.inc` renamed by feature; no test code in the product binary.

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
