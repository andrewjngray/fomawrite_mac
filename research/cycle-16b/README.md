# Cycle 16b — Move To

20 September 2026. Implements local File → Move To… (F10). Keeps the filename, unsaved text and undo. Saved disk bytes are copied without overwrite, read back and compared; source is rechecked before removal. Failed source removal retains both files and the original active path. Successful move updates URL/base URL, recovery, watcher and favorites/recents. Relative linked assets are not moved.

## Checks and artifacts

- Build passed: logs/build.log.
- Final suite: 33 passed, zero failed, logs/test.log. Covers QML picker open/accept, Unicode, no-op same folder, missing/nonlocal destinations, file/dangling-link collisions, failure to remove from a read-only source directory, dirty buffer, undo/redo/save, recovery snapshot, organizer paths and external-change watching. Initial synthetic test failed because selectedFolder was set before opening the picker; the corrected test opens it first. Existing Qt teardown warnings remain.
- Stable Dev prepared after normal clean close: logs/prepare-dev-app.log.
- Packaged app and strict/deep signature verification passed: logs/package-mac.log and logs/package-signature.log.
- Runnable: dist/Omawrite Dev.app and dist/Omawrite.app. No per-cycle app identity created.

## Live app results

Initial automatic review rejected app inspection due to possible private content. Andrew explicitly approved inspection and preservation of unsaved work. The prior sample was clean and closed normally. Native Open and folder pickers left Open disabled even with a valid path; canceled without modifying files. Move To now uses the Qt built-in folder picker on macOS only. Its navigation, path entry (Command-L), destination selection and collision error worked. Screenshot [move collision](screenshots/move-collision.png) visually reviewed and contains only a sample with library hidden. Collision fixture verified unchanged then removed. Source examples/Cycle16-QA-renamed.md remained in its original location.

During the next QA action the tool reported user interaction, and the refreshed window showed the user's own document. Further UI actions stopped. No private screenshots saved. Thus successful live-app move, unsaved live move and recents reopen are still pending; automated equivalents pass.

## Limits

Separate physical/mounted-volume testing, disk-full/disconnection, crash/relaunch recovery, dark/narrow appearance and full keyboard audit remain unverified. Copy verification uses the same code path across volumes but does not eliminate a concurrent writer changing a file after the final check. QFile copy preserves file permissions; extended metadata/ACL fidelity is not guaranteed here. Cross-window document ownership remains Cycle 25 work. Native Open picker issue remains tracked; only Move To uses the fallback in this cycle.

Changes remain uncommitted alongside earlier cycles. Next: finish live Move To exercise, then Cycle 17 search/edit menus. 21 listed feature increments remain. [Optional exercise](../usability/cycle-16b.md).
