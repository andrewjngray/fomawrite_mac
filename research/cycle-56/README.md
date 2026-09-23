# Cycle 56a: Authors setup (partial)

The observed pre-setup iA menu contains Authors → Set Up Authorship… and opens a sheet with required Name and optional Identifier. Omawrite now provides that bounded setup flow on macOS. The profile is local QML Settings data: Name is trimmed, required and limited to 100 UTF-16 units; Identifier is optional and limited to 200; control characters are rejected. Save stays disabled until valid, Cancel preserves the prior profile, and reopening permits editing as an explicit Omawrite choice.

The profile is configuration only. It does not modify Markdown, the caret, modified/Undo state or recovery; it does not create an authorship sidecar or assign Human/AI/Reference labels. Existing Edit → Authorship Annotations remains a separate manual assertion workflow.

`./bin/build` and the full native-access `./bin/test` suite pass with **94 tests, zero failures and zero skips**. The stable `dist/Omawrite Dev.app` was refreshed through 56a. Native QA observed the Authors menu and blank sheet using [`sample/Authors.md`](sample/Authors.md). Save was disabled until a synthetic Name was entered; Cancel restored the unchanged source and status. Automated coverage verifies profile Save/Cancel/reopen persistence, but native Save/reopen was deliberately not exercised to avoid leaving a fake profile. Screenshot capture was unavailable. See [`native-menu.txt`](native-menu.txt).

Post-setup iA behavior was not observed. Author registries, categories, automatic assignment, verified provenance, Mark As and Paste Edits From remain unknown or unimplemented. Native persistence, keyboard, dark/narrow layout and VoiceOver checks remain open. [Optional usability exercise](../usability/cycle-56.md).
