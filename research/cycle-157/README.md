# Cycle 157 — Windows Transformations, Substitutions and What's New menu candidate

22 October 2026. Bounded continuation from Focus/chrome checkpoint `ecf777d6c054446cef4635fc56d194b6e8bba4eb`, preserving the concurrent Go/Open Recent and View/Output work. Current build and package identities remain in [Windows STATUS](../../docs/windows/STATUS.md).

## Change

| Missing Windows route | Candidate behavior |
| --- | --- |
| Edit → Transformations | Make Upper Case, Make Lower Case, Capitalize and Make Title Case call the existing `win.editMarkdown` operations with the native menu's Source-selection guard. |
| Edit → Substitutions | Smart Quotes and Smart Dashes toggle the existing workspace settings. Their checkmarks follow those settings, including changes made outside the menu. |
| Help → What's New in Fomawrite | Opens the existing allowlisted bundled Help page through `helpDialog.showPage`. |

Every route uses the existing Windows focus dispatcher. Checkable items restore their checked binding after activation, following the existing Windows Command pattern. No new backend behavior, native Mac menu change, document format or clipboard operation is introduced.

## Existing regression cases tightened

The four existing cases now have a production-platform row and, on non-Windows hosts, an explicit Windows-component row. Each first verifies the correct production platform menu before activating the existing Windows menu Loader for the component row. Windows uses its actual production menu in the production row. Missing-node assertions and direct Windows backend/dialog fallbacks are replaced by required real Windows menu nodes and their menu signals; all native Mac semantic assertions remain.

| Existing case | Retained and added checks |
| --- | --- |
| `titleCaseTransformationIsDistinctAndPreservesUndo` | Actual ordered Transformation entries and labels; distinct title case/capitalization, selected Unicode text and Undo; protected Markdown stays unchanged. Added real upper/lower menu signals with Unicode expansion and exact Undo, and all four menu actions disabled while the auxiliary Find field owns focus. |
| `smartQuotesAreOptInAtomicAndMarkdownAware` | Actual checkable menu node, both toggle directions, externally changed setting/check synchronization, draft text/selection/revision/dirty/Undo preservation; existing opt-in quote insertion, atomic Undo and protected Markdown exclusions. |
| `smartDashesAreOptInAtomicAndMarkdownAware` | Actual checkable menu node and the same toggle/state preservation checks; existing opt-in dash insertion, atomic Undo, word/frontmatter/code/link/HTML exclusions. |
| `bundledHelpIsAllowlistedAndDoesNotEditDraft` | Actual What's New menu signal; Help/What's New/Shortcuts content and allowlisting, exact draft/cursor/Undo/dirty/status preservation, and unchanged recovery bytes. |

## Verification and limits

- Confirmed old-route red before the seven QML additions: **4 passed, 4 failed, 0 skipped**, including init/cleanup and the two passing View/chrome control cases, **5.870 seconds**. Each explicit Windows-component row failed at its intended missing node: Transformations, Smart Quotes, Smart Dashes, or What's New. Complete shared build/test output is retained in `evidence/edit-help-old-menu-red.log`; it also remains in Cycle 156's evidence.
- Focused `./bin/test` rebuilt the shared test target and passed **12 passed, 0 failed, 0 skipped**, **11.465 seconds**, Qt 6.11.2 ARM64/macOS. This comprises the four existing cases' native Mac and Windows-component rows, `previewTemplateFooterAndNativeMenuShareState`, `documentChromeKeepsMenusKeyboardAndNativeToggleAccessible`, and init/cleanup. Complete output: `evidence/focused-green.log`.
- `git diff --check` is clean for the changed QML and test file.
- These tests activate actual menu signals; they do not certify physical nested-menu clicks, scrolling, native Windows menu loading or ordinary packaged input. No VM, clipboard qualification, full suite, screenshots or package promotion ran in this slice.
- Required product `./bin/build`, complete integration regression, exact ARM64/x64 CI and ordinary packaged Windows qualification remain the parent's integration work. This record makes no CI-green or parity-completion claim.

Only Edit/Help sections of `WindowsMenuBar.qml`, these four test bodies/data methods, and this cycle's record belong to this slice. Go and View changes retain their separate cycle records. Root STATUS, CHANGELOG and native proof-map integration are maintained by the parent.
