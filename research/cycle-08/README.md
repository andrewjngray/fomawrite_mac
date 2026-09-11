# Cycle 8 — rounded sorting menu

Andrew requested a focused menu refinement based on his iA screenshot.

Implemented a rounded Sort by pill with a trailing chevron and a separate compact dropdown: Date Modified, Date Created, Name, Extension; A to Z / Z to A; Pin Folders to Top; Show Date; Show Text Excerpts. Mutually exclusive field and direction groups prevent deselecting the active choice. Menu typography is Helvetica Neue 13px with 28px rows, checkmarks, rounded highlighting and navy selection. Folder-management actions remain in the separate toolbar dropdown, using the same compact style.

Date and excerpt visibility preferences persist. Hidden excerpts are not fetched. Show Date currently toggles modified dates on/off; date-format/type and Navigation submenus from iA are not implemented. This pass changes the sorting/menu UI rather than the whole application.

Build and 23 regression tests passed, including a new UI-action test for all sort fields and both directions. Native visual inspection used the stable Omawrite Dev identity and our sample library; screenshot: screenshots/sort-menu.png. This screenshot precedes the nonvisual exclusive-group correction, which was covered by the final build/test pass. The QA sample was closed normally before refreshing the same development bundle. Source documents were not edited. Dark appearance and a full keyboard-navigation audit remain unverified.

The supplied screenshot contains private sidebar material and was not copied into this public repository. Its menu was used as a visual reference only.

Final native follow-up: reattached to the rebuilt stable app immediately and verified Name selected with Z to A selected/A to Z unselected. The final menu is left open for Andrew to inspect. No new screenshot was saved after the active document changed from the shared sample.

Final packaging and deep/strict signature verification passed. Both dist/Omawrite.app and dist/Omawrite Dev.app contain the final menu implementation.
