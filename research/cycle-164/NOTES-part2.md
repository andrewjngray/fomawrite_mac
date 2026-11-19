# Cycle 164 part 2: re-anchoring, unplaced list, marker chips in Live (developer notes)

No version bump, no README/CHANGELOG/STATUS, not pushed.

## Design choices
- Re-anchoring (brief A) was already built in cycle 163 (`PdfNotes::place`: stored range if its text equals the quote, else nearest occurrence on the page, else pages +1,-1,+2,-2,+3,-3, else unplaced) and `PdfMarks` only draws placed items and never writes. Part 2 adds tests for it and one change: a `note` whose `at=` lies outside its page (x or y < 0 or beyond the page size) is now unplaced; part 1 clamped it. `placeNote`/`moveNote` still clamp what they write.
- Placement is resolved in `PdfMarks::refresh` (30 ms after the notes text or PDF changes), not per frame; page text is cached per page by `PdfSession`, and only the pages an item needs (its own, then neighbours when the quote is not there) are extracted.
- Removal, click-to-notes and notes-to-page already use the placed items entry (drawn page and range), so they act on a re-anchored mark at its drawn position (tested: removing a re-anchored underline).
- Unplaced list: `PdfReader.unplacedItems` (highlight/underline/strike/note items with `placed` false; unknown kinds are not listed). Toolbar button `pdfUnplacedButton` ("N unplaced", only when N > 0, counted in the fold width) opens `PdfUnplacedList.qml`, a Popup in the Inspector's look (palette popover, 10 px radius, border). Row: "Kind · p. N  start of quote or note" (60 characters; "no page" when the marker has none). A click closes the popup, switches PDF-only to Split (`editorNeeded`), then `revealInNotesRequested` (part 1's page-to-notes path: caret at the item's body start, no focus). The caret scroll now waits for a layout change in progress (`pdfCaretScrollTimer` in Main.qml); before it, the notes did not scroll after PDF-only to Split.
- Chips (brief C): `src/editor/src/pdfchips.ts`, a ViewPlugin like extras.ts, active only in Live (`view.plugin(liveExtension)`), working on `CommentBlock` syntax nodes, so markers in fences or indented code are text. Patterns copy `pdfnotes.cpp` (up to 3 leading spaces, whole line). Labels: "Highlight · p. 3", "Underline", "Strikethrough", "Note" (pin glyph), unknown kind capitalised ("Ink · p. 4"), header "PDF notes" (outlined, quiet). Colour dot for text marks uses the pane's colours (red default for underline/strike). CSS in `blocks.css`, light and dark.
- Conventions as other Live widgets: the chip is an atomic replace over the marker; it gives way to the raw marker (line class `fw-pdf-marker-src`) while any selection touches the line; clicking a chip puts the caret at the line start (reveals the raw marker); copy, selection and deletion work on the document text. Chips never change the document. Part 1's `cursorMoved` mapping makes entering a chip reveal the item on the page; not separately tested in Qt (part 1's Live test covers the path).

## Files
`src/pdfmarks.cpp`, `src/PdfReader.qml`, `src/PdfToolbar.qml`, `src/PdfUnplacedList.qml` (new, in `resources.qrc`, `documentviewcheck.cpp`, `bin/check-document-views`), `src/Main.qml`, `src/editor/src/pdfchips.ts` (new), `src/editor/src/main.ts`, `src/editor/blocks.css`, `src/editor/dist/editor.js` (rebuilt), tests below.

## Tests
Qt (`tests/cycle164-pdf-anchoring.inc`, included after the notes include): `pdfReanchoredMarksAreDrawnAtTheQuoteAndRemovable` (shifted on the page, moved two pages on, text and file unchanged, removal), `pdfUnplacedItemsAreNotDrawnAndAreListed` (quote gone, 5 pages away, page 9, note on a missing page, note left/right of the page; button text; list rows; indicator disappears), `pdfClickingAnUnplacedRowRevealsItInTheNotes` (PDF-only switches to Split; caret and scroll). Editor (`test/pdfchips.test.mjs`, 6): parse, chips per kind and header, document unchanged, raw while touched, code is not a chip, Source draws none.
Targeted run: the 60 earlier pdf* tests plus 3 = 63 passed, 0 failed. Editor: 334 passed; typecheck clean; build reproduces `dist/`.

## Red/green (each reverted)
1. `PdfNotes::place` without the quote search: pdfReanchored... fails (mark not placed).
2. Note bounds check replaced by `true`: pdfUnplaced... fails (4 unplaced, not 6).
3. `revealUnplaced` made a no-op: pdfClickingAnUnplacedRow... fails (caret stays 0).
4. Chip gate on Live removed: "Source mode draws no chips; Live does" fails.

## Known gaps
- Not seen on screen: chip look, popup look, dark mode. The popup animates open; tests wait for it to settle.
- No Qt test of a chip click or caret on a chip in a live Live page (editor tests are on the builder; the reveal path is part 1's).
- A list row for an item whose marker has no body (empty note) puts the caret just after the marker line.
- The list is not refreshed while open except to close itself when empty; no keyboard navigation inside it.
