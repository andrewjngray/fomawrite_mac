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
- `editor.css` - base styles for both modes (themes override via `#write`).
- `src/main.ts` - bootstrap + bridge glue. `src/bridge.ts` - QWebChannel connection + mock. `src/changes.ts` - ChangeSet <-> JSON. `src/live.ts` - live-preview decorations. `src/modes.ts` - extensions, mode switching, appearance JSON plumbing, `Session`. `src/appearance.ts` - Source/Code appearances, focus and typewriter modes.

## Bridge contract (`bridge`)

JS **calls** (slots):

- `ready()` - once mounted and connected (all signals below are connected before this call).
- `documentChanged(changesJson: string, revision: number)` - one call per user doc-changing transaction. `changesJson` = JSON array of `{from, to, insert}`; offsets are UTF-16 offsets into the **pre-change** document, ascending by `from`, non-overlapping, so applying them in reverse order needs no adjustment. `revision` = new revision counter (incremented per doc-changing transaction). Includes undo/redo and `simulateUserChanges`; excludes `setDocument`/`applyChanges`.
- `cursorChanged(anchor: number, head: number)` - main selection range, debounced 30 ms; also sent immediately (0, 0) after `setDocument`.
- `metric(name: string, ms: number)` - `keystroke-to-dispatch` (keydown/beforeinput/input -> `documentChanged`), `setDocument`, `decorate` (slowest live-decoration rebuild per 100 ms window).
- `log(message: string)`.
- `textReply(token: number, text: string)` - answer to `requestText`.

JS **connects to** (signals):

- `setDocument(text, revision)` - replace the document; resets undo history, selection (to 0) and revision.
- `applyChanges(changesJson, revision)` - apply external changes (same JSON shape, pre-change offsets); not echoed; not added to undo history; sets revision. Malformed/out-of-range input is rejected and reported with `log`.
- `setMode("source" | "live")`.
- `setTheme(css)` - sets `<style id="fomawrite-theme">`; empty string removes it.
- `setAppearance(json)` - `{fontFamily, fontSize, lineHeight, dark, typewriter, focus, appearance}`; unknown keys are ignored, missing keys leave the previous value. `fontFamily/fontSize/lineHeight` -> CSS vars `--fw-font`, `--fw-font-size` (px), `--fw-line-height` on `#write` (they override the appearance defaults below); `dark` toggles `html.dark`. The remaining keys are owned by `src/appearance.ts` (see "Appearances"): `appearance` (`"manuscript" | "editorial" | "book" | "code"`) -> class `fw-appearance-<name>` on `#write`; `focus` -> `#write.fw-focus`; `typewriter` -> `#write.fw-typewriter`.
- `focusEditor()`.
- `requestText(token)` -> JS calls `bridge.textReply(token, fullText)`.
- Test hooks (the CSP forbids eval): `simulateUserChanges(changesJson)` applies the changes as an ordinary user transaction (`userEvent: "input"`), so it **is** reported via `documentChanged`; `undo()` / `redo()` run CM6's commands (resulting changes are reported like user edits).

`window.fomawriteEditor = { getText, setText, setMode, getMode, getRevision, simulateUserChanges, undo, redo, view }` for `runJavaScript`-driven tests. `setText(text)` is `setDocument(text, currentRevision)`.

### Notes

- Document text inside the editor is LF-normalised by CodeMirror (CRLF/CR in `setDocument` text become LF). Pass LF text from Qt so offsets agree.
- Offsets are UTF-16 code units (JS string indices), i.e. the same as `QString` indices.
- If `window.qt.webChannelTransport` or `window.QWebChannel` is missing, or the channel has no `bridge` object, a mock bridge is used.

## Live mode behaviour

Built from the Lezer tree for visible ranges only (rebuilt on doc / viewport / selection / parse changes). Markers (`HeaderMark` + following space, `EmphasisMark`, inline `CodeMark`, `StrikethroughMark`, link `[`, `](url "title")`, `QuoteMark` + following space) are replaced by zero-width decorations and registered as atomic ranges. If any selection range touches a node (inclusive of both ends), its markers stay visible and the node gets `.fw-revealed`; for `QuoteMark` the "node" is its line. Line classes: `fw-h1..6`, `fw-quote-line`, `fw-list-line`, `fw-code-line`; mark classes: `fw-em`, `fw-strong`, `fw-code`, `fw-strike`, `fw-live-link`, `fw-list-mark`. Fence marks, images, tables, math and diagrams are not touched yet.

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
