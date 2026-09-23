# Preview and output templates — Cycle 47

## Decision

View → Template is the source of truth for live preview, HTML export, PDF export, paginated preview and rendered printing. Templates format rendered content without changing Markdown or the editor font. The selected preset is persisted as the default for subsequently opened windows; existing windows retain their current selection. Old output style IDs 0–3 are retained. The former separate preview font preference is superseded by the saved output selection.

## Presets

| Template | Typeface | Layout |
| --- | --- | --- |
| Modern (Sans) | Helvetica Neue | Proportional headings, 135% line height, clean output without running decorations |
| Classic (Serif) | Georgia | Reading typography with running title/page counter |
| Manuscript (Mono) | Bundled iA Writer Mono S | Double-spaced text, compact headings |
| GitHub | Helvetica Neue | 150% line height, padded bordered tables and shaded code |
| Helvetica | Helvetica | Sans document with running title/page counter |
| Palatino | Palatino | Serif document with running title/page counter |
| MLA Draft | Times New Roman | Double-spaced text, half-inch first-line paragraph indents, centered first heading |
| Custom | JSON fontFamily / pointSize | Existing bounded JSON style loader with header, footer and titlePage fields |

These are Omawrite presets, not copies of iA template assets. Duo and Quattro are not bundled yet. MLA Draft is not full MLA compliance: it does not supply author/course/date metadata, citation formatting, Works Cited hanging indents, surname running heads, or automatic required paper/margin setup. Page Setup remains the source of paper and margins. Use institutional requirements before submitting work.

## Rendering choices

One C++ formatter handles headings, paragraph rhythm, quotations, lists, links, code and tables for preview and output. Preview uses theme colors; export uses readable light-paper colors. Preview text-size controls act as a screen reading adjustment; exports use the preset point size. Continuous preview does not display physical page decorations/title pages; paginated preview shows those.

Qt's Markdown import retained stale first-table-cell layout state. Normalizing the already-formatted document through HTML before preview/pagination fixed the header alignment. Output now lays out in 72-point units and scales once for device DPI, avoiding tiny table dimensions alongside large print fonts. Header/footer fonts use the same logical units. Source Markdown, including explicit page-break comments, is not modified.

## Remaining work

- License-checked Duo/Quattro font packaging and matching presets.
- Full MLA metadata, paper geometry, citation and bibliography rules if wanted.
- iA template-package/HTML-CSS import; JSON custom styles remain the supported format.
- Browser HTML rendering matrix, dark/narrow native template checks, physical printer checks, and long multi-page table/image stress tests.
- Cross-window live propagation and per-document template persistence, if desired.
