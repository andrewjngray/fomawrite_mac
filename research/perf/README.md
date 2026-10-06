# R2 — Performance profile of A2, A3 and P1 after Cycle 132

Review cycle R2 from [INDEPENDENT_REVIEW.md](../../INDEPENDENT_REVIEW.md) §8: the three performance findings
(A2 theme-heavy preview cost, A3 PDF profile/page per render, P1 mapper scaling) were *traced, not profiled*
by the review and partly addressed by [Cycle 132](../cycle-132/README.md). This cycle measures them.

**Method.** Four QTest slots in [`tests/perf-profile.inc`](../../tests/perf-profile.inc)
(`perfThemeHeavyPreviewCost`, `perfPdfRenderProfileReuse`, `perfMapperScaling`, `perfImageDecodeCaching`)
time the production code paths with `QElapsedTimer`, print every figure with `qInfo`, and assert only generous
sanity bounds so they stay green in CI. Nothing under `src/` was changed. Run one with
`./bin/test perfMapperScaling`; all four run in ~12 s. Instruments was not used: these are wall-clock figures
on the GUI thread (or, for Chromium phases, signal-to-signal latency), which is what the review's findings are
about.

**Machine.** Apple M5 Max, macOS 27.0.1, Qt 6.11.2 (Homebrew), release build, `QT_QPA_PLATFORM=offscreen`.
Figures are from the final run on 6 October 2026; run-to-run variance is noted where it exceeded ~30 %.
Expect roughly 2–3× these numbers on an Intel Mac.

## Numbers

### A2 — theme-heavy preview (folder theme embedding a synthetic 16 MiB `.woff`, 6,649-char document)

| Measurement | ms | Notes |
| --- | ---: | --- |
| `requestPublishingPreview("html")`, basic style (no theme) | 0.78 | 9,208-byte output |
| `selectPublishingTheme(id)` | 180 | runs `PublishingThemes::css` twice (validate, then memoize) + SHA-256 of the 22 MB CSS |
| (a) first request with the theme selected | 14.7 | 22,377,846-byte output written to the preview directory |
| (b) request after a one-character source edit (memoized CSS) | 20.1 | new body, new output file |
| (c) request with no change (cache hit) | 12.5 | full HTML is still assembled before the key is compared |
| (c′) second cache hit | 12.5 | 7.4–12.6 across runs |
| `PublishingThemes::css(id)` once — what every request paid before Cycle 132 | 33.8 | 22,369,713 chars; 29–56 across runs |
| `css.toUtf8()` | 1.0 | |
| SHA-256 of the CSS (once per memo, formerly over the whole HTML per request) | 89.7 | |
| `embedImages` regex scan of the assembled 22 MB HTML (per request) | 6.3 | |
| Chromium `loadFinished` for the basic 9 KB page | 65 | raw `QWebEnginePage`, same as the preview pane / PDF renderer pay |
| Chromium `loadFinished` for the themed 22 MB page | 298 | |
| Chromium `loadFinished` for the themed page again (after an edit) | 239 | |

### A3 — PDF render (11 pages, 26,321-char document, basic style)

| Measurement | ms | Notes |
| --- | ---: | --- |
| Warm-up: one-paragraph PDF (first render in the process) | 149 | Chromium start is not visible as a separate cost |
| Render 1 (same consumer: shared profile, new page) | 178 | 11 pages |
| Render 2 (after a source edit) | 176 | |
| Render 3 (after a source edit) | 178 | no speed-up from 1→3: there is nothing to warm |
| Render on a fresh consumer (new `PublishingPdf`, new profile) | 176 | **profile reuse saves nothing measurable** |
| `publishingPreview("pdf")` blocking export path (renderer + profile + page per call) | 178 | same as the async path |
| `QPdfDocument` load + `getAllText` of the 11-page result (anchor-extraction proxy) | 17 | `storePublishingOutput` does this per render |
| raw: new `QWebEngineProfile` / new `QWebEnginePage` / second page on same profile | 0.35 / 0.00 / 0.01 | |
| raw: first page `load` + `printToPdf` | 69 + 41 | |
| raw: reload new content in the same page + print | 20 + 41 | page reuse saves ~48 ms of load |
| raw: print again without reloading | 39 | |
| raw: second page on the same profile, load + print | 68 + 41 | no renderer-process spawn penalty visible |

