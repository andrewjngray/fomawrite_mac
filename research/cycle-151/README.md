# Cycle 151 — Back and forward for folders, and ⌘↑

Build 151 (`0.3.0-dev47`), 12 October 2026. Andrew approved folder arrows in the middle pane and ⌘↑ for Enclosing Folder, so Explore Folder can be undone like a Finder jump.

| Built | Detail |
| --- | --- |
| Back and forward arrows before the folder name in the middle pane's header | They use the Library's existing folder history: every change of the middle pane's folder is a step (Explore Folder, a Location or Favorite, Choose Folder, a folder opened in list view, Enclosing Folder). Hints "Previous folder" / "Next folder"; greyed out at either end. |
| ⌘↑ goes to the enclosing folder | Only while keyboard focus is in the middle pane or its header (a row, the folder button, the arrows), including inside the Files drawer in narrow windows. In Source and Live ⌘↑ keeps meaning "top of the document", and in the filter field "start of text"; the Go menu item shows no shortcut because a menu shortcut would fire everywhere. Enclosing Folder is a history step, so Back undoes it. |
| Narrow headers | New Document gives way first (⌘N and the file menus still create documents), then the arrows, so the folder name keeps room. The default 288 px column keeps the arrows even behind the macOS window buttons; only near the 232 px minimum with the sidebar hidden do they step aside. |

One builder from a fixed brief, then a reviewer (no ⌘↑ leaks found: Live, Source, filter, organizer, dialogs and popups all checked by running). Its findings, fixed before install:

1. An arrow that greyed out after its click (Back at the start of history) dropped keyboard focus to the window, so ⌘↑ stopped working. Focus now moves to the file list.
2. ⌘↑ did nothing in the Files drawer: Qt blocks a window shortcut while a popup that closes on Escape is open. A second shortcut lives inside the pane and is enabled only while the pane is in the drawer.
3. Double-clicking Back could zoom the window: once the arrow greyed out, the second click fell through to the header's drag-and-zoom area; a single click on a greyed arrow started a window move. The toolbar group now keeps its clicks (this also fixes the document arrows).
4. A folder deleted since it was visited left its arrow enabled and silent. Deleted folders now drop out of the history; with none left the arrow greys out and the Library says the folder is no longer available. A folder that exists but cannot be read no longer moves the history out of step.
5. The builder's hiding rule hid the arrows at the default width with the sidebar hidden; the reviewer's ordering (New Document first) is what shipped.

Tests (`tests/cycle151-folder-history.inc`): `libraryHeaderHasFolderBackAndForwardArrows`, `exploreFolderCanBeUndoneWithTheBackArrow`, `cmdUpGoesToTheEnclosingFolderOnlyFromTheMiddlePane`, `libraryHeaderArrowsStepAsideBeforeTheyCanOverlap`, `folderArrowsKeepFocusAndClicksInsideTheGroup`, `deletedFoldersDropOutOfTheFolderHistory`, `cmdUpWorksInsideTheFilesDrawer`. The double-click and drawer tests were checked to fail with their fixes removed.

At the reference 232 px Library column (with the sidebar showing) New Document now steps aside so the arrows and the folder name fit; `referenceWorkspaceCompositionMatchesConcept` asserts that instead of a visible New button.

Open: the Go menu says "Back in Library" / "Forward in Library" where the arrows say "Previous folder" / "Next folder"; ⌘↑ is not shown anywhere in the interface.
