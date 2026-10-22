# Windows parity ledger

22 October 2026. This records ongoing work toward running all applicable Mac regressions on Windows. It does not close desktop parity or qualify a package. Exact candidate/CI identities remain in [STATUS.md](STATUS.md); historical M1 skip totals remain historical.

## Capability exclusions

| Boundary | What may differ or remain excluded | Required portable coverage |
| --- | --- | --- |
| Native macOS checker | NSSpellChecker availability/languages, native-provider findings and provider-specific timing | Actual bundled Harper spelling, grammar, Review, corrections, exclusions and UTF-16 offsets. The 28 common cases now have real-engine Windows rows; native Mac rows remain. [Checker evidence](M2-CHECKER-PARITY.md), [Source fixtures](../../tests/cycle143-source-spelling.inc), [Review fixtures](../../tests/cycle144-review-pane.inc). |
| Cocoa integration | Native menu roles/topology, NSFileVersion, Share/Speech, AppKit tab/window integration | Truthful Windows capability policy and preservation of common document state. The Versions test bodies are Mac-conditional; their Windows function result does not establish version-history support. [Tests](../../tests/tst_fomawrite.cpp), [native menu](../../src/NativeMenuBar.qml). |
| Platform rendering/filesystem semantics | Platform colours/fonts and documented QFile differences | Windows native appearance checks, actual fallback rendering, exact document bytes and filesystem safety. Symlink privilege or watcher unavailability is an environment limitation requiring its own recorded reason, not a blanket Windows exclusion. [Test matrix](TEST-MATRIX.md). |

The complete Windows suite runs without a fixed platform skip list. The old `nativeWorkspaceMenusShareState` skip covered **portable behavior** and is removed in the current candidate. Shared Mac CI's 13 skips are 12 unavailable native-checker grammar rows and one Windows-only appearance test; the corresponding bundled-Harper rows run. These are not Windows feature exclusions.

Mac-conditional assertions need separate treatment: native checker details in `writingReviewExcludesCodeAndCorrectsSafely` and the two NSFileVersion test bodies are genuine capability boundaries. OS-specific colour expectations are legitimate substitutions. Actual recent-menu activation, toolbar metric controls and chrome-menu checks in `recentMenuTracksLibraryAndGuardsOpen`, `statisticsCountRenderedUnicodeTasksAndAuthorship` and `chromePresentationModesPersistAndKeepCommandsReachable` remain portable route omissions. A passing function with a conditional body does not establish coverage of that body.

## Implemented View and clipboard/search slices

Candidate `7c37078b0d135f78b68f412444083ae7f919b339` restores the complete `nativeWorkspaceMenusShareState` assertions using platform menu lookup, plus these 15 Windows View commands and Paginated Preview:

- `sortBar`, `filterBar`, `excerpts`;
- `sortName`, `sortModified`, `sortCreated`, `sortExtension`;
- `ascending`, `descending`, `foldersFirst`;
- `dateNone`, `dateModified`, `dateCreated`;
- `navigationTree`, `navigationList`.

The same candidate implements nine clipboard/search actions: Copy Markdown/HTML/Formatted, Paste As Plain Text/Markdown from HTML, Delete and Find Next/Previous/Selection. `windowsClipboardSearchMenusPreserveFocusAndUndo`, the existing File/Edit regression and search fixtures exercise actual Windows menu routes, Source/Live selection, auxiliary-field isolation and Undo.

The required local build and complete suite passed **367 tests, zero failures and one Windows-only skip**, 540.931 seconds. Exact native CI passed ARM64 340/0/0 but failed x64 339/1/0; shared Mac Qt failed 352/3/13. The confirmed Save defect and fixture corrections are detailed in the Windows status. Ordinary packaged desktop menu checks remain incomplete. Qualification still requires menu activation, checked/enabled state, date/sort independence, responsive geometry and unchanged document bytes/history. [Workspace/menu tests](../../tests/tst_fomawrite.cpp), [platform lookup](../../tests/platform-menu-helpers.inc), [Windows menu tests](../../tests/windows-menu.inc), [Windows menu](../../src/WindowsMenuBar.qml).