A first run of this slot polled the signal with `QTest::qWait(5)` and reported ~275 ms per render; waiting on
the signal with an event loop gives the 176–178 ms above. The ~100 ms difference was the harness, not the code.

### P1 — mapper scaling

| Measurement | ms | Notes |
| --- | ---: | --- |
| `SourceVisualMapping::create`, one 1 KB line of prose | 0.99 | |
| … one 10 KB line | 42 | |
| … one 100 KB line | 4,127 | |
| … one 1 MiB line | ≈ 402,000 (extrapolated) | not run: ~6.7 minutes predicted; skipped when the prediction exceeds 12 s |
| Growth exponent 1 KB → 10 KB / 10 KB → 100 KB | 1.63 / **1.99** | O(n) = 1, O(n²) = 2 |
| `sourceEditForVisualReplacement` on the 10 KB line | 42 | re-projects the candidate: one more `create` |
| `create`, 100 KB multi-line document | 46 | |
| `create`, 1 MiB multi-line document (6,535 lines) | 472 | linear, but ~0.45 µs per character |
| `TextEdit.text = 1 MiB` (Source pane, QML `TextEdit`) | 1,177 | outside the review's scope; recorded because it is the largest number here |
| `Backend::visualProjection()` on the 1 MiB document | 473 | create + `QVariantMap` of 6.5k blocks |
| `Backend::applyVisualEdit` of one keystroke on the 1 MiB document | 969 | create + candidate create + `QTextCursor` insert |
| `visualProjection()` after the keystroke (what the QML pane then does) | 562 | |
| **One Visual keystroke on a 1 MiB document** | **≈ 1,530** | three full projections |

### Image decode caching (20 PNGs of 64×64, 3,546 bytes on disk; one 3000×2000 PNG, 60,776 bytes)

| Measurement | ms | Notes |
| --- | ---: | --- |
| First `requestPublishingPreview("html")` with 20 images | 17.0 | first-use overhead (image plugins, preview directory), not decode |
| Request after a one-character edit (cached images) | 0.74 | |
| `embedImages`, cold cache / warm cache / no cache (pre-132) | 0.84 / 0.16 / 0.76 | tiny PNGs: the cache is a wash |
| `embedImages`, one 3000×2000 PNG, cold (read + decode + base64) / warm | 21.4 / 0.07 | the decode the cache was built for |

## What this shows against the review

