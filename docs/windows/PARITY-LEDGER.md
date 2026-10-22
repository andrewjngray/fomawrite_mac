# Windows parity ledger

22 October 2026. This records ongoing work toward running all applicable Mac regressions on Windows. It does not close desktop parity or qualify a package. Exact candidate/CI identities remain in [STATUS.md](STATUS.md); historical M1 skip totals remain historical.

## Capability exclusions

| Boundary | What may differ or remain excluded | Required portable coverage |
| --- | --- | --- |
| Native macOS checker | NSSpellChecker availability/languages, native-provider findings and provider-specific timing | Actual bundled Harper spelling, grammar, Review, corrections, exclusions and UTF-16 offsets. The 28 common cases now have real-engine Windows rows; native Mac rows remain. [Checker evidence](M2-CHECKER-PARITY.md), [Source fixtures](../../tests/cycle143-source-spelling.inc), [Review fixtures](../../tests/cycle144-review-pane.inc). |
| Cocoa integration | Native menu roles/topology, NSFileVersion, Share/Speech, AppKit tab/window integration | Truthful Windows capability policy and preservation of common document state. The Versions test bodies are Mac-conditional; their Windows function result does not establish version-history support. [Tests](../../tests/tst_fomawrite.cpp), [native menu](../../src/NativeMenuBar.qml). |
| Platform rendering/filesystem semantics | Platform colours/fonts and documented QFile differences | Windows native appearance checks, actual fallback rendering, exact document bytes and filesystem safety. Symlink privilege or watcher unavailability is an environment limitation requiring its own recorded reason, not a blanket Windows exclusion. [Test matrix](TEST-MATRIX.md). |

The complete Windows suite runs without a fixed platform skip list. The old `nativeWorkspaceMenusShareState` skip covered **portable behavior** and is removed in the current candidate. Shared Mac CI's 13 skips are 12 unavailable native-checker grammar rows and one Windows-only appearance test; the corresponding bundled-Harper rows run. These are not Windows feature exclusions.

Mac-conditional assertions need separate treatment: native checker details in `writingReviewExcludesCodeAndCorrectsSafely` and the two NSFileVersion test bodies are genuine capability boundaries. OS-specific colour expectations are legitimate substitutions. Recent-menu activation, toolbar metrics, chrome, Focus and Output Style are now implemented in the Windows candidate, with actual-menu assertions replacing the former absence/shared-command fallbacks. Their native Windows qualification is still pending. Cycles 159–161 replace the remaining Format/authorship/Center limitations and add the five missing shortcut registrations below. Their focused host checks pass; Windows-only key and geometry execution plus ordinary packaged input remain separate qualification.

## Implemented View and clipboard/search slices

Candidate `7c37078b0d135f78b68f412444083ae7f919b339` restores the complete `nativeWorkspaceMenusShareState` assertions using platform menu lookup, plus these 15 Windows View commands and Paginated Preview:

- `sortBar`, `filterBar`, `excerpts`;
- `sortName`, `sortModified`, `sortCreated`, `sortExtension`;
- `ascending`, `descending`, `foldersFirst`;
- `dateNone`, `dateModified`, `dateCreated`;
- `navigationTree`, `navigationList`.

The same candidate implements nine clipboard/search actions: Copy Markdown/HTML/Formatted, Paste As Plain Text/Markdown from HTML, Delete and Find Next/Previous/Selection. `windowsClipboardSearchMenusPreserveFocusAndUndo`, the existing File/Edit regression and search fixtures exercise actual Windows menu routes, Source/Live selection, auxiliary-field isolation and Undo.

The required local build and complete suite passed **367 tests, zero failures and one Windows-only skip**, 540.931 seconds. Exact native CI passed ARM64 340/0/0 but failed x64 339/1/0; shared Mac Qt failed 352/3/13. The confirmed Save defect and fixture corrections are detailed in the Windows status. Ordinary packaged desktop menu checks remain incomplete. Qualification still requires menu activation, checked/enabled state, date/sort independence, responsive geometry and unchanged document bytes/history. [Workspace/menu tests](../../tests/tst_fomawrite.cpp), [platform lookup](../../tests/platform-menu-helpers.inc), [Windows menu tests](../../tests/windows-menu.inc), [Windows menu](../../src/WindowsMenuBar.qml).

