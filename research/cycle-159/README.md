# Cycle 159 — PDF plan Phase 0: decisions and spikes

19 November 2026. No build and no user-facing change: this cycle records Andrew's decisions for [docs/pdf-plan.md](../../docs/pdf-plan.md), five technical spikes run on macOS (Apple Silicon, Qt 6.11.2) and Windows x64 and ARM64 (Qt 6.11.3, GitHub runners), and the plan review that followed.

## Decisions (Andrew, 19 November)
Mission amended as proposed (now in `MISSION.md`); notes file `Report.pdf.md`; notes open in the Markdown editor beside the PDF; the explorer shows one PDF row with a notes badge. Export of annotated PDFs stays undecided (Phase 4).

## Spikes
Code, fixtures, the Windows workflow and raw results are on branch `spike/pdf-phase0` under `spikes/pdf/` (results in `spikes/pdf/results/`); nothing from it is merged. Fixtures are hand-built PDFs with words at known PDF user-space positions (`make_fixtures.py`), plus one annotated with PDFKit, the framework Preview uses.

| # | Question | Result |
|---|---|---|
| 1 | Does Qt PDF draw annotations already in a PDF? | `QPdfDocument::render` draws highlights, underline, strikethrough and notes **only with `RenderFlag::Annotations`**, for annotations with and without appearance streams and for PDFKit's. QML `PdfPageImage` has no such option and never draws them. Same on Mac, Windows x64 and ARM64. |
| 2 | How do Qt's selection rectangles relate to PDF user space? | They are in page-view points: top-left origin, after `/Rotate`, relative to the CropBox; 100% of rendered text ink lies inside them on all five test pages. The mapping (user `(x, y)`, CropBox `[x0 y0 x1 y1]`): rotate 0 `(x−x0, y1−y)`; 90 `(y−y0, x−x0)`; 180 `(x1−x, y−y0)`; 270 `(y1−y, x1−x)`. On rotated pages Qt returns rectangles with negative width or height (normalise them). Qt's boxes are glyph-tight (baseline to cap height), so highlights need vertical padding to look like Preview's. Identical numbers on all three platforms. |
| 3 | Can a PDF menu appear only in PDF mode? | Quick Controls `MenuBar` (the Windows path): `insertMenu`/`removeMenu` at runtime work. Native macOS (`Qt.labs.platform`): a menu inserted at runtime appears with **no title** in the real menu bar (`NSMenuItem`), but a statically declared menu's `visible` property hides and shows it correctly. Declare the PDF menu statically and bind `visible`. |
| 4 | Is qpdf suitable for page operations, on both platforms? | qpdf 12.4.2 (Apache-2.0): Homebrew on Mac; vcpkg on Windows builds in about 4 minutes for x64 and ARM64. Rotate, delete, move, append 300 pages and reverse 300 pages all verified through Qt PDF (page count, order by page text, orientation), each in 0–33 ms. A `/Highlight` annotation written by qpdf as a bare dictionary is drawn by Qt PDF, so qpdf can also write the Phase 4 annotated export; no second PDFium is needed. |
| 5 | Memory and speed on large PDFs? | Opening is instant (300 pages; 48 MB of images). Rendering at 2× takes 1–10 ms a page on Mac, 5–30 ms on Windows. **A `ListView` of QML image items retains every page it has shown**: Qt's `PdfPageImage` held 1.7 GB on Mac and 1.6–2.2 GB on Windows after scrolling 300 pages (cache on or off, synchronous or not, after garbage collection, with 3 delegates alive); an `Image` on a custom provider held 6.7 GB at Retina scale. A `QQuickPaintedItem` that renders its own page peaked at 177 MB on Mac (cocoa, Retina) and 49–97 MB on Windows; reusing delegates (`reuseItems`) peaked at 543 MB. |

Limits: the Windows "provider" variants are invalid (the spike passed a Windows path through an `image://` id and nothing rendered); the decision does not depend on them. Spike 5 ran against generated PDFs, not real scanned documents. The Mac real-renderer runs showed a test window for a few seconds each.

## Consequences for the plan
1. Fomawrite renders pages in its own item (`QPdfDocument::render` with annotations, one image per live delegate); it does not use `PdfPageImage` or `Image` for pages. `PublishingPreview.qml` uses `PdfPageImage` today and probably has the same retention (the earlier "450 MB" question); fixing that is listed in the plan as follow-up, not part of the PDF phases.
2. The PDF menu is declared statically and shown only in PDF mode.
3. qpdf is the one writing library, for page operations and later for the annotated export.
4. Text markup is anchored by page text index plus the quoted text (geometry is derived at draw time), which the geometry result makes safe; note pins are stored in PDF user space using the mapping above.

## Review
The plan was reviewed against the code and these results and rewritten as a cycle-by-cycle development programme ([docs/pdf-plan.md](../../docs/pdf-plan.md), "Review" section lists what changed and why).
