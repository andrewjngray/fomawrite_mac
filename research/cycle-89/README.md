# Cycle 89 — library context-menu audit

3 October 2026. Andrew reported that Open in New Tab and Open in New Window did nothing for the currently open library file.

The session manager reused an existing editor for every open request, including explicit new views. Explicit requests now create an additional editor; ordinary Finder/open requests still focus an existing editor. Native tab creation targets only the newly created window and selects it after AppKit groups the windows. Saved workspace entries can retain separate views of the same file.

Additional views open the saved document. If that document has unsaved edits, the menu asks the writer to save first. Views have independent editing buffers, not live synchronized cursors or edits. A view refuses to overwrite a changed disk baseline, even if the other view later closes. Reload remains usable with multiple views. Rename/Trash ask the writer to close other views first. Copy/export from the active view use that view's draft, not an arbitrary window's text.

Other fixes: New Tab now validates local readable text files just like New Window, opening failures show a dialog, disappeared targets are disabled when the menu is rebuilt, Favorite recognizes canonical path aliases, and Finder/Share failures surface instead of reporting success silently. Sharing a dirty document asks for Save first so the file being shared contains the latest edits.

## Audit

| Menu action | Evidence |
| --- | --- |
| Open in New Tab / Window | Actual QML actions in the production session manager, compiled as an isolated Cocoa integration test: separate window, correct file, correct native tab group, unrelated window unchanged. Ordinary opening still reuses an existing editor. |
| Get Info | QML action opens its information dialog for the clicked target. |
| Favorite / Remove Favorite | QML actions and canonical-path state checked. |
| Duplicate / Rename / New File / New Folder | QML actions open the name dialog; submission changes the disposable target and preserves an unrelated unsaved draft. Existing tests cover invalid names, collisions, folder relocation and metadata. |
| Move to Trash | Cancel preserves the fixture; acceptance moves only the disposable fixture. Dirty and multi-view guards preserve source. |
| Show in Finder | Native reveal request accepted for the disposable sample; missing-target errors are surfaced. Finder's selection is not inspected through accessibility. |
| Share | Native sharing picker request accepted for the sample; no service selected and nothing sent. Third-party share providers remain a manual check. |
| Export HTML / PDF | Selection dialog opens; both backend exports write real files from the clicked document; PDF header checked. Destination-picker interaction remains manual. |
| Print rendered / source / paginated preview | Each QML submenu action opens and cancels its native dialog under Cocoa. No print job submitted. |
| Copy Path / Markdown / Plain Text / HTML | QML actions verify clipboard contents and HTML MIME data against the clicked file. |
| Sort By | Name, created date, extension, both directions and folder pinning dispatched; existing sort tests cover field/direction behavior. |
| View Options | Excerpts, sort/filter bars, date hiding and tree/list navigation dispatched; existing tests cover date and layout settings. |

`./bin/build` and `./bin/test` pass: **118 tests, zero failures**. `./bin/test-window-routing` runs the real session manager with a compile-time test harness, disposable settings/documents and native Cocoa windows; all assertions pass. It does not read the user's workspace or replace an app bundle. The offscreen runner cannot dismiss macOS print dialogs reliably, so print cancellation is checked in the native harness instead.

The test log summaries are adjacent to this file. Cycle 88's broader typography/display-scale acceptance remains open; this menu correction does not close it.

Deployment: the new packaged app is `dist/Fomawrite.app`. The installed Applications and stable Dev copies require a normal quit before replacement if still running.
