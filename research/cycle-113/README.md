# Cycle 113 — formatting focus safety

Date: 4 October 2026. Integrated target: **0.3.0-dev12 / macOS 0.3.0 (116)**.

Status: complete and included in verified build116. The integrated gate passes **173 regression tests** and **13 focused native checks** (11 workflows plus setup/cleanup). All three local app copies pass the actual-bundle checks, embedded-resource/version comparison and strict signatures.

Formatting now has one Source-ownership rule across toolbar buttons, shortcuts, native menus, quick formatting, transformations and direct snippet insertions. Source focus establishes the target; its deliberate formatting controls retain that target. Auxiliary fields, file navigation, Preview and Visual Edit cannot borrow a retained Source selection. Hidden/read-only Source and active input-method composition also block formatting. Open by Path remains a navigation action. Typewriter labels state their Source-only scope.

New bundled-QML regression cases cover Find/Replace, outline search, library filtering, folder controls and all three link fields, plus Preview/Visual focus. Positive toolbar/menu/keyboard paths must preserve the intended selection and one-step Undo; blocked paths must leave exact Unicode text, revision, focus and Undo/Redo unchanged.

Validation: the new positive/negative focus regressions pass, and the Dev/demo/Applications actual-executable focus group passes. Final per-copy identities and remaining acceptance limits are in the combined handoff. Real native keyboard activation, physical IME and VoiceOver remain unverified. Detailed logs and captures stay local and ignored. [Combined handoff](../cycle-116/README.md), [optional review](../usability/cycle-116.md).
