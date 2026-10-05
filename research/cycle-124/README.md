# Cycle 124 — CSS publishing themes and browser output

Verified review build **0.3.0-dev20 / macOS 0.3.0 (124)**. Dev, ordinary/demo and Applications are refreshed. [Verified identities](verified-builds.json).

## Scope and implementation

Publishing now selects CSS document themes through its own footer and **View → Publishing Theme**, including **Claude Like**, imported themes, Import Theme, Open Themes Folder and Reload Themes. The built-in adapts Andrew's actual local Claude-like Typora CSS into document-only rules for semantic HTML, preserving its warm colours, serif typography, spacing, quotes, tables and code appearance. Georgia supplies the fallback; proprietary fonts are not copied. Source and Visual Edit retain independent writing appearance and canonical Markdown/Undo.

Managed imports live in the application's `publishing-themes` data folder. They flatten local CSS imports and embed PNG/JPEG/GIF/WebP images and WOFF/WOFF2/TTF/OTF fonts into standalone CSS. Bounds are 64 files, eight import levels, 1 MiB per source CSS, 4 MiB per asset, 16 MiB combined source resources and 32 MiB expanded CSS. Traversal outside the selected folder and symlink resources are refused. No remote resource is fetched. Typora's `@include-when-export` directive is omitted with an advisory, including URLs whose font-weight lists contain semicolons.

A semantic MD4C HTML renderer preserves supported headings, nested lists/quotes, tables, code, links, footnotes/highlights and explicit page breaks. Arbitrary raw HTML is escaped. Chromium renders the shared document/CSS for Web and vector PDF export. Export and share displays that same Web/PDF result, including real PDF pages, paper settings and page furniture. Its basic theme gallery retains legacy font/page controls and saved basic styles. Each preview owns an asynchronous PDF job. Native rendered Print/Print Preview uses the browser PDF rasterized at 300 dpi; physical printing has not been verified.

## Verification

- **193 regression cases pass, 0 failed, 0 skipped**, including four new publishing slots and setup/cleanup.
- Dev, ordinary/demo and Applications each pass **154 native checks**: 124 footer states, seven daily-writing checks, five pane-zoom checks, six editor acceptance checks, three pane/footer/library checks, six independent Editing/Layout checks and three publishing-theme checks. These use synthetic Qt pointer/key events on Cocoa. All report zero QML warnings, matching embedded UI/version and valid strict signatures.
- Publishing checks cover actual painted cream Web output, semantic quotes/tables, real Chromium PDF pages, A4 and Letter landscape, and repeated Web/PDF, Source/Visual Edit and Single/Split changes. Exact source text, Undo/Redo and saved-file hashes are preserved. Web/PDF screenshots were visually inspected.
- CSS imports cover local stylesheet flattening, embedded images/fonts, warning handling, stored reload and unsafe-resource rejection. Importing the actual Claude CSS preserves document rules and omits the complete export-only Google Fonts directive with an advisory.
- A read-only private writing sample produced **two A4 pages**. Bundled and imported Claude themes have pixel-identical first pages with Georgia fallback, and the source stayed unchanged. This compares the two Fomawrite theme paths; it does not establish pixel parity with Typora's renderer.
- Review locations: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app` and `/Applications/Fomawrite.app`. The previous installed app was retained in a temporary backup. Source commit and final executable hashes are recorded in [verified-builds.json](verified-builds.json).

The pre-existing conservative Edited indicator can remain after Undo restores saved text; exact text/history checks pass. Native launches also emit two nonfatal Qt timer/thread diagnostics, separate from the zero QML-warning count.

The private comparison is retained only under `/tmp/fomawrite-cycle124-nps-comparison`. Original writing, private exports and raw screenshots are excluded from public records. Native captures/logs use disposable fixtures and remain local and ignored. No public release or notarization is included.

## Compatibility and acceptance limits

Common document selectors such as `#write`, headings, paragraphs, tables, quotes and code apply to semantic output. Typora UI/editor selectors have no corresponding elements. CSS escapes, SVG theme assets, parent-relative theme resources, advanced import layer/supports modifiers and image/image-set syntax are unsupported. Remote fonts and assets are blocked; missing proprietary fonts use available fallbacks. Local font embedding supports portable output without downloading fonts.

Math remains literal and is not typeset. Raw HTML is escaped rather than executed. PDF heading fallback refuses ambiguous text matches, so some heading jumps can report an unavailable destination. General Markdown/CSS compatibility, rich editor parity, large-document performance and cross-application output remain bounded. Physical printer, VoiceOver, physical input/IME, display scaling and the historical native activation gaps remain open.

Primary references: Typora documents CSS-file themes and related local resources in [About Themes](https://support.typora.io/About-Themes/), and theme selection for HTML/PDF in [Export](https://support.typora.io/Export/). These informed the bounded document-theme workflow rather than a claim of complete Typora behaviour.

[Optional user review](../usability/cycle-124.md). Source/install identities are recorded in [verified-builds.json](verified-builds.json).
