# Cycle134 — the Live editor spike (overnight run, 6 October 2026)

**Decision this spike serves:** Andrew wants *all* of iA-Writer-style source editing, real code editing, Typora-style
inline WYSIWYG, and the ability to edit "in the published look" — in one app, over one document. The right-hand
publishing pane cannot be made editable (rendered HTML/PDF has no mapping back to Markdown); the answer is a
**single CodeMirror 6 editor with presentation modes** (Source / Code / Live), hosted in the Chromium the app
already ships, styled by the same `#write` theme CSS, with the Markdown text as the only source of truth.

This spike answers, with numbers, whether that architecture holds inside Fomawrite:

| Question | Gate |
| --- | --- |
| Can the page and the C++ document stay byte-identical through user edits? | Cycle 131's round-trip fixtures edited in the page, saved by C++, bytes compared |
| What does a keystroke cost across the QWebChannel bridge? | keystroke → `documentChanged` → QTextDocument mirrored; report ms |
| Does a large document load acceptably? | 1 MiB `setDocument` time |
| Do undo and mode switches keep one truth? | undo in the page mirrors into C++; Source↔Live switch preserves text |
| Do existing Typora-style themes style the editor? | the selected theme's sanitized CSS applied to `#write` |

## Architecture (as built)

- `src/editor/` — TypeScript, CodeMirror 6, bundled by esbuild into the committed `dist/editor.js`; `index.html` is
  loaded from `qrc:/editor/index.html` under a strict CSP; `qwebchannel.js` comes from Qt's own resources.
- `src/editorbridge.{h,cpp}` — the `QWebChannel` object. Page → app: `ready`, `documentChanged(changesJson, rev)`,
  `cursorChanged`, `metric`, `log`, `textReply`. App → page: `setDocument`, `applyChanges`, `setMode`, `setTheme`,
  `setAppearance`, `focusEditor`, `requestText`, plus test-support `simulateUserChanges`/`undo`/`redo`. State pushed
  before the page connects is replayed on `ready()`.
- `Backend::applyLiveChanges` — change lists are `{from,to,insert}` in pre-change UTF-16 offsets, ascending; the
  whole list is validated, then applied in reverse inside one `QTextCursor` edit block, so a malformed list changes
  nothing. `Backend::syncLiveEditor` pushes theme (`Publisher::currentCss`), appearance and the document.
- `src/LiveEditorPane.qml` — the `WebEngineView` + `WebChannel`; `workspaceLayout.liveEditEnabled` selects it instead
  of the Source/Visual surface; command `liveEditing` ("Live") in the View menu; persisted in the workspace state.

## Measurements

Offscreen Qt 6.11.2, Apple Silicon, release build; a real `WebEngineView` driven over the bridge exactly as the app drives it.

| Measurement | Result |
| --- | --- |
| User edits in the page (3 changes in one transaction, incl. ZWJ emoji and combining marks) mirrored into the `QTextDocument` | **byte-identical**, both directions (`requestText` returns the same text) |
| Undo in the page | mirrored back; document returns to the original bytes |
| Source → Live → Source switches | one text, unchanged |
| Cycle 131 persistence fixtures (CRLF, BOM, NBSP, graphemes) edited in the page, saved by C++ | **bytes and conventions preserved** (CRLF stays CRLF, BOM stays) |
| Small document: inject → mirrored in C++ | 53 ms round trip (two channel hops + 10 ms test polling) |
| Page `setDocument` (small / 1 MiB) | 3.4 ms / 7.6 ms |
| 1 MiB document pushed over the channel and loaded | 54 ms wall |
| One keystroke on the 1 MiB document mirrored | 79 ms (two hops + polling; a real keystroke is one hop) |
| Live decoration rebuild (`decorate`) | 0.0–0.4 ms |

For comparison, Cycle 133's measurements of the existing Visual Edit pane: one keystroke on a 1 MiB document ≈ 58 ms after the O(n) fix (≈ 1.5 s before). The bridge is not the bottleneck.

