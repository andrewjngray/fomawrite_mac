# Cycle 165 part 2a: pending page edits, marks follow them (notes for part 2b)

## API (`src/pdfsession.h`)
Properties (change signals): `canUndoPageEdit`, `canRedoPageEdit`, `pagesModified` (all `pageEditsChanged`); `pageEditError` (`pageEditErrorChanged`); `signedEditsAllowed` r/w (`signedEditsAllowedChanged`, reset to false when another PDF opens or on close); `changedOnDiskWhileEditing` (`changedOnDiskChanged`).
Invokable: `bool rotatePages(QList<int> pages, int degreesClockwise)`, `removePages(pages)`, `movePages(pages, int before)`, `insertPdf(QUrl file, int before)`, `undoPageEdit()`, `redoPageEdit()`. All indices 0-based, of the working document.
C++: `struct PageOrigin {int origin; int rotation;}`; `baselineBytes()`, `workingBytes()`, `pageMap()`, `baselinePageCount()`, `adoptWorkingAsBaseline()`, `setUndoLimits(states, bytes)` (default 20 / 256 MB), `setPageEditError(QString)`.
Translation (the one place): `workingPageForBaseline`, `baselinePageForWorking`, `rotationOfWorkingPage`, `baselineSizeOfWorkingPage`, `baselinePointToWorking`, `workingPointToBaseline`, `requestBaselinePage(int)`, `linkPage(int)`, `linkLabel(int)`.

## Where things are
- State: `m_baselineBytes`, `m_working` (bytes + map), `m_undo`, `m_redo` (`EditState`). Edits call `PdfEditor` on working bytes, compose `pages` onto the map (`composed()`), show the new bytes through `showState()` (a memory `UnlockedFilePdf`, then `adopt()` with `m_keepEditState`), then push undo. A document that cannot be shown leaves everything unchanged.
- `adopt()` (load, unlock, reload from disk) resets the edit state: baseline = bytes read. `close()` clears it.
- `PdfMarks` (`pdfmarks.cpp`) translates when drawing (`refresh`, `pageText` callback for `PdfNotes::place`) and when writing (`mark`, `placeNote`, `moveNote`, `baselinePoint`). QML is unchanged except that `Main.qml` and `backend.cpp` call `requestBaselinePage` for `#page=N`. `pageLinkAddress` and `markdownQuoteFor` name the baseline page.

## Decisions (brief silent)
- `pagesModified` = working bytes differ from baseline OR the map is not the identity (rotate and back still counts as modified).
- No-op edits (e.g. a move that changes nothing) return true, record no undo step.
- Unplaced items and invalid pins keep their BASELINE page in `page` (the notes' own numbering; the unplaced list shows "p. N" as the notes say).
- Pin positions drawn on a turned page are rounded to whole points. A stale text mark may re-anchor to a neighbouring baseline page (within 3) as before; on a page removed it can therefore land on a neighbour with the same quote.
- On a page from another file only marking is refused (`pageEditError` = "Save the PDF before marking inserted pages."). `pageLinkAddress` for such a page names its working number.
- Headings created for a moved page use the baseline number as label (`linkLabel`) since baseline labels are not read.
- File over 512 MB (not held in memory): edits refused ("too large"). Password-opened PDFs are refused by `PdfEditor::check`.
- `relocate()` with pending edits only moves the URL; it does not reload.
- Disk change with pending edits: flag only (also when the file is gone). With history but no pending edits, a change whose bytes equal the baseline (a save just written) does NOT reload, so undo survives a save. `changedOnDiskWhileEditing` clears on the next load/reload, not on `adoptWorkingAsBaseline`.
- Requested page after an edit is kept and clamped (the session does not know the reader's current page).

## Text indices under rotation
Verified (`pdfPagesTextIndicesDoNotChangeWithRotation`): Qt PDF's `pageText` and `selectionAt(start,len)` are identical after a 90/180/270 turn. Marks need no re-anchoring beyond the page number.

## Tests (`tests/cycle165-pdf-pages.inc`, 10 `pdfPages*` slots)
EditsApplyToTheSessionsDocument, MapComposesAcrossOperations, UndoAndRedoRestoreExactBytesWithinABound, RefusalsLeaveTheStateUntouched, FileChangedOnDiskWhileEditingIsFlagged, AdoptingTheWorkingStateRebasesTheHistory, TextIndicesDoNotChangeWithRotation, MarksAndPinsStayOnTheirContent, MarkingWhilePendingWritesBaselineNumbers, LinksScrollToTheTranslatedPage. Marks tests use `PdfMarks` over a bare `PdfSession` (no window). All 80 pdf* slots pass (82 with init/cleanup).
Red/green (mutate, run, restore): compose without adding rotation: map, marks and marking tests FAIL; 90-degree transform replaced by the 270 one: pin tests FAIL; `mark.page = page+1` instead of baseline: marking test FAIL.

## Known gaps / for part 2b
- No UI; QML draws working pages from `marks.items`, which are already translated. The reader's `placeNote`/`mark` calls pass working pages and are converted.
- Saving must: write `workingBytes()`, rewrite notes `page=` values, `at=` (by each page's rotation delta) and headings from `pageMap()` (removed pages' items have no counterpart; page-inserted pages have origin -1), reload, then call `adoptWorkingAsBaseline()` (the watcher then sees bytes equal to baseline and keeps the history; reload from disk must not be forced). Clear `changedOnDiskWhileEditing` yourself if saving resolves it.
- Not tested: a chip click in Live, QML unplaced list for removed pages (logic only), password-protected editing with a password supplied.