## Focus/chrome implementation awaiting native qualification

Cycle 154 implements the existing Focus, style-check, Title Bar and Toolbar/Stats Only menu routes. Five existing tests now require real Windows nodes and state instead of accepting absent controls or invoking only shared commands. Full local build/suite passed **370/0/1**; exact native red/green and ordinary popup interaction remain pending. The corresponding entries below describe the earlier route inventory and are not yet qualified as complete. [Cycle record](../../research/cycle-154/README.md).

## Remaining desktop route scope

These portable affordances remain outside the implemented slices. Shared Backend, command, dialog or footer tests establish underlying behavior only. Missing additional routes prevent a claim of complete desktop menu parity; they are broader affordance work, not newly demonstrated data-loss or rendering defects. Unsupported routes must remain explicit rather than become Mac-only test exemptions.

| Scope | Remaining routes and conditional assertions |
| --- | --- |
| File/navigation | Open Recent activation; Show in Library; New Folder/autosave menu controls; Markdown-source printing; Go document/Library history, enclosing folder, open link, Quick Search, command palette and saved locations/searches/hashtags. `recentMenuTracksLibraryAndGuardsOpen` guards actual recent-menu activation to Mac, while shared missing-file protection still runs. |
| Edit/Format | Smart substitutions, transformations/completion and additional heading/list/structure/insertion actions. The implemented clipboard/search routes above are awaiting qualification, not missing implementation. |
| View/output | Output Style presets/custom-theme management; Live exact-theme choice; title/document-bar visibility and metric customization. `previewTemplateFooterAndNativeMenuShareState` retains footer coverage on Windows; `statisticsCountRenderedUnicodeTasksAndAuthorship` and `chromePresentationModesPersistAndKeepCommandsReachable` omit or substitute shared commands for native menu assertions. |
| Focus/authorship/help | Sentence/paragraph/typewriter and style-review menu routes; authorship setup/annotation/export; What's New. `focusMenuKeepsModesExclusiveAndTypewriterIndependent` preserves shared state assertions but explicitly accepts absent Windows menu controls. Center Window is also an unsupported portable affordance, rather than an inherently Mac-only concept. |

References: [conditional tests](../../tests/tst_fomawrite.cpp), [Output Style fixture](../../tests/cycle102-workspace-controls.inc), [Mac menu inventory](../../src/NativeMenuBar.qml), [historical menu limits](M1-EVIDENCE.md#windows-m2-menu-limits). These omissions remain explicit until real Windows routes are implemented and verified.

## Finite stability acceptance still needed

1. Require final-candidate ARM64/x64 complete Qt suites, native appearance and deployed checks, plus shared Mac/editor checks. Include the corrected real-page prerequisites in `footerControlsAndMenusRemainReachableWhenResized` and delivery witness in `livePersistenceMismatchAndTimeoutFailClosed`; the earlier CI failures remain historical evidence. Enumerate actual remaining skips individually.
2. Exercise the implemented View and clipboard/search routes through ordinary final-package Windows input: checked/enabled state, Source/Live and auxiliary-field focus, selection, one-step Undo and unchanged bytes. Synthetic Qt events and command-registry calls do not independently qualify desktop input.
3. Bind final-package normal-launch, rapid Live-save, publishing/export and affected updater workflows to exact package hashes and architecture claims. Earlier ordinary updater acceptance at `6558d35` and native updater regressions at `2d84cc4` do not qualify a different binary. [Menu evidence](M2-MENU-PARITY.md), [Live acceptance](LIVE-EDIT-ACCEPTANCE.md), [publishing acceptance](PUBLISHING-ACCEPTANCE.md), [updater evidence](M2-UPDATER-PARITY.md).
4. Record each remaining conditional assertion as actual Windows route coverage, equivalent shared/real-engine behavior, genuine platform capability or explicit unsupported affordance. Passing totals alone do not close full parity. Keep wider hardware, endurance and installer acceptance separate; claim individual W01–W18 rows only with their required evidence. [Test matrix](TEST-MATRIX.md).

Historical evidence is preserved. This stability checklist does not turn the remaining route inventory into completed desktop parity or close the user's continuing parity work.
