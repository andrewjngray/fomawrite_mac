# Cycle 136 — Live follows the publishing theme, lighter

**Build:** `0.3.0-dev32` / macOS `0.3.0 (136)`. **Date:** 8 October 2026. **Decisions:** [docs/live-appearance-design.md](../../docs/live-appearance-design.md) (Option 3, built as a CSS filter; text size shared; Split kept; exact-theme switch as a setting; Code drops back to Source).

## What changed

| Area | Change |
| --- | --- |
| Editor page (`src/editor/src/livetheme.ts`) | In Live, the theme CSS applies and an editing overlay (`#fomawrite-live-overlay`) neutralises its layout on `#write` and block containers (position, float, columns, transform, box-shadow, background-image, `::before`/`::after` furniture; width, max-width, margin and padding reset to the editor's own measure) while typography survives. The overlay is scoped to `#write.fw-mode-live:not(.fw-live-exact)`, so Source is untouched. |
| Editor page (`src/editor/src/thememap.ts`) | MAPPING_STATUS |
| Editor page | Source appearances (Manuscript/Editorial/Book/Code classes) are not applied in Live; `fontSize`, `fontFamily`, focus, typewriter and dark still are. Readability fill: when the theme leaves `#write`'s background or text colour unset, the host palette fills it (`--fw-live-bg` / `--fw-live-fg`). |
| Appearance JSON | New optional keys `liveThemeFilter: boolean` (default true; false = exact theme) and `palette: {background, text}`. |
| Host (`LiveEditorPane.qml`, `Main.qml`) | The pane sends the two keys; a new workspace setting `liveThemeExact` backs **View → Live Follows Theme Exactly** (command `liveThemeExact`). |
| Left control | In Live the footer control names the Output Style; its menu offers "Output style: <name>…" (opens the same theme menu as the publishing pane), Code, and the text-size items; Manuscript/Editorial/Book are hidden until Source returns. |

## Verification

- Qt suite: **QT_COUNT passed, 0 failed, 0 skipped**. New: `liveLeftControlShowsThemeAndExactSwitchReachesPage` (menu contents in both modes; the switch reaches the page's appearance JSON with `liveThemeFilter:false` and the palette); `liveEditorCapturesEachPresentation` now also renders a deliberately layout-heavy theme filtered and exact and checks the frames differ.
- Editor page: **JS_COUNT node tests** (overlay rules, scoping, exact switch, readability fill, selector mapping).
- Captures ([research/cycle-134/captures](../cycle-134/captures/) `live-heavy-theme-filtered.png`, `live-heavy-theme-exact.png`): filtered keeps the theme's Georgia face, brown text and cream background and drops the two-column layout, rotation, shadow, 420 px measure and the "DRAFT" furniture; exact shows all of them.
- Bundles: BUNDLE_STATUS

## Limits

- LIMITS_STATUS
- Rules a theme puts on `html`/`body` are reset only for padding, margin, background-image and columns.
- The readability check compares `#write`'s colour with `<html>`'s, so a theme that sets only `html { color }` is treated as unset and filled from the palette.
