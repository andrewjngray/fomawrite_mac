# Cycle 118 — independent editing and layout

Date: 5 October 2026. Review target: **0.3.0-dev14 / macOS 0.3.0 (118)**.

Status: implementation complete. The full suite passes **178 tests, 0 failed, 0 skipped**, including setup/cleanup. Dev, ordinary/demo and Applications are refreshed to build118 and each passes **147 actual-executable checks**, embedded-resource/version comparison and strict signature verification, with **zero QML warnings**. The focused Cocoa suite passes **12 tests, 0 failed, 0 skipped**, including setup/cleanup.

## Approved interaction model

Andrew identified that Source / Split / Full mixed an editor choice with layout, while Visual Edit stood alone. Cycle118 separates these into two permanent capsules: **Source / Visual Edit** and **Single / Split**.

| Editing | Layout | Result |
| --- | --- | --- |
| Source | Single | Source editor alone |
| Visual Edit | Single | Visual editor alone |
| Source | Split | Source editor at left; read-only output Preview at right |
| Visual Edit | Split | Visual editor at left; read-only output Preview at right |

Changing editing mode retains layout. Changing layout retains the selected editor. Preview-only remains an explicit reading action; neither footer capsule is selected there, and choosing an editor/layout returns to the retained editing workspace. Single does not mean macOS full screen.

Both capsules stay at fixed positions for a fixed document-area width, including their full labels at minimum supported widths. Appearance remains beneath the editor; output template follows the Preview divider. These menus compact to fit before the fixed groups; reduced padding keeps the Aa label readable at the minimum button width. Their width budgets use both the editor width and the actual Preview edge, preventing temporary overlap while pane geometry settles. The Visual editor uses writing appearance and editor zoom; read-only Preview uses its output template and independent Preview zoom. The headers distinguish Visual Edit from Preview and preserve filename/Edited identity.

The underlying document remains canonical UTF-8 Markdown. Visual Edit is still the existing supported projection, with explicit Source fallback for unsupported structures. This cycle does not add table restructuring, arbitrary multiline paste, complex list editing or a general WYSIWYG conversion layer.

## Editing safety and saved-state migration

Source formatting, clipboard commands and completions cannot act on an old hidden Source selection when Visual Edit or Preview is active. Changing away from Source closes its completion popup, and a late completion choice is refused. Canonical Undo/Redo remains available without borrowing a hidden editor’s clipboard target.

Find/Replace works on canonical Markdown. Invoking it reveals Source while retaining Single/Split; choosing Visual Edit or Preview-only closes Find so a hidden search bar does not keep controlling the document.

Workspace state now writes version2 and accepts both version1 and version2 checkpoints. Legacy Source-only always restores Source + Single, even if an old dormant visual flag was saved. Legacy Full Visual Edit restores Visual Edit + Single. Legacy Split retains the chosen editor beside Preview, and legacy read-only Full restores Preview-only. Version2 also remembers the last editing layout for returning from Preview-only. This migration changes workspace state, not document text or output styles.

## Verification

- Full regression suite: **178 passed, 0 failed, 0 skipped**, including setup/cleanup.
- Dev, ordinary/demo and Applications: **147 checks each**, matching embedded resources/version, valid strict signatures and **zero QML warnings**. Ordinary/demo and Applications have identical executables.
- Focused Cocoa suite: **12 passed, 0 failed, 0 skipped**, including setup/cleanup.
- Production source commit: `4cfdfd80793fc2b773a9bd9c702d14d5dc79aaf9`. [Verified build identities and gate results](verified-builds.json).
The completed checks cover all four editor/layout combinations, Preview-only and return, wide/narrow and light/dark layouts, fixed control geometry, compact menu access and divider tracking. They also cover focus ownership, hidden-Source clipboard/completion refusal, Find transitions, exact draft preservation and Undo/Redo, independent editor/Preview zoom, template independence, version1/version2 workspace migration and read-only Preview. The actual-executable checks run inside each refreshed app copy.

Raw logs, captures and temporary fixtures remain local. Published evidence consists of this reviewed summary and the compact verified-build manifest. The previous wider native-window run stopped after 79 assertions when macOS refused foreground activation; this cycle does not automatically close its later native keyboard/full-screen/Drawer/tab-detach or print/share scenarios. Physical mouse/keyboard/IME, VoiceOver, multiple displays/scaling and arbitrary restored workspaces remain acceptance work. Synthetic Qt input is not physical OS input.

## Review artifacts

Target paths remain `dist/Fomawrite Dev.app`, `dist/Fomawrite.app` and `/Applications/Fomawrite.app`. All three copies are refreshed to build118, with passing actual-executable, resource/version and strict-signature checks. Confirm the running version/path in About before reviewing. These are local ad-hoc review builds, not a notarized public release. The old public RC1 is unchanged.

Use the [short review exercise](../usability/cycle-118.md). Andrew’s next review should assess whether choosing an editor and choosing a layout now feel independent and predictable.
