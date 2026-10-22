# Windows parity ledger

22 October 2026. This records ongoing work toward running all applicable Mac regressions on Windows. It does not close desktop parity or qualify a package. Exact candidate/CI identities remain in [STATUS.md](STATUS.md); historical M1 skip totals remain historical.

## Capability exclusions

| Boundary | What may differ or remain excluded | Required portable coverage |
| --- | --- | --- |
| Native macOS checker | NSSpellChecker availability/languages, native-provider findings and provider-specific timing | Actual bundled Harper spelling, grammar, Review, corrections, exclusions and UTF-16 offsets. The 28 common cases now have real-engine Windows rows; native Mac rows remain. [Checker evidence](M2-CHECKER-PARITY.md), [Source fixtures](../../tests/cycle143-source-spelling.inc), [Review fixtures](../../tests/cycle144-review-pane.inc). |
| Cocoa integration | Native menu roles/topology, NSFileVersion, Share/Speech, AppKit tab/window integration | Truthful Windows capability policy and preservation of common document state. The Versions test bodies are Mac-conditional; their Windows function result does not establish version-history support. [Tests](../../tests/tst_fomawrite.cpp), [native menu](../../src/NativeMenuBar.qml). |
| Platform rendering/filesystem semantics | Platform colours/fonts and documented QFile differences | Windows native appearance checks, actual fallback rendering, exact document bytes and filesystem safety. Symlink privilege or watcher unavailability is an environment limitation requiring its own recorded reason, not a blanket Windows exclusion. [Test matrix](TEST-MATRIX.md). |

`nativeWorkspaceMenusShareState` is **portable behavior**, despite its name and the Mac-only skip in the last CI candidate. The pending View slice removes that skip. Library visibility, sort/filter/excerpts, dates, navigation and preview choices must be tested through the real Windows menu. A generic command call does not prove that a desktop route exists or has the correct checked/enabled state.

## View-menu slice — in progress

The pending change restores the complete `nativeWorkspaceMenusShareState` assertions using platform menu lookup, plus these 15 Windows View commands and Paginated Preview:

- `sortBar`, `filterBar`, `excerpts`;
- `sortName`, `sortModified`, `sortCreated`, `sortExtension`;
- `ascending`, `descending`, `foldersFirst`;
- `dateNone`, `dateModified`, `dateCreated`;
- `navigationTree`, `navigationList`.

The required local build and complete suite passed 366 tests, zero failures and one Windows-only skip. Native Windows CI and ordinary packaged-app checks are still required. Require actual menu activation, check-state updates in both directions, independence of date display from sort order, responsive Library/Organizer behavior, preview routing/label and unchanged document bytes/history. Preserve the existing assertions rather than replace them with command-registry-only checks. [Workspace/menu tests](../../tests/tst_fomawrite.cpp), [platform lookup](../../tests/platform-menu-helpers.inc), [Windows menu tests](../../tests/windows-menu.inc), [Windows menu](../../src/WindowsMenuBar.qml).

## Remaining desktop route scope

These are portable affordances omitted from the last CI candidate outside the View slice. A bounded clipboard/search slice is now restoring nine existing actions (three Copy formats, two Paste As formats, Delete and three Find actions), with actual Windows-menu focus/selection assertions. Its build and native qualification remain pending. Shared Backend, command, dialog or footer tests establish underlying behavior only. Their absence is a product scope limitation, not a Mac-only test exemption.

| Scope | Remaining routes and conditional assertions |
| --- | --- |
| File/navigation | Open Recent activation; Show in Library; New Folder/autosave menu controls; Markdown-source printing; Go document/Library history, enclosing folder, open link, Quick Search, command palette and saved locations/searches/hashtags. `recentMenuTracksLibraryAndGuardsOpen` guards actual recent-menu activation to Mac, while shared missing-file protection still runs. |
| Edit/Format | Copy Markdown/HTML/Formatted and Paste As variants; Find Next/Previous/Selection, Delete; smart substitutions, transformations/completion; additional heading/list/structure/insertion actions. `nativeFileAndEditMenuActionsRespectFocusAndGuards` exercises Windows basic Copy and shared extra-format APIs, not absent extra-format menu routes. |
| View/output | Output Style presets/custom-theme management; Live exact-theme choice; title/document-bar visibility and metric customization. `previewTemplateFooterAndNativeMenuShareState` retains footer coverage on Windows; `statisticsCountRenderedUnicodeTasksAndAuthorship` and `chromePresentationModesPersistAndKeepCommandsReachable` omit or substitute shared commands for native menu assertions. |
| Focus/authorship/help | Sentence/paragraph/typewriter and style-review menu routes; authorship setup/annotation/export; What's New. `focusMenuKeepsModesExclusiveAndTypewriterIndependent` preserves shared state assertions but explicitly accepts absent Windows menu controls. Center Window is also an unsupported portable affordance, rather than an inherently Mac-only concept. |

References: [conditional tests](../../tests/tst_fomawrite.cpp), [Output Style fixture](../../tests/cycle102-workspace-controls.inc), [Mac menu inventory](../../src/NativeMenuBar.qml), [historical menu limits](M1-EVIDENCE.md#windows-m2-menu-limits). These omissions remain explicit until real Windows routes are implemented and verified.

## Acceptance still needed

1. Finish the View slice and run its unchanged common assertions on both native Windows architectures and Mac. Afterward, do not retain the old fixed menu skip; explain any remaining conditional skip individually.
2. For each conditional assertion, record one of: actual Windows route coverage, equivalent real-engine/shared behavior, genuine platform capability, or explicit unsupported affordance. Passing totals alone cannot show this distinction.
3. Keep updater coverage separate from desktop qualification: `2d84cc4` ran all updater cases on both native architectures, including real-engine installation/rollback; its full runs still failed an obsolete menu assertion. Require final green CI at the accepted package identity. [Updater evidence](M2-UPDATER-PARITY.md).
4. Repeat ordinary final-package launch, Live timing and publishing/export workflows with independent receipts. Complete W04/W05 and the applicable W01–W08, W09, W11–W13, W16/W18 scope before claiming the corresponding matrix rows; short-fixture OCR or synthetic diagnostics do not establish every desktop route. [Test matrix](TEST-MATRIX.md), [Live acceptance](LIVE-EDIT-ACCEPTANCE.md), [publishing acceptance](PUBLISHING-ACCEPTANCE.md).

Historical evidence is preserved. An unsupported menu route must not be reported as tested simply because its Backend function passed elsewhere.
