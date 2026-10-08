# Cycle 140 — Typora base shim (9 October 2026)

**Build:** none yet (source change on a worktree branch; bundles stay at 139).

## What changed

| Finding | Root cause | Change |
| --- | --- | --- |
| A dark Typora theme (Lapis Dark) showed pale text on a white page in Web/PDF output and in Live | Such themes define `--bg-color`, `--text-color`, `--select-text-bg-color`, `--code-color`, `--block-bg-color`, `--primary-color`, `--marker-color`, `--md-char-color` on `:root` and rely on Typora's own base stylesheet to paint `body`, the selection, code and markers from them. Fomawrite applied the theme CSS but nothing read the variables. | `src/typorabase.{h,cpp}`: `TyporaBase::css()` is a small base shim, one text, every rule commented with the Typora behaviour it mirrors and every fallback equal to the property's value without the rule (so a theme without the variables is unchanged). `Publisher::html` emits it after the foundations and before the theme CSS; `Backend::liveThemeCss` / `pushLiveTheme` / `syncLiveEditor` send `TyporaBase::liveCss()` (the shim plus CodeMirror selection, caret and Live code-line hooks) ahead of the theme CSS to the page. The page is unchanged. |
| The host could not read a theme's colours (follow-up: app chrome following a dark theme) | No parser | `TyporaBase::themeVariables(css)` (top-level `:root` / `html` rules only, `@media` ignored, last declaration wins, `var()` resolved with fallbacks and chains, cycles dropped), `Publisher::themeVariable(name)` / `themeVariables()` (memoized per CSS text), `Backend::themeBackgroundColor` (`NOTIFY publishingCssChanged`, also emitted when a basic preset replaces a theme that defined `--bg-color`). The follow-up feature is not implemented. |

## Verification

- Qt suite: **216 passed, 0 failed, 0 skipped** (`./bin/test`, offscreen, 237 s); editor page **249 passed** (one test added; the page itself is unchanged, `dist/editor.js` is not rebuilt).
- New tests (`tests/cycle140-typora-base.inc`): `typoraBaseReadsThemeVariables` (pure parser cases plus the Lapis Dark fixture: `--bg-color` `#1e222a`, `@media print` ignored, `var()` chains, `Publisher` / `Backend` accessors and notifications), `publishedOutputPaintsThePageFromThemeVariables` (shim in the HTML once, before the theme; a real PDF has a dark ground), `liveEditorPaintsThePageFromThemeVariables` (computed `html` / `body` / `#write` colours, no readability fill, selection layer and caret follow the theme, exact mode, dark editor, and a basic preset keeps the editor's own colours); `liveEditorCapturesEachPresentation` gained `live-dark-variables.png` (filtered) and `live-dark-variables-exact.png` with corner and margin pixels asserted dark. Fixtures: `tests/fixtures/typora/lapis.css`, `lapis-dark.css` (copies of the Lapis theme by YiNN, MIT).
- Red/green: with the shim wiring removed from `Publisher::html` and `Backend::liveThemeCss` (parser and accessors kept) and the new tests kept, `publishedOutputPaintsThePageFromThemeVariables` fails at `shim >= 0` ("the Typora base shim is missing from the published HTML"), `liveEditorPaintsThePageFromThemeVariables` fails at the body background (`rgb(255, 255, 255)` instead of `rgb(30, 34, 42)`) and `liveEditorCapturesEachPresentation` fails at the frame corner (`#ffffff`); all pass with the wiring. The parser test has no earlier implementation to fail against.
- Native: not run. The captures were reviewed by eye (`research/cycle-134/captures/live-dark-variables*.png`, local only).

## Limits

- `.highlight` (`==text==`) keeps its fixed pale-yellow background and the theme's text colour: pale text on pale yellow in a dark theme. Typora's `mark` rule is theme-specific and Fomawrite emits a span.
- Live: `--md-char-color` is not applied to revealed Markdown marks; theme colours do not reach the real table widget or Live code-token colours.
- `::selection` falls back to the system `Highlight` colour (close to, not bit-identical with, the browser default) in published HTML for themes without `--select-text-bg-color`.
- Print: Lapis's `@media print` text override loses to Lapis Dark's later `:root`, so a PDF of Lapis Dark should be light text on a dark ground (not checked with the real Lapis Dark; the PDF test uses a variables-only theme).
- The variable parser does not model `!important` or selector specificity between `:root` rules.
