# Cycle 137 — Live theme fidelity (overnight, 8 October 2026)

**Build:** shipped together with Cycle 138 as `0.3.0-dev34` / macOS `0.3.0 (138)` (one install for both overnight cycles). **Plan:** [research/overnight-2026-10-08/PLAN.md](../overnight-2026-10-08/PLAN.md). Builder: page agents (Sonnet); reviewer: separate agent; planning and host work: Fable.

## What changed

| Area | Change |
| --- | --- |
| Review fixes for Cycle 136 (independent reviewer, see below) | **Host font no longer overrides a theme's body font in Live**: the host face and line height are zero-specificity fallbacks on `html.fw-live-root`, so a theme's `html`/`body`/`#write` face wins and the host value applies only when the theme sets none (Source unchanged). **Readability fill fixed**: the theme's effective background is measured on `#write`, then `body`, then `html`; text counts as set when it differs from the editor default; a one-sided theme gets a fill chosen by WCAG contrast (palette colour if it reaches 4.5:1, else `#111`/`#eee`). The bundled Claude Like theme now gets no fill and is readable in dark mode. **Exact mode keeps the mapped typography** (mapped copy scoped to `#write.fw-mode-live`; only the overlay is filtered-only). Tables full measure; more `#write` and `body` layout neutralised; revealed links out-specify mapped theme rules; the `livetheme.ts` header and README corrected. |
| Mapping fidelity | Live list lines carry `fw-list-ul` / `fw-list-ol` from the nearest Lezer list, so `ul li` and `ol li` theme rules map correctly. Absolute `px`/`pt`/`pc`/`in`/`cm`/`mm` sizes in mapped copies become `em` relative to the theme's own `#write` base (else 16px), including inside `font` shorthand, so headings follow Larger/Smaller. Quotes map `padding`/`padding-left`/`margin-left` (left value only, nested depth adds 1em per level), `border-left*`, colour, style and background colour; `::before`/`::after` glyphs deliberately not mapped. `mapThemeCss` is exercised on every bundled CSS file plus synthetic Typora-style idiom themes and never emits layout properties. |
| Host | View → Live Follows Theme Exactly is enabled only in Live and sits after a separator; the Code item never shows checked in Live; the capture helpers now send the host's real appearance JSON (font family, filter flag, palette). |
| Captures | New gallery test renders every bundled Output Style (presets 0–6 and Claude Like), light and dark, in filtered Live to `research/cycle-137/captures/` (16 frames; light≠dark and the styles are not all identical). |

## Verification

- Qt suite: **212 passed, 0 failed, 0 skipped**. New: `liveThemeGalleryCapturesEveryBundledStyle`.
- Editor page: **248 node tests** (up from 202 at the start of the night): mapping and overlay rules, the cascade engine tests for the font fallback (a small CSS cascade resolver over the real `editor.css`, which reproduced the Cycle 136 font bug on the old stylesheet), readability by contrast incl. a property test that every fill reads at 4.5:1, bundled-CSS robustness.
- Reviews: the Cycle 136 reviewer (verdict: ship with fixes) found the host-font override, the readability misfire, exact mode losing typography, tables not full measure and smaller items; all fixed here. The Cycle 137 reviewer (verdict: do not ship) then found the bundled presets unreadable in dark Live (a white `html` background read through an opaque dark `body`), the revealed-link rule not matching the real DOM, the dark table header under a light theme, missing `measureTheme` tests and a cascade engine that silently skipped child combinators, and the exact-theme command gated only in the native menu; all fixed before this build, with a WCAG contrast assertion (≥ 3:1 on the heading and body regions of every gallery frame) that was red on the unfixed page (preset 0 dark: 1.01) and is green now.
- Bundles: `dist/Fomawrite.app`, `dist/Fomawrite Dev.app` and `/Applications/Fomawrite.app` all at build 138 ([verified-builds.json](../cycle-138/verified-builds.json)), installed 8 October 22:40 with the app closed; `bin/check-document-views` passes in full against the release bundle.

## Limits

- Mapped rules tie the editor's own Live rules on specificity for `.fw-live-link` and `.fw-code` and win by source order (documented in the page README).
- Colours the readability helper cannot parse (`oklch`, `hsl`) skip the contrast check and use the palette colour as is.
- A theme that paints only `<html>` while the editor's body background remains is read as the html colour.
- Nested quote indent is the parent value plus 1em per level, not the theme's own arithmetic; `h1 code { font-size: 14px }` becomes `em` relative to the heading.
- In dark mode on a cream theme, the editor's dark code-block background makes tokens look washed out (not changed).
