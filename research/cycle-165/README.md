# Cycle 165 - qpdf, page edits, safe saving, notes renumbered

Build 165 (`0.3.0-dev60`), 19 November 2026. First build cycle of Phase 3 of the PDF programme ([docs/pdf-plan.md](../../docs/pdf-plan.md) section 4.4, row "165" in section 5). Built in three parts by a lower-cost model, checked, recorded and released by Claude Sonnet 5.5. **There is no user interface: cycle 166 adds it, so nothing here is reachable by the user yet.**

## What was built
- **`PdfEditor`** (`src/pdfeditor.{h,cpp}`, the only app code using qpdf): `check` (Damaged, NeedsPassword, Signed), `rotate`, `remove`, `move`, `insert`, `extract`, each on in-memory bytes and returning `Result{ok, error, bytes, pages}`, where `pages` maps every output page to its source page (-1 = inserted) and the rotation applied. Edits refuse damaged, password-needed and (unless allowed) signed PDFs; empty or out-of-range selections and removing every page are errors, never silent no-ops. Output keeps encryption, metadata, outlines, links and annotations as qpdf does.
- **`PdfSession`**: pending edits on a working copy against a baseline (the bytes last read or saved). Invokable `rotatePages`, `removePages`, `movePages`, `insertPdf`, `undoPageEdit`, `redoPageEdit` (0-based working pages); properties `pagesModified`, `canUndoPageEdit`, `canRedoPageEdit`, `pageEditError`, `signedEditsAllowed`, `changedOnDiskWhileEditing`. Undo holds exact bytes (20 states / 256 MB). One translation place (baseline/working page, point and size) used by `PdfMarks` when drawing and writing.
- **`Backend`**: `savePdf(force)`, `exportPdfPages(url, pages)`, `pdfSaveError`, `pdfSaveNotice`, `pdfRecoveryFolder()`. `PdfNotes::renumber` rewrites the notes; `src/pdffiles.{h,cpp}` does the verified write.
- **Packaging**: qmake with pkg-config (`libqpdf`), not CMake; CI installs `qpdf pkgconf`. The Mac bundle gains `libqpdf.30.dylib` and `libcrypto.4.dylib`; `licenses/qpdf-Apache-2.0.txt` and a third-party notice section were added; `bin/check-licenses` fails when qpdf is bundled without its notice.

## Save sequence
1. Nothing pending: true. 2. Read the file; if it differs from the baseline and not `force`: "The PDF changed on disk. Your page changes were not saved." 3. `PdfFiles::writeVerified`: refuse read-only; temporary `.fomawrite-XXXXXX.tmp` next to the real file; write, flush, fsync; read back and check the bytes equal and Qt PDF opens them with the expected page count; keep the previous bytes; copy permissions; re-read the file to check it is unchanged; replace (POSIX `rename`; Windows `MoveFileExW`). Any failure removes the temporary file and this attempt's copy and leaves the original, session and notes untouched. 4. Success: notes renumbered from the page map as one editor undo step, the working state becomes the baseline (the file watcher finds the file equal to the baseline and does not reload, so undo survives), then the notes file is saved if it has a target.
- **Previous copies**: `<AppData>/pdf-previous/<sha1 of path, 20 hex>/<timestamp>.pdf` plus `path.txt`; the latest 3 per file. Export also keeps the bytes of a file it overwrites.

## Decisions
- Pending edits keep the saved PDF's page numbers until save; the pane and marks translate at draw time. The notes are renumbered only at save (markers `page=`, `at=`, `rect=`, links to this PDF, `## Page N` headings; removed pages become `page=0` and `## Deleted page N`).
- `pagesModified` is true for rotate-and-back. No-op edits record no undo step.
- Qt PDF text indices do not change with rotation (tested), so marks need only the page number translated.
- Disk change with pending edits is flagged, not reloaded.
- `Backend::failNextPdfWrite` is a plain C++ method, not invokable from QML, and arms only for one call; not reachable by a user.

## Verification
- `bin/test`: **416 passed, 0 failed, 0 skipped** (384 before; 32 new: 9 `pdfEditor*`, 10 `pdfPages*`, 13 `pdfSaving*`). Editor: 334 passed; typecheck clean; `npm run build` leaves `dist/` unchanged.
- Red/green (each mutated, run, restored): remove-all guard off, `rotatePage(turn,false)`, `isSigned` false (editor tests fail); rotation not composed, wrong 90-degree transform, `mark.page = page+1` (pages tests fail); `at=` not turned, no recovery copy, removed page keeps its number, temporary file kept, no `adoptWorkingAsBaseline` (saving tests fail 3, 5, 2, 3, 7).
- Review of `546e2a6..HEAD` for four risks: no pending edits keep the identity map (build 164 behaviour; existing pdf tests pass); the test hook is inert; the original is replaced only after the new file is verified and the previous bytes are kept, with no temporary file left on any failure path; the save leaves the file equal to the baseline so the watcher does not treat it as external (a test waits past the debounce and checks a later outside change is still noticed).
- Self-check, headless: `pdfChecks` 2 pass, `qmlWarnings` empty, everything passes except the five known headless `publishingThemeChecks`. No new self-check group this cycle.
- Packaging: `bin/package-mac` exit 0, `dist/Fomawrite.app` 524 MB; `bin/check-licenses` passes; `otool -L` of the packaged binary shows `@rpath/libqpdf.30.dylib` and `libqpdf.30.dylib` is in `Contents/Frameworks`; nothing resolves to /opt/homebrew.

## Known gaps
- No interface yet, so nothing is reachable by the user.
- Windows build and packaging of qpdf are not done in this commit; the Windows replace code (`MoveFileExW`, `FlushFileBuffers`) is written but not yet built or run.
- After a save, undoing the notes renumbering in the editor (Cmd-Z) leaves the notes with old page numbers against the saved PDF, so marks can land on wrong pages. To be designed in cycle 166 before the interface makes saving reachable.
- Marking a page inserted from another PDF is refused until saved.
- Sections in the notes are renumbered but not re-ordered after a page move.
- Editing a password-protected PDF with its password supplied is untested (a locked PDF is refused).
- PDFs over 512 MB refuse edits.
- Also: stale hand-edited `page=` values are renumbered as if current; nothing prunes recovery folders of PDFs that no longer exist.

## Not installed
`/Applications/Fomawrite.app` and the Windows VM are build 164.

## Cycle 166 must
Call `savePdf()` from Save in PDF mode and show `pdfSaveError` / `pdfSaveNotice`; on "changed on disk" ask whether to overwrite (`savePdf(true)`); ask before editing a signed PDF and before closing with pending page edits; use `exportPdfPages` for Export; tell the writer where previous copies live; and design the Cmd-Z renumbering gap.
