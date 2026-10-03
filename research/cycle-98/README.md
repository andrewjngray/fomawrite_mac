# Cycle 98 — workspace keyboard and native acceptance

3 October 2026. This cycle addresses concrete workspace focus gaps and extends the production-main native fixture. The runnable review app is the stable `dist/Fomawrite Dev.app`; the combined cycle handoff records its final version and packaging evidence.

## Changes

- F6 and Shift+F6 move through the visible Organizer, Files, Source, Preview and toolbar regions. They leave Tab available for writing input and are disabled while another modal surface owns keyboard focus. The shortcuts are listed in Help → Keyboard Shortcuts.
- Compact navigation now takes keyboard focus on its named Close control. Escape closes it and returns focus to the initiating control where still available; otherwise focus returns to a visible writing surface.
- Automatic contraction moves keyboard focus out of a newly hidden pane, and hiding a pane also handles a read-only Preview workspace.
- Toolbar focus rings belong to the active window; inactive selected toolbar controls use the neutral selection fill.

## Acceptance matrix

The native fixture uses synthetic Markdown, independent test preferences and temporary files. It exercises production window/session code. It does not attach to or alter Andrew's private documents.

| Area | Evidence / scope |
| --- | --- |
| Keyboard regions | Passed native shortcut dispatch for F6, Shift+F6, compact focus rescue and faded-toolbar reveal. |
| Compact navigation | Passed named focused Close control, Escape and restored initiator. Existing dirty-navigation cancellation regression remains part of the full suite. |
| Accessibility | Passed Qt native accessible button role/name and source text interface. Physical VoiceOver listening and complete reading order remain manual acceptance. |
| Native windows/tabs | Passed creation of a separate window/native tab, per-window checkpoint save/restore, native tab detach and regroup through the production restoration helper; all requested layouts remain independent. This is not an actual quit/relaunch test. |
| Full screen | Passed native entry/exit, aligned headers, remembered pane state and untouched dirty draft/selection/Undo. |
| Dark/inactive | Passed synthetic app-only captures and inactive focus indicator check. [Dark](screenshots/workspace-dark.png), [inactive](screenshots/workspace-inactive.png) and [full screen](screenshots/workspace-fullscreen.png) captures were inspected. |
| Scaling | Logical resize matrix is tested. Actual macOS display setting changes and moving between physical displays are not verified. |
| Canonical document safety | Native layout/keyboard sequence checks text, selection, dirty state and Undo; final Undo must restore the exact saved source. |

`./bin/test-window-routing` compiled the real production main/QML and passed with exit 0; [native build and verification log](native-workspace.log). It also verifies Cycle 100 navigation and the three print-dialog cancellation routes, and accepts Finder/share requests without sending or printing. The [compact](screenshots/workspace-compact.png) and [wide](screenshots/workspace-wide.png) captures contain only synthetic documents. Full regression and packaged app identity are recorded in the combined handoff.

The integrated fixture initially selected its draft before the previous navigation fixture's asynchronous document-loaded cursor reset had settled. A short boundary wait now separates those independent fixture setups; no production cursor behavior was weakened. Compilation attempts during parallel source edits were rerun once those source files were ready.

This cycle does not certify all historical menu states, actual app quit/relaunch with arbitrary workspaces, physical VoiceOver/display scaling, or full competitor parity.

[Optional review exercise](../usability/cycle-98.md).
