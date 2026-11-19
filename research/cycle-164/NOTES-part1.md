# Cycle 164 part 1: note pins and page/notes navigation (developer notes)

Scope: plan §4.1-4.3 and brief items A1-A6, B7-B8. Not built (part 2): re-anchoring changes, the "unplaced" list, marker chips in Live. No version bump, no README/CHANGELOG/STATUS.

## Design choices
- Marker `<!-- fomawrite:pdf note page=N at=X,Y -->`; X,Y integers in page view points (same space as the 163 overlays, so rotation needs no extra maths). `at` may be negative on parse; drawing and edits clamp to the page.
- A new note = marker + one empty text line (+ the usual separator), placed at the end of its page section (after text marks and notes). The caret goes on the empty line (`PdfMarks::placeNote` returns it). At end of a file without a final newline the empty line cannot exist (caret at end); insert-then-remove restores bytes for texts that end in a newline or are empty (tested), not for that edge.
- Pin = 22 px square, yellow, "note" LineIcon, centre at `at`. Page is always white, so fixed colours (selection ring uses `palette.focus`, as the selection rects do). Popover is a child of the pin, plain text (800 chars max), flips left near the page edge.
- Hit-testing is in the page MouseArea (pins first, then marks, then links); a pin press that moves >3 px is a drag (centre follows pointer with grab offset, clamped), written on release only.
- Remove: non-empty note: marker line goes, plus one blank line right after it if there is one; text stays. Empty note: removed whole (marker, its empty line, the separator before it, a generated page heading and header line when it was the last item).
- Page -> notes: plain click (not drag/link) on a mark or pin sets the caret to the item's body start (`quoteStart`, or first non-blank line after the marker) WITHOUT taking focus (the page keeps the keyboard so Delete works). Source uses `ensureCursorVisible` (minimal scroll, not centred). PDF-only: select only.
- Notes -> page: caret moves (Source `cursorPositionChanged`, Live `cursorMoved`) are mapped to an item by marker line (`PdfMarks::itemAt`); a change of item reveals it (`PdfReader.revealItem`: minimal scroll with 24 px margin, none if fully visible; 1.1 s flash ring). Typing inside an item does nothing. Loop/insert guard: `quietPdfNavigation()` (600 ms) is armed by our own caret moves, by `PdfMarks::editStarting` (every mark/note edit) and on PDF open; caret events inside it only re-baseline.
- Selection of a pin is by marker text (`selectedNoteMarker`); after a move it follows the new marker.
- Escape order: disarm tool, deselect pin, clear text selection, leave marking mode. Delete/Backspace remove the selected pin.
- Ctrl-Cmd-N (`Meta+Ctrl+N`) collides with nothing (test scans every item). Windows bar entry is not added (same as 163).
- Choices where the brief was silent: a click on the page with the tool armed places on release (no drag); armed drags do nothing; no popover for marks; unplaced notes (bad page or missing at=) are not drawn.

## Files
`src/pdfnotes.{h,cpp}` (Item gains hasAt/atX/atY/bodyStart/contentEnd/hasBody; insertNote, moveNote, noteMarkerLine; insertMark refactored onto shared `insertItem`; removeMark handles notes), `src/pdfmarks.{h,cpp}` (note items, placeNote, moveNote, bodyPosition, itemAt, editStarting), `src/PdfReader.qml`, `src/PdfToolbar.qml`, `src/LineIcon.qml` ("note"), `src/NativeMenuBar.qml`, `src/Main.qml`, `tests/cycle164-pdf-notes.inc`, `tests/tst_fomawrite.cpp`. Nothing under `src/editor`.

## Tests (tests/cycle164-pdf-notes.inc)
pdfNotesNoteMarkerRoundTrip, pdfNotePlacedByClickIsOneUndoStepInSource, ...InLive, pdfNotePinLandsWhereClickedOnEveryRotation, pdfNotePopoverMoveAndRemove, pdfClickOnMarkOrPinShowsItsItemInTheNotes (Source, PDF-only, Live), pdfCaretIntoAnItemShowsItOnThePage, pdfDragSelectingFromAHighlightStillSelects, pdfNoteToolButtonMenuItemAndShortcut. Menu found through `Cycle161Menus`; Live tests wait for the bridge and page.
Targeted run: the 49 existing pdf* tests (cycles 160-163) plus these 9 = 60 passed, 0 failed (whole suite not run).

## Red/green (each reverted afterwards)
1. `revealPdfItemInNotes` made a no-op: pdfClickOnMarkOrPinShowsItsItemInTheNotes fails (caret not at the quote).
2. `pdfReader.revealItem` call removed from `pdfCaretMoved`: pdfCaretIntoAnItemShowsItOnThePage fails (pane does not scroll).
3. `PdfNotes::moveNote` inserts instead of replacing the at= value: pdfNotesNoteMarkerRoundTrip and pdfNotePopoverMoveAndRemove fail.
4. `quietPdfNavigation` made a no-op: pdfCaretIntoAnItemShowsItOnThePage fails (click on a mark re-scrolls/flashes the page).

## Known gaps
- Not seen on screen: pin look, popover placement, flash, drag feel, dark window. Live caret scroll relies on the page's own reveal.
- Dragging a pin to another page is not supported (clamped to its page). Pin hit area is the 22 px square only; pins overlapping marks win clicks.
- Drag-selecting into blank page space (past the end of the text) selects nothing; pre-existing.
- A note typed in PDF-only layout: the layout switches to Split first; in a narrow window Split may still show one pane.
- Windows menu entry for Note not added; self-check (`--check-document-views`) not extended.
