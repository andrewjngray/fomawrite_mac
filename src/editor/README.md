# Fomawrite editor (web side)

CodeMirror 6 Markdown editor bundle hosted by the Qt app in a `WebEngineView` (`qrc:/editor/index.html`)
and driven over `QWebChannel`. The Qt build has no node step: `dist/editor.js` is **committed**.

## Commands (run in `src/editor/`)

| Command | What |
| --- | --- |
| `npm ci` | install pinned deps |
| `npm run build` | bundle `src/main.ts` -> `dist/editor.js` (IIFE, minified, chrome120, no runtime deps). **Re-run and commit `dist/editor.js` after any change in `src/`.** Size: see "Fence languages" below. |
| `npm test` | `node:test` suites in `test/` (TypeScript is run directly through `tsx`; no DOM needed) |
| `npm run dev` | esbuild watch + static server on http://localhost:8000/index.html (`?mode=source` for source mode). Opened outside Qt, the page uses a mock bridge that logs calls to `console.log` and loads a sample document. The dev build is unminified with a sourcemap; run `npm run build` before committing. |
| `npm run typecheck` | `tsc --noEmit` |

## Files

- `index.html` - page; strict CSP; `<div id="write"><div id="editor">`; loads `qrc:///qtwebchannel/qwebchannel.js` then `dist/editor.js`.
- `src/extras.ts` - footnotes, `[toc]` and front matter in live mode (see Footnotes, TOC and front matter below).
- `src/images.ts` - paste / drop of images (see "Pasting images" below).
- `src/links.ts` - in-page link editor panel, host command `link` / Mod-k (see "Links" below).
- `src/math.ts` - math parsing + KaTeX rendering (see Math below). `dist/katex.css` + `dist/fonts/*.woff2` - KaTeX assets copied by `esbuild.mjs` (committed).
- `editor.css` - base styles for both modes (themes override via `#write`).
- `src/main.ts` - bootstrap + bridge glue. `src/bridge.ts` - QWebChannel connection + mock. `src/changes.ts` - ChangeSet <-> JSON. `src/live.ts` - live-preview decorations. `src/blocks.ts` + `blocks.css` - live-mode block rendering (phase 2). `src/languages.ts` - curated fenced-code languages. `src/tables.ts` - grid rendering of GFM tables in live mode. `src/modes.ts` - extensions, mode switching, appearance JSON plumbing, `Session`. `src/appearance.ts` - Source/Code appearances, focus and typewriter modes. `src/livetheme.ts` - Live theme overlay, exact switch and readability fill.

## Bridge contract (`bridge`)

JS **calls** (slots):