## Verification

- Full offscreen regression suite at the end of the night: **FINAL_COUNT_PLACEHOLDER** (`./bin/test`, Qt 6.11.2, Apple Silicon).
- Live editor tests in [`tests/cycle134-live-editor.inc`](../../tests/cycle134-live-editor.inc): change lists apply byte-exactly and malformed ones are refused whole; the bridge replays state to a late page; images resolve through the host (valid, missing, remote); a real `WebEngineView` mirrors page edits byte-exactly, mirrors undo, keeps one text across mode switches, round-trips the Cycle 131 persistence fixtures through the page and a C++ save, loads a 1 MiB document in ~55 ms, follows **host-side** edits (a `QTextCursor` change reaches the page; a format-only change does not) and keeps offsets agreeing afterwards; a capture test renders Live (basic, Claude Like light/dark), Source (Manuscript, Editorial, Code) and a math/table document and saves PNGs under `research/cycle-134/captures/` — the frames are checked to be non-uniform, then reviewed by eye.
- Editor page tests (`cd src/editor && npm test`, node:test, DOM-free): change serialisation round-trips (incl. ZWJ emoji, combining marks, CJK), live decorations hide/reveal correctly, mode switches preserve text/revision, appearance JSON maps to classes/compartments, focus paragraph ranges, typewriter decisions, block builders (image/task/rule/fence/table/quote), table parsing (ragged rows, escaped pipes, alignments), math parsing (`$5 and $10` stays text, `\$` escapes, unclosed `$`), render caches. **JS_COUNT_PLACEHOLDER** tests.
- Test isolation fixes found along the way: the two recovery-baseline tests now wait for the draft's recovery write to land instead of assuming a fixed delay; every test process uses its own app-data directory (`FomawriteTests-<pid>`, removed in `cleanupTestCase`) so suites running concurrently (worktrees, CI) no longer see each other's recovery snapshots — the cause of two "flaky" failures tonight.
- Screenshots reviewed: Live mode under the Claude Like theme renders headings, inline marks, real checkboxes, the quote bar, the host-resolved image, a table **grid**, a rule and a labelled, highlighted Python fence; Source/Manuscript shows iA-Writer-style hanging `#`/`-`/`>` markers in the bundled mono face; Code shows line numbers, indent guides and horizontal scroll.

## Limits and what comes next

**Measured answers to the spike's questions:** the bridge is not a bottleneck (one keystroke on 1 MiB mirrors in ~75 ms including two hops and test polling; a real keystroke is one hop), the document stays byte-identical in both directions, existing Typora-style themes style the editor via `#write`, and the Chromium page costs nothing until Live is first used (it is then kept, hidden, so mode switches do not reload it).

**Not done tonight / known limits**
- **Mermaid diagrams** (would add ~2.5 MB) and **footnotes/TOC/front-matter** rendering: not started.
- **Bundle size**: `dist/editor.js` is 1.8 MB because every CodeMirror language grammar is bundled for fence highlighting; a curated language list would halve it. KaTeX adds 275 KB + 20 fonts.
- **Keyboard shortcuts and app commands while Live is focused**: the page handles its own editing keys (CodeMirror keymap incl. search, history); the app's Format commands apply to the Source editor and are refused while focus is outside it — with the host→page mirror they would now reach the page, but no Live-aware Format menu wiring exists yet.
- **Cursor carry-over** between Source and Live is not implemented (each mode starts at its own caret); scroll position is synced with the publishing pane in both directions.
- **Authorship marks** are format-only `QTextDocument` edits and are deliberately not pushed to the page; they are invisible in Live mode.
- The **old Visual Edit pane and bounded mapper remain** untouched; the decision to retire them is Andrew's.
- Offscreen only: no bundle refreshed, no native harness rerun (installed app in use). The footer "Live" button and the CI runner fixes were in flight when this record was written — see STATUS.
