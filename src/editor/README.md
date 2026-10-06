# Fomawrite editor (web side)

CodeMirror 6 Markdown editor bundle hosted by the Qt app in a `WebEngineView` (`qrc:/editor/index.html`)
and driven over `QWebChannel`. The Qt build has no node step: `dist/editor.js` is **committed**.

## Commands (run in `src/editor/`)

| Command | What |
| --- | --- |
| `npm ci` | install pinned deps |
| `npm run build` | bundle `src/main.ts` -> `dist/editor.js` (IIFE, minified, chrome120, no runtime deps). **Re-run and commit `dist/editor.js` after any change in `src/`.** |
| `npm test` | `node:test` suites in `test/` (TypeScript is run directly through `tsx`; no DOM needed) |
| `npm run dev` | esbuild watch + static server on http://localhost:8000/index.html (`?mode=source` for source mode). Opened outside Qt, the page uses a mock bridge that logs calls to `console.log` and loads a sample document. The dev build is unminified with a sourcemap; run `npm run build` before committing. |
| `npm run typecheck` | `tsc --noEmit` |

## Files

- `index.html` - page; strict CSP; `<div id="write"><div id="editor">`; loads `qrc:///qtwebchannel/qwebchannel.js` then `dist/editor.js`.
- `src/math.ts` - math parsing + KaTeX rendering (see Math below). `dist/katex.css` + `dist/fonts/*.woff2` - KaTeX assets copied by `esbuild.mjs` (committed).
- `editor.css` - base styles for both modes (themes override via `#write`).
- `src/main.ts` - bootstrap + bridge glue. `src/bridge.ts` - QWebChannel connection + mock. `src/changes.ts` - ChangeSet <-> JSON. `src/live.ts` - live-preview decorations. `src/blocks.ts` + `blocks.css` - live-mode block rendering (phase 2). `src/tables.ts` - grid rendering of GFM tables in live mode. `src/modes.ts` - extensions, mode switching, appearance JSON plumbing, `Session`. `src/appearance.ts` - Source/Code appearances, focus and typewriter modes.

## Bridge contract (`bridge`)

JS **calls** (slots):

- `ready()` - once mounted and connected (all signals below are connected before this call).
- `documentChanged(changesJson: string, revision: number)` - one call per user doc-changing transaction. `changesJson` = JSON array of `{from, to, insert}`; offsets are UTF-16 offsets into the **pre-change** document, ascending by `from`, non-overlapping, so applying them in reverse order needs no adjustment. `revision` = new revision counter (incremented per doc-changing transaction). Includes undo/redo and `simulateUserChanges`; excludes `setDocument`/`applyChanges`.
- `cursorChanged(anchor: number, head: number)` - main selection range, debounced 30 ms; also sent immediately (0, 0) after `setDocument`.
- `metric(name: string, ms: number)` - `keystroke-to-dispatch` (keydown/beforeinput/input -> `documentChanged`), `setDocument`, `decorate` (slowest live-decoration rebuild per 100 ms window).
- `log(message: string)`.
- `textReply(token: number, text: string)` - answer to `requestText`.
- `scrolled(fraction: number)` - the editor's vertical position as a 0..1 fraction, debounced 60 ms, for host pane sync (not sent while applying `scrollToFraction`).
- `requestImage(token: number, src: string)` - live mode asks the host for an image. `src` is the raw URL from the markdown (angle brackets stripped); the host resolves it relative to the document, and answers with `imageReply` (below) using the same `token`. One request per distinct `src`; replies are cached by `src` until the next `setDocument`, and failed `src`s are retried after 5 s the next time their widget is created. Hosts without this slot make images show the placeholder.

JS **connects to** (signals):

- `setDocument(text, revision)` - replace the document; resets undo history, selection (to 0) and revision.
- `applyChanges(changesJson, revision)` - apply external changes (same JSON shape, pre-change offsets); not echoed; not added to undo history; sets revision. Malformed/out-of-range input is rejected and reported with `log`.
- `setMode("source" | "live")`.
- `setTheme(css)` - sets `<style id="fomawrite-theme">`; empty string removes it.
- `setAppearance(json)` - `{fontFamily, fontSize, lineHeight, dark, typewriter, focus, appearance}`; unknown keys are ignored, missing keys leave the previous value. `fontFamily/fontSize/lineHeight` -> CSS vars `--fw-font`, `--fw-font-size` (px), `--fw-line-height` on `#write` (they override the appearance defaults below); `dark` toggles `html.dark`. The remaining keys are owned by `src/appearance.ts` (see "Appearances"): `appearance` (`"manuscript" | "editorial" | "book" | "code"`) -> class `fw-appearance-<name>` on `#write`; `focus` -> `#write.fw-focus`; `typewriter` -> `#write.fw-typewriter`.
- `imageReply(token: number, dataUrl: string, error: string)` - answer to `requestImage`. On success `dataUrl` is a `data:image/...` URL and `error` is `""`; on failure `error` is a short message (shown as the placeholder tooltip) and `dataUrl` is ignored. Anything that is not a `data:image/` URL is treated as an error. Unknown/stale tokens are ignored. In mock mode the page answers itself with a 1x1 PNG after 50 ms.
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
### Blocks (phase 2, `src/blocks.ts`)

Same reveal rule as above (a selection range touching the node shows the raw markdown; for task markers the whole line counts). Blocks render only in live mode and only for visible ranges. Because a ViewPlugin cannot supply block decorations, the widgets are inline replace decorations; ones that stand alone on a line are styled as full-width blocks.

