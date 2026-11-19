# Cycle 164 - Notes on the page, navigation, unplaced list, Live chips

Build 164 (`0.3.0-dev59`), 19 November 2026. Second build cycle of Phase 2 of the PDF programme ([docs/pdf-plan.md](../../docs/pdf-plan.md), row "164" in section 5). Built in two parts by a lower-cost model from a written brief; checked, recorded and released by Claude Sonnet 5.5. The programme is planned and reviewed by Claude Fable 5.2.

## What it does
- **Note pins.** A toolbar button (`pdfNote`), the PDF menu item Note (Ctrl-Cmd-N) arm a tool; the next click on a page places a note there. It is written to the notes as `<!-- fomawrite:pdf note page=N at=X,Y -->` followed by one empty text line, with the caret on that line (the PDF-only layout switches to Split first). The pin is a 22 px yellow square with a note icon, centred at the point, the same size at any zoom. Click a pin to select it and show its text in a popover; drag to move it (written on release, one edit); Delete or Backspace removes the selected pin; Esc disarms, deselects, clears the selection, then leaves marking mode, in that order.
- **Page and notes follow each other.** A plain click on a mark or pin puts the notes' caret at the start of that item's body without taking the keyboard from the page. The caret moving into a different item in the notes (Source or Live) scrolls the page just enough to show it and flashes it for 1.1 s. Typing inside one item does nothing. A 600 ms quiet window (`quietPdfNavigation`) after the window's own caret moves, after every mark or note edit and on PDF open keeps the two directions from answering each other.
- **Unplaced list.** The toolbar shows "N unplaced" when marks or notes cannot be drawn (quote gone, more than 3 pages away, note on a missing page, `at=` outside the page). It opens `PdfUnplacedList.qml`; a row shows the item in the notes (PDF-only switches to Split first).
- **Marker chips in Live.** Lines `<!-- fomawrite:pdf KIND page=N ... -->` are drawn as a small chip ("Highlight - p. 3", "Note" with a pin glyph) and the header line as a quiet "PDF notes" chip. Source draws none. A chip gives way to the raw marker while a selection touches its line; clicking it puts the caret on the line. Chips never change the document.

## Design choices
- `at` is two integers in page view points (the space of the cycle 163 overlays, so rotation needs no extra maths). A note's marker, empty line and the separator are inserted as one edit; the new note goes at the end of its page section.
- Removing a note with text removes the marker line (and one blank line after it) and keeps the text; an empty note is removed whole, with a generated page heading and the header line when it was the last item.
- Re-anchoring was already in cycle 163 (stored range, then nearest on the page, then pages +1, -1, +2, -2, +3, -3, else unplaced). Cycle 164 adds tests and one change: a note whose `at=` lies outside its page is unplaced (placing and moving still clamp). Placement is resolved in `PdfMarks::refresh`, never per frame, and never writes.
- Hit tests in the page MouseArea: pins, then marks, then links. Pins keep fixed colours (the page is always white); the selection ring uses `palette.focus`.
- Chips are a ViewPlugin on `CommentBlock` nodes, so markers in code stay text; patterns copy `pdfnotes.cpp`.
- No new self-check group this cycle. `PdfUnplacedList.qml` was added to the file lists of `documentviewcheck.cpp` and `bin/check-document-views`.

## Changed files
`src/pdfnotes.{h,cpp}`, `src/pdfmarks.{h,cpp}`, `src/PdfReader.qml`, `src/PdfToolbar.qml`, `src/PdfUnplacedList.qml` (new), `src/LineIcon.qml`, `src/NativeMenuBar.qml`, `src/Main.qml`, `src/editor/src/pdfchips.ts` (new), `src/editor/src/main.ts`, `src/editor/blocks.css`, `src/editor/dist/editor.js` (rebuilt), `src/resources.qrc`, `src/documentviewcheck.cpp`, `bin/check-document-views`, `tests/cycle164-pdf-notes.inc`, `tests/cycle164-pdf-anchoring.inc`, `tests/tst_fomawrite.cpp`, `src/main.cpp`, `macos/Info.plist`.

