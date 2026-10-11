# Cycle 149 — The folder chooser starts where the folder lives

Build 149 (`0.3.0-dev45`), 12 October 2026. From Andrew trying build 148.

| What Andrew saw | Cause | Fix |
| --- | --- | --- |
| Clicking the folder button above the middle pane ("ops") opened the chooser at the top of the disk, not where ops is. | The "Choose your writing folder" dialog was opened with no starting folder, so macOS chose one. | `LibraryPane.chooseFolder` starts the dialog in the folder that holds the current one, with the current folder selected, as Show in Finder does. The Library options menu's Choose Folder… uses the same function. Test `folderChooserStartsWhereTheCurrentFolderLives`. |
| Show in Finder on a file. | Already reveals the file selected in its own folder (`open -R`); his screenshot shows it working. | Nothing to change. |

Assessed, not built (waiting for Andrew):

- The back and forward arrows above the document are document history for this window: each document opened is a step, and the arrows reopen the previous or next one at the cursor position it had. They do not move the middle pane.
- The middle pane already keeps its own folder history (every folder change, including Explore Folder, is a step), reachable only from Go → Back in Library / Forward in Library and the Workspace menu's Previous folder / Next folder. Back and forward buttons in the middle pane's header would expose it.

Verified offscreen only; the native macOS panel's highlighting of the selected folder is up to macOS.