- **Images** `![alt](src "title")`: replaced by `.fw-image` (`.fw-image-block` when alone on its line) holding an `<img class="fw-image-img">`, or, while the host has not answered / on error, `.fw-image-placeholder` with the alt text. Reference images, unclosed syntax and images spanning lines stay raw. Clicking puts the caret at the node start (reveals the markdown).
- **Task items**: `TaskMarker` -> `<input type=checkbox class="fw-task">`; clicking toggles `[ ]`/`[x]` with an ordinary user transaction (`userEvent: "input.toggle"`), so it is reported via `documentChanged` and undoable.
- **Horizontal rules** -> `<hr class="fw-hr">`.
- **Fenced code**: lines get `fw-code-open` / `fw-code-close`; when the caret is outside the block the opening line (backticks + info string) is replaced by a `.fw-code-label` showing the info string, the closing fence by an empty `.fw-fence-end`, and both lines get `fw-fence-hidden`. `markdown({ codeLanguages })` is fed `@codemirror/language-data`, so fenced code is parsed per language and coloured with `.fw-tok-*` classes (colours are CSS variables in `blocks.css`). The nested trees are never walked as markdown (`IterMode.IgnoreMounts`).
- **Tables**: raw (selection touches the table): `fw-table-line` on every line, `fw-table-delim-line` + `.fw-table-delim` on the delimiter row. Rendered: see "Tables" below.
- **Blockquotes**: `fw-quote-line` (phase 1) plus `fw-quote-d1..d4` by nesting depth; `blocks.css` adds a bar and indent per level.

`blocksExtension(bridge)` is registered from `main.ts`; it connects `bridge.imageReply` / `bridge.requestImage`.

### Tables (live mode, `src/tables.ts`)

When no selection range touches a top-level GFM `Table` (inclusive of both ends), the whole table (first line start to last line end) is replaced by one **block** widget holding `<table class="fw-table">` (`th`/`td` get `fw-align-left|center|right` from the `:--` / `:-:` / `--:` delimiter row; cell text supports `**` `*` `_` `~~` `` ` `` `[text](url)` `<br>` and backslash escapes such as `\|`, built as DOM nodes with no `innerHTML`, and only http(s)/mailto/tel/relative links). A caret or selection inside the table shows the raw lines instead (the reveal rule). Clicking a cell puts the caret at the start of that cell's source text (which reveals the raw table); Escape inside a table, or moving out, brings the grid back; Up/Down/Left/Right from the line adjacent to a grid step into it. **Multi-line replacement:** CodeMirror forbids block decorations and line-break-spanning replaces from a ViewPlugin *or* a view-function facet, so (unlike `live.ts`'s per-line hiding) the replace decoration comes from a `StateField` through `EditorView.decorations.from` and is a single `Decoration.replace({ widget, block: true })`; it is also an atomic range and reports `estimatedHeight`. A StateField cannot see the Live/Source compartment, so a small ViewPlugin mirrors `view.plugin(liveExtension)` into the field through a `setLive` effect (dispatched from a microtask on mode switch / new state); the field is empty in Source mode. The parser (`splitRow`, `parseTableText`, `parseInline`) is pure: cells are split on unescaped pipes (empty cells included), rows are padded/truncated to the delimiter row's column count, and anything without a valid delimiter row, or nested in a quote/list, stays raw. Tables are recognised in the syntax tree parsed so far. `buildTableDecorations(state, selection)` is the pure, node-testable builder; `buildTableDom` / `TableWidget.toDOM` / the key commands need a DOM and are not covered by `npm test`. Dark variants live in `blocks.css` (`--fw-table-*`).
### Math (`src/math.ts`, KaTeX)

Parsing (`mathMarkdown`, a `@lezer/markdown` extension added to `markdown({ extensions })` in `modes.ts`, so both modes parse it): inline `$tex$` -> `InlineMath` with two `MathMark`s; block `$$` on its own line ... `$$` on its own line -> `MathBlock` with `MathMark` lines (also the one-line form `$$tex$$`). Inline rules (Pandoc's): the opening `$` is not followed by whitespace and not adjacent to another `$`; the first unescaped `$` after it must close (non-space before it, not followed by a digit) or the opening `$` is text, so `$5 and $10` stays text; no line break inside; `\$` never closes. A block needs a closing `$$` line before the next blank line (an unclosed `$$` stays a paragraph); it interrupts a paragraph and works inside blockquotes.

Live mode (`mathExtension()`, registered from `main.ts`; same reveal rule as above - any selection range touching the node shows the raw TeX, marked `.fw-math-src` / `.fw-math-src-line`): `InlineMath` -> inline `.fw-math-render.fw-math-inline` widget; `MathBlock` -> a `.fw-math-display` widget replacing its first line, every other line hidden and collapsed to zero height (`.fw-math-collapsed`), as fenced code does line by line. Clicking a widget puts the caret inside the node. HTML comes from `katex.renderToString` (`throwOnError: true` so the message is available); a TeX error renders the TeX in `<span class="fw-math-error" title="message">`. Rendered HTML is cached per TeX string and mode in `mathCache`, cleared when a new editor state is created (`setDocument`). Source mode only colours math spans with `.fw-math` (a highlight tag). The styles are an `EditorView.baseTheme` inside `math.ts`.

**Assets** (generated by `esbuild.mjs` from `node_modules/katex` on every build, committed): `dist/katex.css` (`katex.min.css` with the woff/ttf fallbacks removed; every `url()` is `fonts/<name>.woff2`, relative to the stylesheet) linked from `index.html` after `blocks.css`, and the 20 `dist/fonts/KaTeX_*.woff2` files. All of them must be listed in `src/resources.qrc` under `editor/dist/`. The CSP's `font-src qrc:` covers them (on the http dev server the CSP blocks the fonts, so equations show in a fallback face there).