## Implemented cycles 154–158 — native qualification pending

These routes exist in the current integration source; they are no longer missing menu implementations. Host component/menu-signal tests and real Qt pointer tests establish the bounded behavior in their records. They do not establish ordinary packaged Windows input, complete desktop parity or promotion. Exact candidate/CI failures and remaining gates are recorded in [Windows status](STATUS.md).

| Cycle | Implemented Windows routes or repair | Coverage boundary |
| --- | --- | --- |
| [154 — Focus/chrome](../../research/cycle-154/README.md) | Sentence/Paragraph/Typewriter, Fillers/Custom style checks, Writing Review, Title Bar and all Toolbar/Stats Only metric controls. | Five existing cases require actual Windows nodes, guards, checked state and persistence. Complete host suite passed; exact Focus/chrome Windows CI has a stale chrome-absence assertion, corrected in integration. Native reruns and ordinary interaction remain pending. |
| [155 — Go/Open Recent](../../research/cycle-155/README.md) | File → Open Recent and Smart Folders → Recents; document/Library history, Enclosing Folder, Open Link, Quick Search, Command Palette, Locations/Add Location, saved/new Smart Folders and Hashtags/refresh. | Three existing cases require actual routes and retain unsaved/open/root guards. Native menu proof, picker and pointer follow-through remain pending. |
| [156 — Output Style/Live theme](../../research/cycle-156/README.md) | All eight basic styles, dynamic Custom Themes, settings edit/load, theme import/folder/reload/gallery, and Live Follows Theme Exactly. | Actual Windows nodes, footer synchronization, repeated/rejected selection checks and existing Live guard; host old-menu red/green passed. Native Windows qualification remains pending. |
| [157 — Edit/Help](../../research/cycle-157/README.md) | All four case transformations, Smart Quotes/Smart Dashes, and What's New in Fomawrite. | Four existing cases retain native Mac rows and require actual Windows menu signals; host missing-route red/green passed. Physical nested-menu and packaged input remain pending. |
| [158 — Immediate Save](../../research/cycle-158/README.md) | Windows menus close immediately so their exit transition cannot swallow the next Ctrl+S. | Real Bold pointer → Save shortcut host red/green covers settled/immediate Live and immediate Source with exact disk bytes and clean state. The unchanged zero-gap Win32 input sequence still requires final-package qualification. |

The same behavior may have a different menu label or grouping. Title Bar → Fade In/Out and Always Show already control the native menu's Auto-Hide Document Bars setting; the registry's Insert Page Break uses the same snippet as native Add Page Break. Neither is a remaining behavior gap. All `NativeCommand` IDs in the native menu now have Windows entries except the intentionally unavailable `engineMacos`.

## Remaining portable routes implemented in cycles 159–161

The source inventory identified **40 absent desktop menu affordances and five missing keyboard bindings**. They are now implemented in the working integration, using existing document handlers and guards; native Windows qualification remains pending. Counts describe routes, not new backend document features. Standard OS minimize/maximize controls and the Cocoa exclusions above remain outside this inventory.

