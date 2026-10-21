# Cycle 153 — Daily-writing follow-through

Automated native evidence is recorded in [the cycle record](../cycle-153/README.md). It uses exposed Cocoa windows and synthetic Qt input in the actual app bundles. The tests cover Source/Live typing and switching, exact save/reopen, undo/redo, pane layouts and zoom, CSS themes, Web/PDF document identity and painted output.

For the next ordinary writing session, use disposable copies where testing destructive edits:

- [ ] Type and select with your usual keyboard/mouse in Live; bold a selection, edit a link, undo and redo.
- [ ] Switch Source → Live → Source and confirm the caret and text remain useful to you.
- [ ] Switch rapidly between documents with different headings in PDF mode; the right pane must always match the selected file.
- [ ] Change output theme and Web/PDF mode, hide/show the preview, then save, quit normally and reopen.
- [ ] Check grammar suggestions and corrections on a realistic long document.
- [ ] Review VoiceOver and a second display if those workflows matter to you.

Record the app build from About, file/mode/theme, the preceding action and whether the issue repeats. The automated pass does not mark these physical-input, accessibility or hardware checks complete.