**A2 (HIGH in the review) — closed on the backend; the residual cost is Chromium's, and it is not a hang.**
Per-request work with a 16 MiB font is 12–20 ms on the GUI thread, dominated by concatenating and writing a
22 MB HTML string. The cache hit (c) still assembles the whole document (the limit recorded in the Cycle 132
README) but that costs 12 ms, so it does not matter. The review assumed the pre-132 path ("base64 of up to
64 MiB + full image decode + SHA-256 over the whole HTML, before the cache is consulted") made the pane hang.
Measured, that path cost about 34–56 ms (`css()`) + 90 ms (SHA-256 of 22 MB) ≈ 150 ms per debounce tick at
16 MiB, so roughly 0.6 s per tick at the 64 MiB limit on this machine, or ~1.5 s on an Intel Mac: a visible
stutter, not a freeze. Cycle 132 removed it either way. What remains is that Chromium parses a 22 MB page with
an inline `data:` font on every edit (240–300 ms per load, in the renderer process, not on the GUI thread) and
Fomawrite writes 22 MB to disk per edit (~5 ms here). Moving generation off-thread, as the review suggested, is
not justified by these numbers.

**A3 (MED in the review) — the profile sharing done in Cycle 132 changed nothing measurable; a page is not
the cost either.** A new off-the-record profile costs 0.35 ms, a new page 0.01 ms, and a second page on the
same profile loads in the same 68 ms as the first, so no renderer-process spawn shows up in latency. A PDF job
costs ~177 ms regardless of page count (149 ms for one paragraph, 178 ms for 11 pages): ~110 ms Chromium
(load 68 + `printToPdf` 41), ~17 ms re-opening the PDF for heading anchors, and ~50 ms of Fomawrite plumbing
(temporary directory, HTML write, the isolated-world readiness poll with its 40 ms retry, zero-timer hops).
Reusing one page per renderer would save ~48 ms of load (68 → 20) — about a quarter of a job that is already
behind a 200 ms debounce. The review's observation of eight `QtWebEngineProcess` helpers is a memory-footprint
matter, not a latency one; this profile did not measure memory.

**P1 ("plausible, lower" in the review) — confirmed quadratic, and the largest real cost measured.**
`appendInline` copies the remainder of the line (`text.mid(cursor)`) and runs three anchored regexes for every
plain character, so a single line costs O(n²): exponent 1.99 between 10 KB and 100 KB. A 10 KB paragraph (a
long pasted paragraph, ~1,500 words) costs 42 ms per projection; a 100 KB line costs 4.1 s; a 1 MiB line would
take minutes. Each Visual keystroke runs three projections (`applyVisualEdit` creates the mapping, the
candidate check creates it again, then the pane re-projects), so a 10 KB paragraph costs ~130 ms per keystroke,
a 100 KB line ~12 s, and even an ordinary 1 MiB document of short lines — linear but at ~0.45 µs per character —
costs ~1.5 s per keystroke. The Source pane's own `TextEdit` cost for a 1 MiB document (1.2 s to set the text)
is a separate, QML-side problem.

**Image cache — works, and only matters for large images.** 21 ms → 0.07 ms per 6-megapixel PNG; for small
images it is neutral. The 17 ms first request is first-use overhead, not decode.

## Recommendations, ranked by measured impact

1. **Fix P1 now (Cycle 133 or earlier; ~50 lines in `src/sourcevisualmapping.cpp`).** In `appendInline`,
   stop taking `text.mid(cursor)` per character: match with `QRegularExpression::match(text, cursor,
   NormalMatch, AnchorAtOffsetMatchOption)` and add a fast path that appends a whole run of characters that
   cannot start a construct (anything but `` ` ``, `[`, `*`, `_`) with one `appendMapped` call. That makes a
   line O(n) with a far smaller constant; the 1 MiB document should drop from ~470 ms to tens of ms per
   projection. Then cut the three projections per keystroke to one or two: `applyVisualEdit` already tracks
   `m_lastVisualResultSource`, so the mapping for the current source can be cached by source hash and reused by
   `visualProjection()`. Gate: existing mapper tests, the R3 fuzz, and `perfMapperScaling` reporting an
   exponent near 1 with the 1 MiB single line no longer skipped.
2. **A3: do not build page reuse.** Measured saving is ~48 ms of a ~177 ms job behind a 200 ms debounce,
   against the cancellation complexity the Cycle 132 README chose to avoid. If any PDF latency work is wanted,
   the cheaper targets are in Fomawrite's plumbing: shorten the readiness poll's 40 ms retry (or resolve
   `document.fonts.ready` once instead of polling) and skip the second `QPdfDocument` pass for anchors when the
   document has no headings. Together these are worth up to ~50 ms; neither is urgent.
3. **A2: nothing further on the backend.** Per-request assembly before the cache lookup is 12 ms at 16 MiB
   and not worth restructuring; off-thread generation is not justified. The one remaining cost is structural:
   the inline `data:` font makes every preview load re-parse 22 MB (240–300 ms in Chromium). If users with
   multi-megabyte font themes matter, serve theme fonts out of line (a custom scheme handler or an interceptor
   allow-list under the preview directory) so Chromium can cache them; that is a medium-sized change and the
   only way below ~250 ms per edit for such themes. Trivial tidy-up: `selectPublishingTheme` sanitizes the theme
   twice (validate, then memoize); reusing the validated result halves a one-off 180 ms.
4. **Images: leave as is.**
5. **Harness note for future profiles:** wait on signals with an event loop, not `QTest::qWait` slices —
   polling inflated the PDF figures by ~100 ms in the first run of this cycle.

## Limits

- Wall-clock on one fast machine, offscreen, no Instruments; no memory or process-count measurement (the
  review's eight helper processes remain unmeasured).
- The 16 MiB `.woff` is synthetic (`'A'` bytes): the sanitizer and Chromium's HTML parser do the same work,
  but Chromium's font decode fails immediately rather than decoding a real font.
- The 1 MiB single-line figure is extrapolated from the measured exponent; the slot skips the run when the
  prediction exceeds 12 s so CI stays bounded.
- Variance: `PublishingThemes::css` ranged 29–56 ms and the cache hit 7–12 ms across five runs; PDF jobs were
  within ±3 ms.
