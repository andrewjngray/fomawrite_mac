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

VERIFICATION_PLACEHOLDER

## Limits and what comes next

LIMITS_PLACEHOLDER
