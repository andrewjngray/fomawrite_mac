# Cycle 44: file and folder context menus

See docs/build-cycles.md for scope, safety semantics and limitations.

Build and 73 tests pass. Native sample verification:
- Both complete menus fit and show only the appropriate actions; submenus open individually.
- Duplicated Note.md to Note copy.md, then moved the new copy to system Trash through its confirmation dialog. Original content unchanged.
- New File on Folder created Folder/Created.txt.
- Open in New Tab added Note.md to the existing native group (three tabs).
- Get Info reported the clicked 70-byte sample, path and dates.
- Print → Paginated Preview showed Context menu sample while Long-notes.md remained active in the main editor.
- Copy → Markdown pasted the sample heading into the library filter, then the filter was cleared.
- Share opened the native picker with the sample text document; cancelled without sending. No screenshot of sharing contacts saved.

Screenshots show sample writing only. Standard app and Dev are refreshed from the same source; compiled artifacts are excluded from Git. The RC1 release ZIP is unchanged.

Not exercised: real print/sharing delivery, full dark/VoiceOver/keyboard matrix, filesystem failures on physical media, simultaneous editing of a target in the separate everyday/Dev processes. Folder copies over 1 GB/20,000 items or containing links require Finder. Get Info reports basic metadata rather than Finder's full permissions editor.
