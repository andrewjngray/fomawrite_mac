# Cycle 85 — Studio and writing appearances

The reference is Andrew's Ulysses writing-window screenshot from 27 September. Fomawrite uses original colors and its own existing editor, not copied Ulysses assets. The optional Studio theme separates organizer, file list and writing canvas. Manuscript preserves the previous mono editor; Editorial and Book change source-editor typography without changing Markdown or the selected export style.

Synthetic review captures: [Studio Editorial wide](studio-editorial-wide.png), [Studio Editorial narrow](studio-editorial-wide-narrow.png), [Studio Book](studio-editorial-wide-book.png), [Dark Editorial](studio-editorial-wide-dark.png). The text and test library entries are disposable; no private writing is present.

`./bin/build` passed, followed by the full suite with 116 passing tests after the save/reopen and capture additions. The appearance test verifies source, cursor, Undo, layout and output-style independence, plus exact UTF-8 bytes after save/reopen. The existing source editor applies 140% line height in its block formatting; the new presets alter font, size, measure and top inset.

Known gaps: this is a visual foundation, not general visual editing. Andrew's direct evaluation of typography, tone and display scaling is pending. The wide synthetic image shows an empty sample file list because the test does not point at a private folder.