## Verification
- `./bin/test`: **384 passed, 0 failed, 0 skipped** (372 before; 12 new).
- Editor: **334 passed, 0 failed** (6 new in `src/editor/test/pdfchips.test.mjs`); `npm run typecheck` clean; `npm run build` leaves `dist/` unchanged.
- New Qt tests, 12: `pdfNotesNoteMarkerRoundTrip`, `pdfNotePlacedByClickIsOneUndoStepInSource`, `...InLive`, `pdfNotePinLandsWhereClickedOnEveryRotation`, `pdfNotePopoverMoveAndRemove`, `pdfClickOnMarkOrPinShowsItsItemInTheNotes`, `pdfCaretIntoAnItemShowsItOnThePage`, `pdfDragSelectingFromAHighlightStillSelects`, `pdfNoteToolButtonMenuItemAndShortcut`, `pdfReanchoredMarksAreDrawnAtTheQuoteAndRemovable`, `pdfUnplacedItemsAreNotDrawnAndAreListed`, `pdfClickingAnUnplacedRowRevealsItInTheNotes`.
- Red/green (each reverted): `revealPdfItemInNotes` a no-op fails the click test; removing `revealItem` from `pdfCaretMoved` fails `pdfCaretIntoAnItemShowsItOnThePage`; `moveNote` inserting instead of replacing fails `pdfNotesNoteMarkerRoundTrip` and `pdfNotePopoverMoveAndRemove`; `quietPdfNavigation` a no-op fails `pdfCaretIntoAnItemShowsItOnThePage`; `PdfNotes::place` without the quote search fails the re-anchoring test; the note bounds check replaced by `true` fails the unplaced test (4, not 6); `revealUnplaced` a no-op fails the row test; the Live gate on chips removed fails the editor test "Source mode draws no chips; Live does".
- Review of the diff (Claude Sonnet 5.5) for three risks: no PDF open (the caret handlers return unless a PDF is shown; the scroll timer starts only from PDF-driven calls); chips match only `fomawrite:pdf` marker lines and only decorate; no loop or focus steal in reveal/flash (flash and reveal never take focus; only the writer's own note placement focuses the notes). Nothing found to fix.
- Self-check, headless: `pdfChecks` 2 groups pass, `qmlWarnings` empty, every other group passes; the same five `publishingThemeChecks` fail headless as on every earlier build. No new group was added.

## Known gaps
- **Nothing has been seen on screen**: pin, popover, flash, chips, unplaced list, dark mode. The popup animates open; tests wait for it.
- Placing a note and then removing it restores the bytes exactly only when the notes end in a newline (or are empty).
- No Qt test of clicking a chip in a running Live page (the editor tests are on the decoration builder; the reveal path is covered by the Live test of `pdfClickOnMarkOrPinShowsItsItemInTheNotes`).
- No new self-check group.
- Marks can only be re-anchored within 3 pages either way.
- A pin cannot be dragged to another page; its hit area is the 22 px square, and pins overlapping marks win clicks. Unplaced notes are not drawn.
- A row for an item with no body (an empty note) puts the caret just after the marker. The unplaced list does not refresh while open and has no keyboard navigation.
- In a narrow window Split may still show one pane. The Windows menu entry for Note is added by the Windows merge, without a shortcut.

## Not installed
`/Applications/Fomawrite.app` and the Windows VM are build 163; build 164 exists in source and in `build/` only.

## CI follow-up: GitHub macOS runner flake

`pdfClickOnMarkOrPinShowsItsItemInTheNotes` failed once on GitHub's macOS runner (run 44460688304) at the PDF-only pin click (`selectedPin` still null). Cause: test timing only. `cycle164Click` computed the click point after two fixed 30 ms waits, right after the layout switched to PDF only, so on a slow machine the click landed on blank page (which clears the selection). The quiet window never suppresses selecting a pin, and the product code is unchanged. Not reproduced locally, even with the machine loaded.
Fix: `cycle164Click` now waits until the point is stable inside the viewport, the PDF-only click waits for the pin to be drawn and uses `QTRY_VERIFY`, and two immediate note-count asserts after a click became `QTRY_COMPARE`.
