# Cycle 162 — PDF links and integration (Phase 1 complete)

Build 162 (`0.3.0-dev57`), 19 November 2026. Third build cycle of the PDF programme ([docs/pdf-plan.md](../../docs/pdf-plan.md), row "162" in §5); it closes the open lists of cycles 160 and 161 and completes Phase 1 (open and read PDFs, with the notes file beside them). Implemented from a written brief by Claude Opus 5.5; reviewed and verified by Claude Fable 5.2.

## What it does
- **Links.** A link to a local PDF, with or without `#page=N`, opens that PDF at that page through the window's open flow (unsaved-changes prompt included): from Go → Open Link at the caret in Source or Live, and from the publishing Preview (which used to pass PDF links to the system, where nothing happened). In the PDF's own notes the link scrolls the PDF beside them, bringing it back if the layout was Notes only. A missing PDF reports like a missing Markdown file.
- **Copy as Markdown Quote** (PDF menu, ⇧⌘C; enabled with a selection): the selection as a tidied blockquote, a blank line, then `[Page N](Report.pdf#page=N)`, using the page label when it differs (`[Page iii](Report.pdf#page=3)`) and a percent-encoded file name.
- **Right-click on a page:** Copy, Copy as Markdown Quote, Find "selection" in PDF, and Copy Link Address over a link.
- **Table of contents.** When the PDF has an outline, the sidebar has a Thumbnails | Contents switch; Contents lists the outline indented, marks the current section and goes to an entry's page. The choice is remembered. The thumbnails button has its own "pages" icon.
- **Restore.** A PDF window is checkpointed by its PDF (also with untitled notes) and comes back in PDF mode with its notes, layout, page and zoom. The Markdown layout is checkpointed separately, so ordinary documents never inherit Notes / Split / PDF.
- **Crash recovery.** Unsaved notes are recovered with their PDF: PDF mode, split, modified, and Save writes `X.pdf.md` without a dialog. Older recovery snapshots still load.
- **Rename, Move To, Move to Trash** for a PDF carry its notes file and their authorship sidecar as one operation: both names are checked first and the PDF is put back if the notes cannot follow. The open window follows a rename or move (unsaved notes, page and layout kept; recents, history and remembered view updated); trashing the PDF a window shows leaves its notes as an untitled draft. Duplicate, Export, Print and text copies stay unavailable for PDFs.
- **From the system.** The Open dialog offers "Markdown, text and PDF"; a Markdown, text or PDF file dropped on the window opens through the usual prompt (file drops were not handled at all before); `Info.plist` declares `com.adobe.pdf` as Viewer with rank Alternate, so Fomawrite is offered in Open With and is never the default (the installer's default-app scripts still name only Markdown and plain text). Show in Finder reveals the PDF.
- **Other windows.** Every "is this open or modified elsewhere" check treats a PDF window as owning both the PDF and its notes.

## Review findings (fixed before commit)
1. **The PDF file was held open while shown.** Qt PDF keeps the file open for the document's life; Windows then refuses to rename, move or replace it, which would break this cycle's Rename on Windows, Phase 3's page saves, and any other program regenerating the PDF. `PdfSession` now reads the PDF into memory (up to 512 MB; larger files keep the old behaviour) and holds no file open. `pdfSessionDoesNotHoldTheFileOpen` counts the process's open descriptors for the file on macOS: 1 before the change, 0 after; Windows CI's rename test is the evidence there.
2. **Show in Finder revealed the notes file** in PDF mode, and was disabled with untitled notes; it now reveals the PDF (not covered by a test: it launches Finder).

## What changed
`src/pdfsession.h/.cpp` (quote text, link addresses, sidebar setting, `relocate`, remembered-view moves, memory loading); `src/backend.h/.cpp` (`movePdfWithNotes`, `trashPdfWithNotes`, `relocatePdf`, `leaveTrashedPdf`, `showsPdf`, ownership checks, recovery `pdfUrl`); `src/workspace.h/.cpp` and `src/main.cpp` (the per-window checkpoint and restore factored into `WorkspaceStore::documentEntry` / `restoreDocumentEntry`, used by `main.cpp` and by tests); `src/Main.qml` (link routing, `revealPdfPane`, checkpointed layouts, drop area, dialogs); `src/PdfReader.qml`, `PdfThumbnails.qml` (now the sidebar), `PdfContents.qml` (new), `PdfToolbar.qml`, `LineIcon.qml`; `src/NativeMenuBar.qml`, `WorkspaceCommands.qml`, `LibraryContextMenu.qml`; `macos/Info.plist`, `bin/prepare-dev-app`; identity lists in `src/documentviewcheck.cpp` and `bin/check-document-views`.

## Verification
- `./bin/test`: **357 passed, 0 failed, 0 skipped** (346 before; 10 new in `tests/cycle162-pdf-integration.inc`, 1 new in `tests/cycle160-pdf-core.inc`), run by the reviewer. `./bin/build` passes.
- Self-check, headless on the built app: the PDF group passes, no QML warnings; all editor, daily-writing, zoom, pane and layout groups pass; the same five publishing groups fail headless as on every earlier build.
- Red/green by the reviewer: memory loading (open descriptor count 1 ≠ 0 with the file path load); the `Info.plist` PDF type (role "" ≠ "Viewer"). By the implementer, each undone in turn: link regex without `pdf`; `revealPdfPane`; the Live caret for Open Link; checkpoint URL (`geometry.pdf.md` instead of `geometry.pdf`); checkpointed layout (2 ≠ 0); recovery `pdfUrl`; `openInNewWindow` and the view-conflict flag without `ownsDocument`; rename/trash refused; drop refused. Copy as Markdown Quote, the context menu and the Contents sidebar are new; the "same page twice" link check and two settings checks are guards that also pass on the old code.

## Limits
- Live has no click-to-follow for links (the page swallows navigation); Go → Open Link at the caret is the route, as for Markdown links.
- The trash tests move files from their own temporary directories into the real Trash, as the existing library test does.
- The command-line and Finder open-event paths in `main.cpp` are not reachable from the suite; they go through `Backend::open`, which routes PDFs.
- A build 161 checkpoint taken in PDF mode stored the PDF layout as the window's layout; one restore of such a checkpoint can still carry it into Markdown. Build 161 was never installed.
- The failure-undo test for rename and trash uses an immutable file and is macOS only.
- Selection across pages, pinch zoom (untested) and softness above about 300% remain as in cycle 161.
