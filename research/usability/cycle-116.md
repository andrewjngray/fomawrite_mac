# Cycle 116 — everyday writing review

Target: **0.3.0-dev12 / macOS 0.3.0 (116)**. All 173 regressions, 13 focused native checks and all three local app checks pass. Consult the [handoff record](../cycle-116/README.md) for identities and the separate activation-blocked native scenarios. Confirm the running version/path in About before testing.

Use a disposable Markdown document with Unicode prose, an ordinary inline link, a simple table and an unsaved sentence.

1. Select a word in Source. Click Bold, then Undo; try the keyboard shortcut and paragraph menu. Each should format the intended selection in one Undo operation. Focus Find, Replace, outline search, a library filter or a link field and try the same commands: they should leave the document alone. Click back into Source to format again.
2. Select ordinary text and insert a link with a local destination containing spaces and an optional title. Reopen the link: the dialog should say Edit link/Save and show the original fields. Cancel/Escape should return to the unchanged selection. Undo/Redo should affect only the link operation.
3. Open a link, then change its Source text outside the dialog. Apply should refuse the old target while keeping your entries and explaining how to reopen safely. Try a narrow window and dark appearance; all fields and both actions should remain reachable.
4. In Visual Edit, click a supported table cell. Tab should move forward, Shift-Tab backward, including into empty cells. The first/last cell should stay within the table. Type into a cell and Undo/Redo; compare exact Markdown, including delimiters and alignment. F6 should leave the writing surface. Use Source for row/column restructuring.
5. Repeatedly switch Source, Split, Full and Visual Edit while resizing from a broad workspace to a narrow, short window. Check stationary footer actions, navigation restoration, independent zoom and the current reading position. Open Export, narrow it, scroll horizontally to the right-hand bands, then Cancel without saving.
6. With the app active, use F6 and Shift-F6 through Source, Preview and toolbar in both directions. Try full-screen entry/exit, focus switching between app windows and compact navigation Escape. These native scenarios were blocked by foreground-activation refusal in the wider fixture and need hands-on review.
7. Save or deliberately discard the disposable work. Report the mode, focused field, zoom, window size and action if a command changes the wrong text or a control becomes unreachable.

Physical typing, trackpad, IME and VoiceOver observations add coverage beyond the synthetic native checks. Typewriter scrolling currently applies to Source only.
