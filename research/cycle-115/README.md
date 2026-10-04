# Cycle 115 — actual-bundle writing acceptance

Date: 4 October 2026. Integrated target: **0.3.0-dev12 / macOS 0.3.0 (116)**.

Status: complete. All six additional groups pass in the refreshed Dev, ordinary/demo and Applications executables. The integrated gate passes **173 regression tests** and **13 focused native checks** (11 workflows plus setup/cleanup), with no failures or skips.

Six new groups in the actual executable check:

1. Formatting follows the active writing/auxiliary field.
2. Link insertion, editing, Cancel and canonical Undo/Redo.
3. Stale-link refusal with entered fields retained in a narrow dark window.
4. Table navigation through real Tab/Shift-Tab handlers without source mutation.
5. Continuous 720–1440px width and 520–800px height changes, plus workspace restoration.
6. Narrow export horizontal access to the right-hand bands and safe Cancel.

The wider keyboard-routing check also exposed a skipped read-only Preview pane after footer consolidation. F6/Shift-F6 now target the rendered writing surface directly, keeping Source → Preview → toolbar and reverse traversal intact. A regression covers both read-only Preview and Visual Edit, including exact draft/selection and Undo preservation.

These supplement the existing footer, daily-writing and independent-zoom checks. The checker uses disposable documents/settings and synthetic Qt input in the app’s own isolated window. It renders before measuring/capturing and reports the actual executable plus embedded-resource/version comparison. It does not use physical OS pointer/keyboard or macOS Accessibility automation.

Validation: production packages are built; Dev, ordinary/demo and Applications each pass 124 footer states, seven earlier daily-writing workflows, five zoom workflows and all six additional acceptance groups. All three report zero QML warnings, matching tested executable/resource identities and valid strict signatures. Final suite/native counts and reviewed artifact identities are in the combined handoff. Raw reports, logs and screenshots remain local and ignored; concise reviewed outcomes belong in this file and the [combined handoff](../cycle-116/README.md). Physical IME, VoiceOver, display changes and arbitrary restored user workspaces remain open.


## Wider native-window check: activation blocked

The separate native window-routing fixture passes **79 assertions** before macOS refuses to activate its command-line-launched application. Covered: initial presentation migration, exact draft/selection/Undo/template preservation, separate windows and native tab grouping, ordinary open routing, live Unicode heading transfers and missing-heading/dirty-Cancel behavior, independent workspace checkpoints and native resizing through 1440, 1120, 900, 720 and back to 1440px.

The application remains inactive despite a bounded, test-only activation request. Its Source focus item and F6 shortcuts are enabled, but native WindowShortcut dispatch requires an active application/window. The test therefore fails its explicit activation precondition; this is **not an F6 native pass** and no keyboard assertion was weakened. The production Preview focus target has been corrected, and the separate bundled-QML F6 regression passes with real offscreen shortcut dispatch.

The later native-keyboard, contraction-focus, faded-toolbar/Drawer Escape, inactive-window, full-screen, tab-detach/restoration and print/share scenarios were not reached in this run. Earlier historical evidence remains historical; these workflows and physical VoiceOver/display/input acceptance remain open for this build.
