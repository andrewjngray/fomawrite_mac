# Cycle 10 records

## Cycle 10 — library options menu

**Scope:** add the actions visible in Andrew's reference screenshot to the library toolbar dropdown.

**Implemented:** New File, New Folder, Sort By and View Options submenus, Hide/Show Sort Bar and Hide/Show Filter Bar. Original line icons, submenu chevrons and compact rounded styling. Bar visibility persists. Sort By shares its implementation with the existing sort dropdown; View Options controls dates and text excerpts. Choose Folder and Refresh remain below a separator. Hiding the filter bar preserves its active filter text.

**Verification:** build and 24 tests passed, including bar-action labels and submenu sorting. Native QA verified opening both submenus and hiding/restoring both bars. Sample document unchanged. Screenshots and logs in research/cycle-10/. New File/New Folder use the existing dialogs and I/O; creating new items via these new menu entries and full keyboard/dark-mode behavior remain untested in native QA. No claim of full iA View Options parity.

**Artifacts:** dist/Omawrite Dev.app and dist/Omawrite.app. Optional exercise: open the dropdown beside +, try the two submenus, hide and restore each bar, and confirm your preferred settings survive restart.

Screenshots use sample writing with the organizer hidden to exclude private recents.

Cycle 10 final check: New File and New Folder dialogs opened and cancelled successfully in native QA. Final packaging and deep/strict signature verification passed.
