# Cycle 98 keyboard and workspace review

Use the latest Fomawrite Dev build with a disposable document. This checklist distinguishes remaining human acceptance from the automated/native fixture evidence in [Cycle 98](../cycle-98/README.md).

1. In a wide split workspace, press F6 repeatedly (Fn+F6 if the Mac function row uses media keys). Focus should move through visible workspace regions; Shift+F6 reverses direction. Tab still edits in Source and traverses controls once focus has left the editor.
2. Choose the fading toolbar option, then use F6 to reach the toolbar. Its actions should become visible. Confirm each focused control has a clear ring and descriptive tooltip.
3. Focus a file-list control, narrow to 720 logical pixels, and confirm focus returns to the visible editor. Use Workspace to open Files; focus should begin on Close navigation. Escape should restore the control that opened it.
4. Switch to read-only Preview, use temporary Files navigation and close it. Confirm keyboard focus remains in visible UI. Repeat with Visual Edit and with an unsaved draft; cancel any change-document prompt and check the draft remains intact.
5. Open another native window and tab. Give each different pane widths, enter/leave full screen, switch the active window and return. Check selection and Undo, then close/reopen saved documents to inspect restoration.
6. Remaining human checks: listen to VoiceOver names and reading order; review actual normal/scaled macOS display settings, moving across displays, fullscreen/traffic lights, and the inactive/dark appearance with your usual window arrangement. Record any issue with the build, pane mode, appearance and logical window size.
