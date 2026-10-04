# Cycle 114 — safe Source link editing

Date: 4 October 2026. Integrated target: **0.3.0-dev12 / macOS 0.3.0 (116)**.

Status: complete and included in verified build116. The integrated gate passes **173 regression tests** and **13 focused native checks** (11 workflows plus setup/cleanup). All three local app copies pass the actual-bundle checks, embedded-resource/version comparison and strict signatures.

The Source link action opens one consistent Insert/Edit dialog from the toolbar, shortcut, quick formatting or native Format menu. It exposes text, destination and optional title, keeps errors beside the fields and replaces the captured range as one Undo operation. Ordinary supported inline links can be edited; saving unchanged fields retains their exact original syntax. Destinations with spaces and escaped characters receive safe Markdown delimiters.

The dialog captures document identity, source and a load generation. An intervening edit, file change or reload permanently invalidates that session, including edit-then-Undo. Refused application retains the entered fields. Explicit Cancel/Escape restores the original Source selection and reading position only while its session remains valid; outside dismissal leaves focus with the clicked destination.

Boundaries: multiline selections, code, comments and ambiguous/unsupported link forms use Source editing. This is not a complete Markdown grammar, image editor or general rich-link model.

Validation: four focused link regression slots pass, covering syntax, Unicode/escaping, unchanged-save, stale-session, cancellation and Undo/Redo. Dev/demo/Applications actual-executable checks pass link insert/edit/cancel/Undo and stale-link refusal with bounds checks in narrow dark layout. Final per-copy identities and remaining acceptance limits are in the combined handoff. Detailed reports stay local and ignored. [Combined handoff](../cycle-116/README.md), [optional review](../usability/cycle-116.md).
