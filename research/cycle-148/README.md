# Cycle 148 — Getting from the open document to its folder

Build 148 (`0.3.0-dev44`), 12 October 2026. Andrew's request after opening files from Finder and other apps: Recents shows the file, but nothing takes the middle pane to where the file lives.

| Andrew asked | What was built |
| --- | --- |
| Hovering the document name in the top bar should show where the file is. | The tooltip shows the full path, with " — Edited" when unsaved; an untitled document still shows "Untitled.md". Both name instances (split Preview and full Preview). `Backend::filePath`. |
| Right-clicking the document name should give the same menu as a file in the middle pane (Show in Finder, Move to Trash, and the rest). | The Library's own file menu opens for the open document, wherever it lives and even when the Library column is hidden. Only the name itself takes the right-click, not the empty header beside it. Untitled documents have no menu. Every action goes through the same `Backend::libraryItemAction` guards as the middle pane (Trash is refused while there are unsaved edits; Rename keeps the document open under its new name). |
| A new "Explore Folder" under Open in New Tab and Open in New Window, which puts the file's folder in the middle pane. | Explore Folder in every file menu: the middle pane, Recents and Favorites in the sidebar, and the top bar. The file's own folder becomes the current folder (tree and list mode, even when the file was already visible deeper in the tree), the filter clears, the Library column shows (or the Files drawer, when the window is too narrow to dock it), and the file's row is selected and scrolled into view. `FileLibrary::exploreFolder`. |

Unchanged on purpose: opening a file from Finder still leaves the middle pane where it was; Explore Folder is the deliberate step. Browsing to a folder outside every Location adds it as a Location, as choosing a folder always has.

Built by two parallel builders from one contract (library and tests; QML), then a reviewer, whose findings were fixed before install:

1. The explored row was not visibly selected unless it was the open document (the list has no highlight for its current row). The explored file now carries the selection until another folder or document takes over; one highlighted row at a time.
2. The selection test passed without doing anything (the file was already row 0). It now sorts by name, explores a file at row 1 through the menu item itself, then a second file that is not the open document, and counts highlighted rows.
3. In a narrow window Explore Folder changed the folder with nothing visible. It now opens the Files drawer, as the toolbar's Library button does; Show in Library gets the same.
4. A non-Markdown file or a hidden file gave "no longer available". Each now says why (the Library lists only Markdown and text files; it does not list hidden files) and the folder does not change.
5. Right-clicking empty header space to the right of the name opened the menu. The handlers now cover the name only.
6. With auto-hide chrome, the header could fade while the menu was open. The menu now holds the chrome.
7. An unreadable parent folder could report success. It now reports that the folder could not be opened.

Tests (`tests/cycle148-document-navigation.inc`): `exploreFolderMakesTheFilesFolderCurrentInTreeAndListModes`, `exploreFolderInsideALocationAddsNoLocation`, `exploreFolderRefusesMissingRemoteFolderAndNonTextTargets`, `backendFilePathIsEmptyUntilADocumentIsOpened`, `topBarDocumentNameShowsThePathAndOpensTheLibraryMenu`, `rightClickingTheTopBarDocumentNameOpensTheLibraryMenu` (a real right-click on the name, and none beside it).

Open: the document menu is mouse-only (no keyboard or VoiceOver route to it); highlight matching compares URLs without resolving symlinks (pre-existing).
