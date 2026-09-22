# Cycle 17 — search/edit menus

21 September 2026. Implemented native Find submenu and focused-field editing states/Delete. Shared search entry reveals source from preview-only mode. Command-G and Shift-Command-G wrap forward/backward. C++ finds literal case-insensitive matches without lowercasing the entire document; replacements use one QTextCursor undo block and preserve UTF-8 source semantics. Empty/no-match replacement controls are disabled.

Build and all 34 tests passed (logs/build.log, logs/test.log), covering Unicode positions after İ/emoji, empty/missing queries, wrapping, single and all replacements/undo, selection search and Delete on the query field. Existing Qt Material teardown warnings remain. Dev prepared and package/signature checks passed (logs/prepare-dev-app.log, logs/package-mac.log, logs/package-signature.log).

Andrew explicitly authorized inspection of current Dev content. Clean user document closed normally without modification; stable bundle refreshed. Native sample entered as `caf CAF caf` (the typing tool omitted accented characters, so Unicode is verified by automated tests). Find and Replace opened from Edit; replacing all with tea produced three replacements; clicking editor then Command-Z restored all three in one step. Shift-Command-G wrapped 1/3 → 3/3 and Command-G returned to 1/3. [Screenshot](screenshots/search-and-replace.png) visually inspected; sidebar hidden, no private writing saved. Agent-created unsaved sample discarded via New prompt; Dev left on clean Untitled.

Both dist/Omawrite Dev.app and dist/Omawrite.app include this cycle and Cycle 16c tooltip fix. Exact hover timing, successful live Move To/separate-volume QA and earlier UI gaps remain pending. Search does not provide regex, whole-word or Unicode normalization equivalence. Full dark/narrow/keyboard/accessibility checks and unknown iA submenu behavior remain open. Undo targets the focused input: focus source to undo a document replacement.

Changes remain uncommitted alongside earlier cycles. Next: 18a block formatting. 20 feature increments remain. [Optional exercise](../usability/cycle-17.md).


**Final visual correction:** screenshot review showed the search panel covering the first source line. The editor now reserves vertical space beneath Find/Replace. Rebuilt, all 34 tests passed again, and the refreshed native sample screenshot confirms source matches remain visible. Final screenshot uses `cat CAT cat`; only agent-created test text was discarded afterward.
