# Live appearance — design (decided 7 October 2026)

Decisions from the Source / Live debate, with the option numbers used in the discussion.

| Question | Decision |
| --- | --- |
| 1. Does Live follow the publishing theme? | **Option 3, built as (a): the theme is applied and then filtered.** Live keeps the theme's typography (faces, colours, sizes, heading scale, code, quote, table and link styling) and neutralises its layout (page widths and margins, columns, floats, positioned elements, decorative backgrounds and shadows, page furniture). The right-hand publishing pane remains the exact rendering. |
| 2. Text size in Live | Shared with Source: Larger / Smaller / Reset act on both surfaces. (Assumed from "the rest are yeses"; confirm.) |
| 3. Split while in Live | Kept: the published pane stays beside Live because it shows what Live cannot (exact layout, PDF pagination). No automatic switch to Single. (Assumed; confirm.) |
| 4. Code in the Live dropdown | Stays; choosing it drops back to Source silently. |
| Escape hatch | A setting, not a dropdown entry: **View → Live Follows Theme Exactly** turns the filter off for themes that paint their look through layout. Off by default. |

## The filter (how "lighter" is defined)

The page applies the theme CSS to `#write` as today, then an editing overlay stylesheet with higher precedence that, for `#write` and everything inside it:

- keeps: `font-family`, `font-size`, `font-weight`, `font-style`, `color`, `line-height`, `letter-spacing`, `text-decoration`, list markers, heading styles, code and quote colouring, table cell borders and alignment, link colour;
- neutralises: `position` (other than static), `float`, `columns`, `max-width` / `width` on block containers (the editor sets its own measure), `margin` and `padding` that create page geometry, `background-image`, `box-shadow`, `transform`, `::before` / `::after` furniture that is not a list marker;
- guarantees: the page background and text colour are a readable pair in light and dark (the host's palette fills whichever the theme omits), the caret is visible, code blocks and tables are full measure.

Drift between Live and the published pane is by design and layout-only. Rule of thumb for tests: anything that is bold, coloured, a heading, code or a quote in one is the same in the other.

## The left control in Live

Manuscript, Editorial and Book have no meaning in Live and are hidden there. The control shows: the current theme's name (opens the same Output Style menu as the right pane), text size (Larger / Smaller / Reset, shared with Source), and Code. Returning to Source restores the Source items.

## Out of scope

Reading theme files to extract values (approach (b)); per-document overrides; Mermaid.
