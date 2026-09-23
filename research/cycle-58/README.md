# Cycle 58a: local Markdown fragments (partial)

Omawrite now follows heading fragments in explicit local `.md`, `.markdown`, `.mdown`, `.txt` and `.text` links, `[[wikilinks#fragments]]` and same-file links. The source editor, Go → Open Link and rendered preview share the guarded route. Duplicate heading slugs use suffixes such as `#same-1`, and the fragment is stripped from the opened file’s identity.

Cross-file navigation applies its caret jump only after a successful open and queues the preview jump until the target Markdown has parsed. Same-file navigation does not reload the document. Dirty Cancel and failed opens retain the original document, caret and scroll. The source and Undo history are not changed by navigation.

`./bin/build` and the full native-access `./bin/test` suite pass with **98 tests, zero failures and zero skips**. The stable `dist/Omawrite Dev.app` was refreshed through 58a. Native QA opened [`sample/Source.md`](sample/Source.md) by path, placed the caret in its inline link and used Go → Open Link. [`sample/Target.md`](sample/Target.md) opened with clean status and no `#` fragment in the window title. Precise caret placement and preview scrolling are covered by automation only; native visual scrolling and screenshot capture were unavailable. See [`native-fragments.txt`](native-fragments.txt).

Fragment transfer to a target already open in another window is not implemented. A missing heading opens the valid document without a jump or a dedicated error. Dark/narrow layout, keyboard-only traversal and VoiceOver remain unchecked. This is bounded local navigation support, not complete Markdown parser or output parity. [Optional usability exercise](../usability/cycle-58.md).
