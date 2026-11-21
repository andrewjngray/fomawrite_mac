# PDF programme: summary of work so far and what remains

*Written 20 November 2026. The detailed plan is [`pdf-plan.md`](pdf-plan.md); its §9 is the running state record. Each cycle's full record is `research/cycle-NNN/README.md` on `master` (and `README-mac.md` on `windows/port`).*

## 1. Short answer

- **Cycle 165 (build 165) is complete in source on both branches, with its records written and committed.** Nothing is left uncommitted on either branch apart from Andrew's own `examples/` files (Mac) and the `.codex/` folder (Windows), which are deliberately never touched.
- **It is not the end of the build cycle.** Phase 3 (page editing) is only half done. Cycle 165 built the engine (qpdf, unsaved page edits, safe saving, notes renumbering) but **no buttons or menus**: nothing in it is reachable by the user until cycle 166.
- Three small loose ends from 165 are listed in §5.

## 2. What Fomawrite can do now (installed: build 165 on the Mac and in the Windows VM, 20 November)

- Open a PDF in its own pane, beside a Markdown notes file `Report.pdf.md` (Notes / Split / PDF layouts).
- Read: zoom, fit, thumbnails, table of contents, find, go to page, links, Inspector, copy as a Markdown quote, `Report.pdf#page=3` links from any document.
- Mark text: highlight (five colours), underline, strikethrough. Marks are stored in the notes file as invisible HTML comments plus a quote, and undo in the editor removes them from the page.
- Notes pinned on the page: place, drag, popover, remove (the writer's text is always kept).
- Click a mark or pin to jump to its note; move the caret in the notes and the PDF scrolls to the mark.
- Marks whose text has moved are found again (up to 3 pages away); those that cannot be found go in an "unplaced" list.
- In Live, marker lines show as small chips ("Highlight · p. 3"); Source shows the raw markers.

**Not yet looked at on screen by anyone:** the highlighter, pins, popover, flash, chips, unplaced list, and all of it in a dark window. This is the most important check outstanding.

## 3. Cycle by cycle

| Cycle | Build | What | Mac `master` | Windows `windows/port` | Installed |
|---|---|---|---|---|---|
| 159 | — | Phase 0: decisions, five spikes, plan v2 | record only | — | — |
| 160 | 160 | PDF core: `PdfSession`, `PdfPage` item, open routing, notes file, explorer badge | `73212bf` | merged | Mac + VM (as 163/164) |
| 161 | 161 | PDF pane: toolbar, zoom, find, thumbnails, Inspector, PDF menu | `c07177d` | merged | Mac + VM |
| 162 | 162 | Links, contents tab, restore, rename/move/trash with notes, Open With; Windows PDF menu | `4657596` | `a1ddef6`, `cdf47c3` | Mac + VM |
| 163 | 163 | Notes model; highlight, underline, strikethrough | `279d72e`, `8283065` | `1d21369` | Mac + VM |
| 164 | 164 | Note pins, page↔notes navigation, unplaced list, Live chips | `1d2626c`, `85be127`, `546e2a6` | `ad900b3` (+ Note in Windows menu) | Mac + VM |
| 165 | 165 | qpdf page edits, unsaved edits with undo, safe save, notes renumbered at save, export | `543d534`…`24dca0b`, `16e8606` | `9f1de06`, `f7e8a1c`, `7b11ce7`, `2a43400` | **Mac (`0184707`) + VM (`7b11ce7`), 20 Nov (current)** |

### Evidence at the end of each cycle

| Cycle | Mac full suite | Editor tests | Windows CI (x64 and ARM64) | GitHub macOS job |
|---|---|---|---|---|
| 163 | 372 / 0 | 328 | green, run 44440728575 | not run (no machine) |
| 164 | 384 / 0 | 334 | green, run 44461009397 | green on `546e2a6`, run 44467581434 |
| 165 | 416 / 0 (at `24dca0b`; again at `0184707` on 20 Nov) | 334 | green, run 44506354001 | **unproven** on `16e8606` (no machine, twice) |

Every behavioural test was shown failing with the guarded code disabled (red/green), recorded per cycle.

### Cycle 165 in more detail (the engine behind cycle 166)

- `src/pdfeditor.{h,cpp}`: the only code that uses qpdf. Rotate, remove, move, insert, extract; refuses damaged and password-locked PDFs, and signed PDFs unless explicitly allowed.
- `PdfSession`: unsaved page edits applied at once to the PDF on screen, with undo/redo (20 steps or about 256 MB). Notes keep the *original* page numbers while edits are unsaved; marks and pins are translated to where their page now is.
- `Backend::savePdf()`: write a temporary file, check it opens with the right page count, keep the previous file in `<AppData>/pdf-previous/` (latest 3 per PDF), then replace. Any failure leaves the original untouched. On success the notes are renumbered in one edit (`page=`, page headings, pin positions on rotated pages; items on deleted pages get `page=0` and show as unplaced).
- `Backend::exportPdfPages()`: selected pages to a new PDF.
- Packaging: Mac bundles `libqpdf` and OpenSSL 4 (app 524 MB); Windows ships `qpdf30.dll`, `z.dll`, `jpeg62.dll` from vcpkg baseline `9eec002` (qpdf 12.4.2), about 1.1 MB more per package. Licence checks on both platforms require the qpdf notice.

## 4. Where the notes and records are

| What | Where |
|---|---|
| The plan, decisions and running state | `docs/pdf-plan.md` (§8 decisions, §9 state) |
| This summary | `docs/pdf-programme-summary.md` |
| Per-cycle records | `research/cycle-159/` … `research/cycle-165/README.md` (Mac); `README-mac.md` on `windows/port` |
| Release history | `CHANGELOG.md`, `STATUS.md` on each branch (Windows merge notes at the top of its `CHANGELOG.md`) |
| Working rules for cycles | `AGENTS.md` ("What a cycle is") |
| Session resume notes (Claude's memory, outside the repo) | `fomawrite-resume-point.md`, `feedback-cost-and-models.md` |

All of the repository records above are committed and pushed on both branches.

## 5. Loose ends from cycle 165

1. **GitHub's macOS job has not run on `16e8606`.** It failed for real on the previous settings (qpdf's C++ flag reaching the one C file); `16e8606` fixes that and builds here, but GitHub found no machine twice. It will be proven by the next push that gets a runner. On 20 November the full Mac suite passed here on `0184707` (which includes `16e8606`): 416 / 0, and that build is installed.
2. **`windows/port` lacks `16e8606`'s build-file fix** (`fomawrite.pro`, `tests/tests.pro`). It arrives with the cycle 166 merge; the two `.pro` files differ between branches, so that merge needs care (keep `QPDF_PREFIX` for Windows, pkg-config split for Mac).
3. **vcpkg cache hit not yet observed** on Windows CI; `bin/windows/build.ps1` now needs `QPDF_PREFIX` for any local Windows build.

## 6. What is required to complete the build cycle (Phase 3 = cycle 166)

### Save and Undo decision

After a successful PDF page save, page-edit Undo and Redo start fresh. Source and Live start fresh notes histories only when Save rewrites page references or coordinates. A failed PDF write preserves both histories.

### Cycle 166 scope (from the plan, §4.3–4.4 and the cycle 165 record)
- Thumbnails: multi-select, drag to reorder, context menu.
- Rotate left/right (⌘L / ⌘R), Delete Page, Insert Pages from File…, drop a PDF from the explorer onto the thumbnails, Export Selected Pages…; PDF menu entries on Mac and Windows (Ctrl on Windows after a collision check).
- Save in PDF mode calls `savePdf()` and shows its error or notice; "changed on disk" asks whether to overwrite; ask before editing a signed PDF; ask before closing with unsaved page edits; tell the writer where previous copies are kept.
- The ⌘Z decision above.
- Tests per operation and for each prompt; full Mac suite; merge to `windows/port`; Windows CI green on x64 and ARM64; GitHub macOS job green.
- Install build 166 on the Mac and in the Windows VM (with Andrew's yes, Fomawrite closed on the Mac) — this is the first build where page editing is usable.

### Also outstanding (not blocking 166)
- Andrew to try builds 163–166 on screen, in light and dark.
- Known gaps carried forward: marking a page inserted from another PDF is refused until saved; note sections are renumbered but not re-ordered after a page move; editing a password-protected PDF with its password is untested; PDFs over 512 MB refuse edits; nothing prunes previous copies of PDFs that no longer exist; marks re-anchor only within 3 pages; Live has no click-to-follow for links; no selection across pages; pinch zoom untested; text soft above about 300% zoom; no test of a chip click in a running Live page; intermittent Windows theme-file test.
- Follow-up: move `PublishingPreview.qml` off `PdfPageImage` (likely cause of the old 450 MB memory question).

### After Phase 3: Phase 4 (each item needs Andrew's go)
Shapes, text box and speech bubble stored in the notes; Export Annotated PDF (marks written as real PDF annotations into a copy); importing a PDF's existing annotations into the notes; and the other items in `pdf-plan.md` §4.5.

### Earlier questions Andrew deferred
Who 1.0 is for; Omawrite grey for quotes; narrow-width New Document/folder arrows; verdict on builds 147/148+; Harper documents; the 450 MB memory question; AI-tools spike; unmerged branches (`cursor/fomawrite-website-a380`, `cursor/manual-ci-trigger-8347`, `codex/cycle49-format`, `fix/issue-32`).

## 7. How the work was run, and cost

Planning on Fable; all development, merges, installs and records by sub-agents, one at a time, each ending in a commit. Approximate sub-agent usage:

| Stretch | Agents | Tokens (approx.) |
|---|---|---|
| Cycles 160–162 and the start of 163 (18–19 Nov) | five Opus agents | 185k–490k each |
| Finishing 163 | one Sonnet | — |
| Cycle 164 (two build parts, finish, a CI test fix) | four Sonnet | 705k |
| Cycle 165 (qpdf, pending edits, saving, Mac finish, Windows, close-out) | six Sonnet | 957k |
| Installs of builds 163 and 164 | two Sonnet | about 73k each |

Cycle 166 is likely to be similar in size to 164 (interface work plus a Windows merge): roughly 700k–900k Sonnet tokens.