| Slice | Count | Implemented routes | Evidence boundary |
| --- | ---: | --- | --- |
| [159 — Format](../../research/cycle-159/README.md) | 28 | Six headings; five list actions; nine block/structure/style actions; eight insertions. | Actual Qt pointer activation, exact Unicode Source edits, one-step Undo/Redo, unchanged disk, Live selection and auxiliary-field guards. Combined 159/160 focused batch **53/0/0**. Long-menu wheel fixture corrections and initial failures are retained. |
| [160 — File](../../research/cycle-160/README.md) | 6 | New in Library in Window; New in Library; Show in Library; New Folder; Autosave Saved Files Every Minute; Print Markdown Source. | Real handlers/dialogs, document/root guards and preference synchronization. Print Source currently has node/label/enabled-state coverage; ordinary native dialog cancellation is still pending. |
| [160 — View](../../research/cycle-160/README.md) | 2 | Synchronized Scrolling; Show Completions. | Existing completion semantics and Unicode/Undo guards retained, including native Mac and Windows-component rows. |
| [160 — Edit/Authors](../../research/cycle-160/README.md) | 3 | Authorship Annotations; Export Authorship Metadata; Set Up Authorship. | Actual menu nodes and existing dialogs, with profile validation, cancellation, document and metadata preservation. |
| [161 — Window](../../research/cycle-161/README.md) | 1 | Center. | Windows implementation centers the attached window on its own screen's available geometry; Mac handler unchanged. Geometry/other-window/document assertions and non-Windowed guards require native Windows execution. |
| [161 — Shortcuts](../../research/cycle-161/README.md) | 5 bindings | Open by Path (`Ctrl+Shift+O`); Command Palette (`Ctrl+Shift+P`); Spelling and Grammar (`Ctrl+:`); Next Issue (`Ctrl+;`); Previous Issue (`Ctrl+Alt+;`). | One Windows-scoped registration per chord; native Mac bindings retained. Host focused batch **15/0/0**. Windows code uses actual Qt key delivery and real bundled Harper; physical punctuation mapping remains an ordinary VM gate. |

These focused totals overlap selected existing controls and include init/cleanup; they are not full-suite totals. Required builds and the complete integration host suite pass (425/0/1 at Cycle 162). Exact final Windows ARM64/x64 CI and ordinary-package qualification remain separate open gates. Source inventory no longer identifies an unimplemented route in this finite list, but that does not establish complete desktop parity.

References: [native menu](../../src/NativeMenuBar.qml), [Windows menu](../../src/WindowsMenuBar.qml), [command registry](../../src/WorkspaceCommands.qml), [shortcut/dialog handlers](../../src/Main.qml), [Format tests](../../tests/cycle159-format-menu.inc), [remaining route tests](../../tests/cycle160-portable-menus.inc), [Center/shortcut tests](../../tests/windows-center-shortcuts.inc).

## Finite stability acceptance still needed

1. Require final-candidate ARM64/x64 complete Qt suites, native appearance and deployed checks, plus shared Mac/editor checks. Include the corrected real-page prerequisites in `footerControlsAndMenusRemainReachableWhenResized` and delivery witness in `livePersistenceMismatchAndTimeoutFailClosed`; the earlier CI failures remain historical evidence. Enumerate actual remaining skips individually.
2. Exercise all implemented slices, including cycles 154–161, through ordinary final-package Windows input: checked/enabled state, Source/Live and auxiliary-field focus, selection, one-step Undo, immediate post-menu Save and unchanged bytes. Synthetic Qt events and command-registry calls do not independently qualify desktop input.
3. Bind final-package normal-launch, rapid Live-save, publishing/export and affected updater workflows to exact package hashes and architecture claims. Earlier ordinary updater acceptance at `6558d35` and native updater regressions at `2d84cc4` do not qualify a different binary. [Menu evidence](M2-MENU-PARITY.md), [Live acceptance](LIVE-EDIT-ACCEPTANCE.md), [publishing acceptance](PUBLISHING-ACCEPTANCE.md), [updater evidence](M2-UPDATER-PARITY.md).
4. Record each remaining conditional assertion as actual Windows route coverage, equivalent shared/real-engine behavior, genuine platform capability or explicit unsupported affordance. Passing totals alone do not close full parity. Keep wider hardware, endurance and installer acceptance separate; claim individual W01–W18 rows only with their required evidence. [Test matrix](TEST-MATRIX.md).

Historical evidence is preserved. This stability checklist does not turn the remaining route inventory into completed desktop parity or close the user's continuing parity work.