- `ready()` - once mounted and connected (all signals below are connected before this call).
- `documentChanged(changesJson: string, revision: number)` - one call per user doc-changing transaction. `changesJson` = JSON array of `{from, to, insert}`; offsets are UTF-16 offsets into the **pre-change** document, ascending by `from`, non-overlapping, so applying them in reverse order needs no adjustment. `revision` = new revision counter (incremented per doc-changing transaction). Includes undo/redo and `simulateUserChanges`; excludes `setDocument`/`applyChanges`.
- `cursorChanged(anchor: number, head: number, byUser: boolean)` - main selection range, debounced 30 ms; `byUser` is true when a transaction carrying a user event (typing, keys, pointer) moved it and false for host-driven changes (`setDocument`, `setCursor`, `applyChanges`); also sent immediately (0, 0, false) after `setDocument`.
- `metric(name: string, ms: number)` - `keystroke-to-dispatch` (keydown/beforeinput/input -> `documentChanged`), `setDocument`, `decorate` (slowest live-decoration rebuild per 100 ms window).
- `log(message: string)`.
- `textReply(token: number, text: string)` - answer to `requestText`.
- `scrolled(fraction: number)` - the editor's vertical position as a 0..1 fraction, debounced 60 ms, for host pane sync (not sent while applying `scrollToFraction`).
- `requestImage(token: number, src: string)` - live mode asks the host for an image. `src` is the raw URL from the markdown (angle brackets stripped); the host resolves it relative to the document, and answers with `imageReply` (below) using the same `token`. One request per distinct `src`; replies are cached by `src` until the next `setDocument`, and failed `src`s are retried after 5 s the next time their widget is created. Hosts without this slot make images show the placeholder.
- `saveImage(token: number, name: string, mime: string, base64: string)` - the user pasted or dropped an image (see "Pasting images"). `name` is a suggested file name (the file's own name, extension ensured, or `pasted-image-YYYYMMDD-HHMMSS.<ext>`), `mime` is one of `image/png|jpeg|gif|webp`, `base64` the raw bytes (no `data:` prefix). The host saves the file next to the document and answers with `imageSaved` (below) using the same `token`. Anything else (other MIME types, empty, > 20 MiB decoded) is rejected in JS with a `log` message and never reaches the host. Hosts without this slot leave paste/drop to the browser's default.

JS **connects to** (signals):

- `setDocument(text, revision)` - replace the document; resets undo history, selection (to 0) and revision.
- `applyChanges(changesJson, revision)` - apply external changes (same JSON shape, pre-change offsets); not echoed; not added to undo history; sets revision. Malformed/out-of-range input is rejected and reported with `log`.
- `setMode("source" | "live")`.
- `setTheme(css)` - sets `<style id="fomawrite-theme">`; empty string removes it.
- `setAppearance(json)` - `{fontFamily, fontSize, lineHeight, dark, typewriter, focus, appearance, liveThemeFilter, palette}`; unknown keys are ignored, missing keys leave the previous value. `fontFamily/fontSize/lineHeight` -> CSS vars `--fw-font`, `--fw-font-size` (px), `--fw-line-height` on `#write` (they override the appearance defaults below); `dark` toggles `html.dark`. The remaining keys are owned by `src/appearance.ts` (see "Appearances"): `appearance` (`"manuscript" | "editorial" | "book" | "code"`) -> class `fw-appearance-<name>` on `#write`; `focus` -> `#write.fw-focus`; `typewriter` -> `#write.fw-typewriter`. **In Live mode the `appearance` classes are not applied** (only `fontFamily` / `fontSize` as the base face / size, `dark`, `typewriter`, `focus`); switching modes re-evaluates them. Two optional keys belong to the Live theme filter (see "Live theme filter"): `liveThemeFilter` (boolean, default `true`; `false` = Live follows the theme exactly, overlay off) and `palette` (`{"background": "<css colour>", "text": "<css colour>"}`; the host's readable pair, used only when the theme sets no background / text colour on `#write`).
- `imageReply(token: number, dataUrl: string, error: string)` - answer to `requestImage`. On success `dataUrl` is a `data:image/...` URL and `error` is `""`; on failure `error` is a short message (shown as the placeholder tooltip) and `dataUrl` is ignored. Anything that is not a `data:image/` URL is treated as an error. Unknown/stale tokens are ignored. In mock mode the page answers itself with a 1x1 PNG after 50 ms.
- `imageSaved(token: number, relativePath: string, error: string)` - answer to `saveImage`. On success `error` is `""` and `relativePath` is the saved file's path relative to the document (the host may rename to avoid clashes; `/` separators); the editor inserts `![stem](relativePath)` (spaces, parentheses and `%` in the path are percent-encoded as `%20` `%28` `%29` `%25`; the alt is the file stem) at the paste/drop position. On failure `error` is the host's message (e.g. "save the document first"): nothing is inserted and the message is passed to `log`. Unknown/stale tokens (e.g. after `setDocument`) insert nothing. In mock mode the page answers itself with `imageSaved(token, "assets/" + name, "")` after 50 ms.
- `command(name: string)` - run an editor command from the host: `find`/`replace` open CodeMirror's search panel, `selectAll` selects the document, `link` opens the link editor panel (see "Links"); unknown names are logged.
- `setCursor(pos: number)` - place the caret at a UTF-16 offset (clamped) and scroll it into view.
- `scrollToFraction(fraction: number)` - scroll the editor to a 0..1 fraction without echoing `scrolled`.
- `focusEditor()`.
- `requestText(token)` -> JS calls `bridge.textReply(token, fullText)`.
- Test hooks (the CSP forbids eval): `simulateUserChanges(changesJson)` applies the changes as an ordinary user transaction (`userEvent: "input"`), so it **is** reported via `documentChanged`; `undo()` / `redo()` run CM6's commands (resulting changes are reported like user edits).

`window.fomawriteEditor = { getText, setText, setMode, getMode, getRevision, simulateUserChanges, undo, redo, view }` for `runJavaScript`-driven tests. `setText(text)` is `setDocument(text, currentRevision)`.

### Notes

- Document text inside the editor is LF-normalised by CodeMirror (CRLF/CR in `setDocument` text become LF). Pass LF text from Qt so offsets agree.
- Offsets are UTF-16 code units (JS string indices), i.e. the same as `QString` indices.
- If `window.qt.webChannelTransport` or `window.QWebChannel` is missing, or the channel has no `bridge` object, a mock bridge is used.

## Live mode behaviour

Built from the Lezer tree for visible ranges only (rebuilt on doc / viewport / selection / parse changes). Markers (`HeaderMark` + following space, `EmphasisMark`, inline `CodeMark`, `StrikethroughMark`, link `[`, `](url "title")`, `QuoteMark` + following space) are replaced by zero-width decorations and registered as atomic ranges. If any selection range touches a node (inclusive of both ends), its markers stay visible and the node gets `.fw-revealed`; for `QuoteMark` the "node" is its line. Line classes: `fw-h1..6`, `fw-quote-line`, `fw-list-line`, `fw-code-line`; mark classes: `fw-em`, `fw-strong`, `fw-code`, `fw-strike`, `fw-live-link`, `fw-list-mark`. Math: see the Math section below. Diagrams are not touched yet.

Source mode keeps all syntax visible and highlights it with `.fw-heading`, `.fw-emphasis`, `.fw-strong`, `.fw-code`, `.fw-link`, `.fw-quote`, `.fw-list-mark`, `.fw-strike`, `.fw-mark` (the syntax characters).

## Appearances (Source mode / Code mode)

`appearance.ts` is registered once in `main.ts` (`appearanceExtension()`); everything arrives through `setAppearance`. `Session.setAppearance` dispatches an effect, the extension reconfigures its compartments (line wrapping, code extras, hanging markers, focus) in the same transaction and mirrors the state onto `#write` as classes - the document is never reloaded. The state survives `setDocument` and mode switches. Appearance CSS is scoped to source mode (`:not(.fw-mode-live)`); in live mode the compartments fall back to wrapping/no extras. Until an `appearance` is sent the page keeps its base look.

| `appearance` | Look |
| --- | --- |
| `manuscript` | bundled iA Writer Mono S (`qrc:/fonts/iAWriterMonoS-*.ttf`, falls back to Menlo), 18px / 1.6, 680px column. Heading `#`s, list markers (with nesting indent) and `>` quotes hang in a 4ch left gutter outside the body column (`.fw-hang` line decoration + `text-indent`, wrapped lines align with the body). |
| `editorial` | system sans (`-apple-system, SF Pro Text, ...`), 19px / 1.6. |
| `book` | Georgia, 20px / 1.65. |
| `code` | Menlo 15px / 1.5, full width, no wrapping (horizontal scroll), line numbers, 4-column indentation guides (`.fw-guided` + CSS gradient), Tab / Shift-Tab = `indentWithTab` with a 4-space indent unit (beats the list-aware Tab handler). |

- **Focus** (`focus: true`): every line outside the caret's paragraph (run of non-blank lines; a blank line is its own paragraph; selections keep every covered paragraph lit) gets `.fw-dim` (opacity `--fw-dim-opacity`).
- **Typewriter** (`typewriter: true`): after typing, deleting, keyboard caret movement or undo/redo, and once when switched on, the caret line is scrolled to the vertical middle (`EditorView.scrollIntoView(pos, {y: "center"})`); pointer clicks and host edits do not scroll. `#write` gets 50vh top/bottom padding so the first/last line can reach the middle.
- Fonts: `@font-face` for iA Writer Mono S (regular/italic/bold/bold italic) in `editor.css`; the CSP already allows `font-src qrc:`. Every new colour is a CSS variable with a `:root.dark` value.
### Live theme filter (`src/livetheme.ts`)

Live follows the publishing theme, filtered (design: `docs/live-appearance-design.md`). `setTheme` CSS is applied as before; in Live a second stylesheet, `<style id="fomawrite-live-overlay">`, is kept **after** the theme element (`applyThemeDom` inserts the theme before it) so it wins on precedence. Every rule is scoped to `#write.fw-mode-live:not(.fw-live-exact)`, so Source is untouched.

- **Kept** (theme value stands): font-family / size / weight / style, colour, line-height, letter-spacing, text-decoration, list markers, heading styles, code and quote colouring, table borders and alignment, link colour.
- **Neutralised** (`!important`): on `#write` position, float, columns, transform, box-shadow, background-image, `::before` / `::after`, and width / max-width / margin / padding (reset to the editor's own 760px column, `48px 40px 40vh`, 50vh top/bottom with typewriter); on `#write > *` and block containers (p, ul, ol, blockquote, pre, table, figure, section ...) float, columns, transform, box-shadow, background-image, max-width 100%; `position: static` on pre / table / figure / img / hr / section-like blocks (not on li, blockquote, headings or paragraphs, whose pseudo-elements are often markers). The caret follows the text colour.
- **Exact switch**: `liveThemeFilter: false` removes the overlay and mapped-theme elements, clears `fw-live-filtered` from `<html>` and sets `fw-live-exact` on `#write` (Typora-exact Live). Default is `true`.
- **Readability**: in Live with the filter on, after each theme / appearance / mode change the page measures `getComputedStyle(#write)` (with its own fills removed first). A transparent / `rgba(0, 0, 0, 0)` background, or a text colour equal to `<html>`'s (the editor default), counts as "theme did not set it" and is filled from `palette` via `--fw-live-bg` / `--fw-live-fg` on `#write` plus classes `fw-live-fill-bg` / `fw-live-fill-fg` (pure rule: `pickReadableColours(themeBg, themeFg, palette)`). Both set: untouched. No palette: nothing filled.
- **Host text size wins**: once the host has sent a numeric `fontSize` (`setAppearance`; Larger / Smaller / Reset keep sending one), `#write` gets `fw-host-size` and the overlay sets `font-size: var(--fw-font-size) !important` on it, so a theme's `#write { font-size: 30px }` no longer wins in filtered Live. Before the host has sent a size the theme's size stands. Mapped heading sizes stay relative (a theme's `rem` becomes `em`), so the theme's scale follows the host size. Line height and font family are not forced (`--fw-line-height` is only the base under the theme's; `fontFamily` is only the base face).
- **`<html>` / `<body>`**: while Live is active with the filter on, `<html>` carries `fw-live-filtered`; the overlay rule `html.fw-live-filtered body` resets margin, padding, background-image and columns (`!important`; colours are left alone). The class is removed in Source and in exact mode.
- **Selector mapping** (`src/thememap.ts`, pure `mapThemeCss(css): string`): theme rules that style real elements cannot match Live text lines (`.cm-line` divs with classes), so a *mapped copy* of each such rule is appended in `<style id="fomawrite-theme-mapped">`, kept between the theme and the overlay (`theme, mapped, overlay` in `<head>`; present iff the filter is on and the theme maps to something; rebuilt by `syncLiveTheme` after `setTheme`). Each mapped selector is scoped to `#write.fw-mode-live:not(.fw-live-exact)`, so its specificity beats the editor's own Live rules, and the theme's own rule is untouched. The splitter ignores comments, descends into `@media` / `@supports`, passes `@font-face` / `@keyframes` through untouched and drops other at-rules and rules with nested CSS.

  | Theme selector (subject) | Live selector | Properties kept in the copy |
  | --- | --- | --- |
  | `h1` .. `h6` | `.cm-line.fw-h1` .. `.fw-h6` | typography + `border-left*` / `border-bottom*` |
  | `blockquote` | `.cm-line.fw-quote-line` | typography + `border-left*` / `border-bottom*`; `padding-left` only for depth-1 quotes (`.fw-quote-d1`, forced `!important` past the editor's own indent) |
  | `li` (`ul` / `ol` ancestors drop out) | `.cm-line.fw-list-line` | typography |
  | `p` | `.cm-line` except heading, list, code-fence, table, front-matter, math-source and footnote-definition lines | typography; an absolute `font-size` (px, pt, ...) is dropped so the host size wins |
  | `blockquote > p`, `blockquote li`, `li p` | ancestor classes merged into one `.cm-line` compound | by the subject's kind |
  | `code` (not under `pre`) | `.fw-code` | typography + `background-color` (`background: <colour>` becomes `background-color`) |
  | `a`, `a:hover`, `a:focus`, `a:active` | `.fw-live-link` (+ the state) | typography |
  | `strong`, `b` | `.fw-strong` | typography |
  | `em`, `i` | `.fw-em` | typography |
  | `del`, `s`, `strike` | `.fw-strike` | typography |

  "Typography" is `font*`, `color`, `line-height`, `letter-spacing`, `word-spacing`, `text-decoration*`, `text-transform`, `text-underline-offset|position`. Combinators `>` and descendant both become descendant; `#write >` is dropped (`#write` becomes the scope, extra classes on it are kept); a bare `#write` rule needs no mapping. `!important` is kept. Never emitted: position, float, width / max-width, margin, padding (except the quote case above), display, columns, transform, background-image, box-shadow, list-style, text-align, `content`, custom properties, anything with `url()`. **Not mapped** (the whole selector is skipped): `pre`, `table` (and cells), `img`, `hr` and any selector through them (`pre code`, `table td`), which are real elements in Live widgets the theme already reaches; `+` / `~` sibling combinators; elements with classes, ids, attributes or structural pseudo-classes (`h1.title`, `a[href]`, `p:first-child`, `li:nth-child`, `:not()`, `a:visited`); pseudo-elements (`h1::before`, `li::marker`); `ul` / `ol` as a subject; unknown elements, `div`, `span`, `*`, `body`.
- Limits: a block rule beats a plain `p` rule on the same line (`.cm-line` is both the block and its paragraph; in a rendered quote the inner `p` would win); `ul` and `ol` are not told apart; absolute (`px`) heading sizes are kept as the theme wrote them and do not follow Larger / Smaller; the host `fontFamily` is only the base face under the theme's; rules on `html` / `body` other than the reset above are not filtered (use the exact switch). Tests: `test/thememap.test.mjs`, `test/livetheme.test.mjs`.

### Blocks (phase 2, `src/blocks.ts`)

Same reveal rule as above (a selection range touching the node shows the raw markdown; for task markers the whole line counts). Blocks render only in live mode and only for visible ranges. Because a ViewPlugin cannot supply block decorations, the widgets are inline replace decorations; ones that stand alone on a line are styled as full-width blocks.

- **Images** `![alt](src "title")`: replaced by `.fw-image` (`.fw-image-block` when alone on its line) holding an `<img class="fw-image-img">`, or, while the host has not answered / on error, `.fw-image-placeholder` with the alt text. Reference images, unclosed syntax and images spanning lines stay raw. Clicking puts the caret at the node start (reveals the markdown).
- **Task items**: `TaskMarker` -> `<input type=checkbox class="fw-task">`; clicking toggles `[ ]`/`[x]` with an ordinary user transaction (`userEvent: "input.toggle"`), so it is reported via `documentChanged` and undoable.
- **Horizontal rules** -> `<hr class="fw-hr">`.
- **Fenced code**: lines get `fw-code-open` / `fw-code-close`; when the caret is outside the block the opening line (backticks + info string) is replaced by a `.fw-code-label` showing the info string, the closing fence by an empty `.fw-fence-end`, and both lines get `fw-fence-hidden`. `markdown({ codeLanguages })` is fed the curated list in `src/languages.ts` (re-exported as `fenceLanguages` from `blocks.ts`), so fenced code is parsed per language and coloured with `.fw-tok-*` classes (colours are CSS variables in `blocks.css`). See "Fence languages" below. The nested trees are never walked as markdown (`IterMode.IgnoreMounts`).
- **Tables**: raw (selection touches the table): `fw-table-line` on every line, `fw-table-delim-line` + `.fw-table-delim` on the delimiter row. Rendered: see "Tables" below.
- **Blockquotes**: `fw-quote-line` (phase 1) plus `fw-quote-d1..d4` by nesting depth; `blocks.css` adds a bar and indent per level.

`blocksExtension(bridge)` is registered from `main.ts`; it connects `bridge.imageReply` / `bridge.requestImage`.

### Fence languages (`src/languages.ts`)

**Bundle:** `dist/editor.js` is a single IIFE, and esbuild cannot code-split it, so every language that is imported is inlined. `@codemirror/language-data` (every grammar behind a lazy `import()`) was therefore +1.3 MB and a much slower page load; it has been replaced by an explicit, curated list. Current size: 1.42 MB raw / 474 KB gzip (was 1.93 MB / 653 KB), of which KaTeX is about 260 KB. Do not re-add `language-data`, and check `npm run build` output when adding a language (heavy ones: C/C++ ~100 KB, PHP ~95 KB, Rust ~80 KB, JavaScript/TypeScript ~75 KB).

Matching is `LanguageDescription.matchLanguageName(fenceLanguages, info, true)` (what `lang-markdown` does), which compares only the lowercase `alias` list (the name is added automatically) and, in fuzzy mode, also accepts an info string that merely *contains* an alias longer than 2 characters, so keep aliases lowercase and avoid short, word-like ones. Grammars are built lazily in `load`. An info string that matches nothing (`text`, `plaintext`, `mermaid`, a typo) gets no nested parse: plain code, no error.

| Language | Info strings |
| --- | --- |
| JavaScript / TypeScript / JSX / TSX | `js` `mjs` `cjs` / `ts` `mts` `cts` / `jsx` / `tsx` |
| Python, Java, Go, Rust, PHP | `python` `py` `py3`, `java`, `go` `golang`, `rust` `rs`, `php` |
| C, C++, Objective-C, C#, Dart | `c` `h`, `c++` `cpp` `cc` `cxx` `hpp`, `objc` `objective-c` `mm`, `cs` `csharp`, `dart` |
| HTML, CSS, JSON, XML, SQL, YAML, Markdown | `html` `htm` `vue` `svelte` (all HTML), `css`, `json` `jsonc` `json5`, `xml` `svg` `plist`, `sql` `mysql` `postgres` `sqlite`, `yaml` `yml`, `markdown` `md` `mdx` |
| Shell, PowerShell, Dockerfile, Makefile, Nginx | `sh` `bash` `zsh` `ksh` `fish` `console`, `powershell` `ps1` `pwsh`, `dockerfile` `docker`, `makefile` `make` `mk` (shell grammar, there is no Makefile mode), `nginx` |
| Swift, Kotlin, Ruby, Scala, Perl, Lua, R, Haskell, Clojure | `swift`, `kotlin` `kt` `kts`, `ruby` `rb`, `scala`, `perl` `pl`, `lua`, `r`, `haskell` `hs`, `clojure` `clj` `cljs` `edn` |
| TOML, INI / properties, Diff | `toml`, `ini` `properties` `dotenv` `editorconfig`, `diff` `patch` |

Not highlighted (plain code): `text` / `plaintext` / `txt`, `mermaid`, `latex`, `graphql`, `csv`, `protobuf`, `elixir`, `zig` and everything else not listed. Tests: `test/languages.test.mjs`.

### Tables (live mode, `src/tables.ts`)

When no selection range touches a top-level GFM `Table` (inclusive of both ends), the whole table (first line start to last line end) is replaced by one **block** widget holding `<table class="fw-table">` (`th`/`td` get `fw-align-left|center|right` from the `:--` / `:-:` / `--:` delimiter row; cell text supports `**` `*` `_` `~~` `` ` `` `$tex$` (inline math rendered with KaTeX through `math.ts`'s shared `inlineMathEnd` rule and render cache; a cell's `\|` reaches KaTeX as `|`; `$5 and $10` stays text) `[text](url)` `<br>` and backslash escapes such as `\|`, built as DOM nodes with no `innerHTML`, and only http(s)/mailto/tel/relative links). A caret or selection inside the table shows the raw lines instead (the reveal rule). Clicking a cell puts the caret at the start of that cell's source text (which reveals the raw table); Escape inside a table, or moving out, brings the grid back; Up/Down/Left/Right from the line adjacent to a grid step into it. **Multi-line replacement:** CodeMirror forbids block decorations and line-break-spanning replaces from a ViewPlugin *or* a view-function facet, so (unlike `live.ts`'s per-line hiding) the replace decoration comes from a `StateField` through `EditorView.decorations.from` and is a single `Decoration.replace({ widget, block: true })`; it is also an atomic range and reports `estimatedHeight`. A StateField cannot see the Live/Source compartment, so a small ViewPlugin mirrors `view.plugin(liveExtension)` into the field through a `setLive` effect (dispatched from a microtask on mode switch / new state); the field is empty in Source mode. The parser (`splitRow`, `parseTableText`, `parseInline`) is pure: cells are split on unescaped pipes (empty cells included), rows are padded/truncated to the delimiter row's column count, and anything without a valid delimiter row, or nested in a quote/list, stays raw. Tables are recognised in the syntax tree parsed so far. `buildTableDecorations(state, selection)` is the pure, node-testable builder; `buildTableDom` / `TableWidget.toDOM` / the key commands need a DOM and are not covered by `npm test`. Dark variants live in `blocks.css` (`--fw-table-*`).
### Math (`src/math.ts`, KaTeX)

Parsing (`mathMarkdown`, a `@lezer/markdown` extension added to `markdown({ extensions })` in `modes.ts`, so both modes parse it): inline `$tex$` -> `InlineMath` with two `MathMark`s; block `$$` on its own line ... `$$` on its own line -> `MathBlock` with `MathMark` lines (also the one-line form `$$tex$$`). Inline rules (Pandoc's): the opening `$` is not followed by whitespace and not adjacent to another `$`; the first unescaped `$` after it must close (non-space before it, not followed by a digit) or the opening `$` is text, so `$5 and $10` stays text; no line break inside; `\$` never closes. A block needs a closing `$$` line before the next blank line (an unclosed `$$` stays a paragraph); it interrupts a paragraph and works inside blockquotes.

Live mode (`mathExtension()`, registered from `main.ts`; same reveal rule as above - any selection range touching the node shows the raw TeX, marked `.fw-math-src` / `.fw-math-src-line`): `InlineMath` -> inline `.fw-math-render.fw-math-inline` widget; `MathBlock` -> a `.fw-math-display` widget replacing its first line, every other line hidden and collapsed to zero height (`.fw-math-collapsed`), as fenced code does line by line. Clicking a widget puts the caret inside the node. HTML comes from `katex.renderToString` (`throwOnError: true` so the message is available); a TeX error renders the TeX in `<span class="fw-math-error" title="message">`. Rendered HTML is cached per TeX string and mode in `mathCache`, cleared when a new editor state is created (`setDocument`). Source mode only colours math spans with `.fw-math` (a highlight tag). The styles are an `EditorView.baseTheme` inside `math.ts`.

**Assets** (generated by `esbuild.mjs` from `node_modules/katex` on every build, committed): `dist/katex.css` (`katex.min.css` with the woff/ttf fallbacks removed; every `url()` is `fonts/<name>.woff2`, relative to the stylesheet) linked from `index.html` after `blocks.css`, and the 20 `dist/fonts/KaTeX_*.woff2` files. All of them must be listed in `src/resources.qrc` under `editor/dist/`. The CSP's `font-src qrc:` covers them (on the http dev server the CSP blocks the fonts, so equations show in a fallback face there).

### Footnotes, TOC and front matter (`src/extras.ts`)

Typora conventions in Live mode. `extrasMarkdown` (a `@lezer/markdown` extension added to `markdown({ extensions })` in `modes.ts`, so both modes parse it) adds `FootnoteRef` (`[^id]`, inline, before `Link`), `FootnoteDef` (`[^id]: text` at the start of a top-level line, continuing over following lines indented 2+ columns and over indented-4+ paragraphs after blank lines; the text is parsed as inline markdown; a `FootnoteLabel` child covers `[^id]:`), `TocBlock` (`[toc]`, any case, alone on a line; it does not interrupt a paragraph) and `FrontMatter` (a `---` line at offset 0 up to the next `---` or `...` line; without a closing fence it stays a horizontal rule, and a document that does not start with `---` never has front matter). `extrasExtension()` (registered from `main.ts`; a ViewPlugin plus atomic ranges, Live mode only, same reveal rule as everywhere: a selection range touching the node shows the raw text) renders: refs as `<sup class="fw-footnote-ref">n</sup>` numbered by first reference (definitions nobody references are numbered after them; a ref with no definition gets `.fw-footnote-missing`), definition lines get `fw-footnote-def` and `[^id]: ` becomes a `n.` label widget while the caret is off the definition's first line; `[toc]` becomes a `.fw-toc` widget with nested `<ul>` of the ATX headings of levels 1-3 (clicking an entry puts the caret in that heading and scrolls it to the top; the widget identity is levels + texts, so edits that do not change a heading leave its DOM alone); front matter collapses to a one-line `.fw-front-matter` widget `front matter · N fields` (N = top-level `key:` lines; the other lines are hidden and collapsed like a math block) or, when touched, shows raw lines with `fw-front-matter-line`. The footnote numbering and the heading list are derived from the syntax tree and cached per tree object (`footnoteInfo`, `collectHeadings`). Styles are the `/* extras */` section of `blocks.css` (`--fw-footnote*`, `--fw-toc-*`, `--fw-front-matter-*`, with `:root.dark` values). Because the caret starts at offset 0 when a document is opened, leading front matter starts out revealed until the caret moves away.

### Pasting images (`src/images.ts`)

`imagesExtension(bridge)` (registered from `main.ts`, both modes) handles, through `EditorView.domEventHandlers`, `paste` events whose clipboard holds an image (screenshot pastes via `clipboardData.items`, Finder file copies via `clipboardData.files`) and `drop` of image `File`s (several files at once are allowed; `dragover` with files is accepted so the drop can happen). Text pastes, and drops without an image file, fall through to CodeMirror untouched. Each acceptable image (png/jpeg/gif/webp, 1 byte to 20 MiB; checked on the `File` and again on the decoded base64) gets a token and a *pending position* - the caret for paste, `posAtCoords` of the drop point - and is sent with `saveImage`. **Nothing is inserted until `imageSaved` arrives**: pending positions live in a `StateField`, are mapped through every document change (`ChangeSet.mapPos`, assoc -1, so text typed at the spot ends up after the image), and the image is inserted by an ordinary user transaction (`userEvent: "input.image"`: reported via `documentChanged`, undoable, caret moves behind it if it was still at that spot). A multi-file drop inserts one image per line in file order, whatever order the host answers in (all but the last image carry a trailing newline). Errors, rejected files and stale tokens only produce `log` messages; a replaced document (`setDocument`) forgets all pending saves. Tests: `test/images.test.mjs`.

### Links (`src/links.ts`)

`linksExtension()` (registered from `main.ts`, both modes) adds a CodeMirror panel at the top of the editor (the search panel's mechanism: `showPanel` fed by the `linkField` StateField, toggled by `openLinkEffect` / `closeLinkEffect`) with **Label**, **URL** and optional **Title** fields and **Apply**, **Remove** (visible only when editing an existing link) and **Cancel** buttons. It opens on the host command `link` (`runCommand("link")` from Qt) and on **Mod-k** inside the page; pressing Mod-k while it is open re-reads the selection. Styles: `/* link editor panel */` in `editor.css` (`--fw-*` variables, light and dark; the same rules also give the search panel's inputs/buttons page colours in dark).

- **What is edited.** If the main selection is inside or touching (both ends inclusive) an inline Markdown link `[label](url "title")` (Lezer `Link` node with four direct `LinkMark`s `[ ] ( )`, plus `URL` / `LinkTitle`; reference links, shortcut references, images and autolinks do not count), the fields are prefilled from it (label with `\[` `\]` unescaped, URL with `%20 %28 %29` decoded - and `%25` for scheme-less paths - and `<...>` stripped, title unquoted from `"..."`, `'...'` or `(...)`) and Apply replaces the whole link. Otherwise Label is the selected text (single line only; a multi-line selection is not consumed: the link is inserted at its end with an empty Label) and Apply inserts at the selection, replacing the selected text.
- **Text written.** `[label](url "title")`, title omitted when empty. `[` `]` in the label are backslash-escaped (line breaks become spaces); the title is quoted with `"` and `"` / `\` escaped. URL: spaces and parentheses are always percent-encoded; a URL starting with a scheme (`https:`, `mailto:` ...) is otherwise kept as typed, a scheme-less path is encoded exactly like an image path (`encodeImagePath`, so `%` becomes `%25`). Empty Label falls back to the URL text; an empty URL does not apply (the URL field is marked invalid and focused). Applying unchanged values to an existing link changes nothing.
- **Remove** replaces the existing link with its label text exactly as written in the document.
- **Transactions.** Apply is an ordinary user transaction with `userEvent: "input.link"`, Remove `"delete.link"`: both reach the host through `documentChanged` and are undoable. The caret ends just after the inserted link (or after the label text on Remove) and the editor regains focus after Apply, Remove and Cancel. Cancel / Escape closes the panel and changes nothing.
- **Focus and keys.** On open the Label field is focused when it is empty, else the URL field (text selected). Enter in any field applies; Escape cancels.

Pure helpers (`findLinkAt`, `linkContext`, `buildLinkMarkdown`, `encodeLinkUrl`, `applyReplacement`, `removeReplacement`, `replacementSpec`, ...) are exported and unit-tested without a DOM in `test/links.test.mjs`; the panel DOM is not covered by `npm test`.
