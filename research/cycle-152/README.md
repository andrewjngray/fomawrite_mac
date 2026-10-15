# Cycle 152 — The Omawrite-like look

Build 152 (`0.3.0-dev48`), 15 October 2026. Andrew asked for the look of Omawrite (github.com/omacom/omawrite), "exactly what fonts and colours and spacing", and where it belongs in Fomawrite.

**Where it lives: both places.** Source gets a fifth Writing appearance, **Omawrite**, after Manuscript, Editorial, Book and Code (Aa in Source; View → Writing Appearance). Output Style gets a second built-in style, **Omawrite**, after Claude Like; Live and Preview follow the Output Style (Aa in Live opens that menu), and so do HTML, PDF and print exports.

**The values, read from Omawrite's source (`src/Main.qml`, `markdownhighlighter.cpp`, `backend.cpp`):**

| | Omawrite | In Fomawrite |
| --- | --- | --- |
| Font | IBM Plex Mono (bundled, SIL OFL) | IBM Plex Mono from IBM's own repository (`fonts/`, licence `fonts/IBMPlexMono-OFL.txt`): TTF for Source, WOFF2 embedded as data in the style for Live and exports |
| Size | 20 px, headings bold at body size | Source preset 20 px; Live 20 px when the Omawrite style is chosen; both follow Larger/Smaller text |
| Line height | 140% of the font's line spacing (Typora's value) | Source 140%; CSS 1.82 (the same, as Plex Mono's spacing is 1.3 em) |
| Measure | 65 characters, or window less 20 characters, centred | Source: the same rule. HTML: the same. Live keeps its own column (about 57 characters at 20 px) |
| Paragraphs | the Markdown blank line, no extra spacing | the same |
| Dark / light | #101010 / #ffffff page, #eeeeee / #222324 text, #5584aa / #2077b2 links, #4f525a / #aeb1b5 marks | the same, following the app's light/dark mode |

Measured on the README screenshot, the colours are a little warmer (#181215 page, #c5bfc2 body text): that window is about 90% opaque over a reddish wallpaper, and the theme used there sets its own text colour.

Built by two parallel builders (Source preset; Output Style) and checked by a reviewer. Builder B found that with "Appearance follows Output Style" on (the default) a style pins the whole app to its own background; a style that declares a dark variant (`--dark-bg-color`) now follows the app's light/dark mode instead. Exports and print always use the light variant, and Preview shows the export, so it is light too.

Reviewer findings fixed before install:
1. Fenced code blocks in Source were drawn on the old panel colour (#252a32 / #eaecf0) instead of #1d1d1d / #f2f2f2; the test had checked a block format that nothing paints. Fixed, and the test now reads the drawn panel.
2. Live showed Plex Mono at the Source preset's size (18 px with Manuscript), not 20 px. The Omawrite style now sets Live's base size to 20 px; other styles are unchanged.
3. The header band and footer strip around the Source page kept the app colour (a bluish band in dark with following off, a cream band with Warm Paper). They now match the page.
4. Markdown marks shown on the caret line in Live used the editor's grey; a style can now colour them (Typora's `--md-char-color`), and Omawrite uses its marker colours.
5. The command palette listed Omawrite before Code; it now matches the menus.

Left as is, for Andrew: Live and exported quotes keep the text colour (Omawrite's marker grey is about 2:1 on white, hard to read), while Source quotes use the marker colour as Omawrite does. Preview is always light. The fonts are decoded twice in Live (a small one-off cost).

Tests: `tests/cycle152-omawrite-source.inc` (3) and `tests/cycle152-omawrite-style.inc` (7).
