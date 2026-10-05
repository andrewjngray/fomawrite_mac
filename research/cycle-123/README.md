# Cycle 123 — Code appearance and Manuscript alignment

Build **0.3.0-dev19 / macOS 0.3.0 (123)**.

Andrew requested a fourth writing appearance for code, headings whose Markdown markers hang beside the body column in Manuscript, and continuous backgrounds for fenced code. Code is available through the Source Aa menu, compact Writing appearance menu, native View → Writing Appearance menu and command palette. Selecting Code reveals Source while retaining the Single/Split choice.

Code uses Menlo at a 15px base with 135% line spacing, line numbers and guides every four columns. Long lines retain their shape with horizontal scrolling. Tab/Shift-Tab indent or outdent the current line or selected lines; the editor infers existing tab/space indentation. Return preserves leading indentation and adds a level after `{`, `[`, `(` or `:`. Code retains straight quotes, hyphens and plain pasted URLs even when prose smart punctuation is enabled. Appearance changes preserve the document's literal text and saved UTF-8 bytes.

Lightweight syntax colours cover JavaScript/TypeScript, Python, JSON, C/C++, shell, CSS and HTML, selected from a supported file suffix or Markdown fence language. Comments and strings retain their own colours across supported multiline forms; unknown languages remain literal. This is a bounded lexer: language-server integration, autocomplete, linting and debugging are outside this cycle.

Manuscript reserves a seven-character gutter in each block so Qt can display heading markers to the left without clipping. The heading's negative text indent aligns its words and wrapped lines with ordinary body text. Existing list/task/quote hanging indents share that body column. Fenced-code backgrounds extend across every block, including empty lines and closing fences, in all writing appearances. Inline code keeps a compact character background.

## Verification

**189 regressions passed, 0 failed, 0 skipped.** Dev, demo and Applications each pass **151 actual-executable checks** with matching embedded UI/version, valid strict signatures and zero recorded QML warnings. Native captures were visually reviewed in light/dark and Single/Split, including the full grey code-block panel. [Verified build identities](verified-builds.json).

Added focused checks cover real heading/wrap geometry, continuous fenced backgrounds, compact inline code, Code typography, exact saves, appearance cycles, language/string/comment highlighting and atomic indentation/Return Undo. The isolated native harness dispatches synthetic Qt events through the shipped window and captures `manuscript-hanging-headings-code-blocks.png`, `manuscript-continuous-code-background.png`, `code-style-light.png` and `code-style-dark.png` across Single/Split layouts. Those captures and logs remain local and ignored.

Verified runnable copies: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app` and `/Applications/Fomawrite.app`. All three copies are refreshed to build 123. No public release or notarization is included.

## References and limits

Reviewed official [VS Code basic-editing conventions](https://code.visualstudio.com/docs/editing/codebasics) for familiar indentation behaviour and [Qt QSyntaxHighlighter documentation](https://doc.qt.io/qt-6/qsyntaxhighlighter.html) for block formatting and state. No external code or new dependency was copied. Fomawrite's implementation remains in its existing QML/C++ editor.

As in Cycle122, typography refresh defers while Redo is pending, preserving text history until the next safe edit/reload. Syntax colours and indentation inference are conservative presentation/editing helpers. Physical keyboard/IME, VoiceOver, multi-display testing, the earlier native activation limitation and broader Markdown/parser/export acceptance remain open. [Optional review exercise](../usability/cycle-123.md).

Verified source commit: `0a2a2d040a195214f390c86ee161307d525706bf`. Existing check harnesses now invoke the app Undo/Redo route introduced in Cycle122, and appearance checks preserve either a clean or already-dirty state. Qt Quick does not paint QTextBlockFormat backgrounds, so the visible source shading uses a viewport-aligned QML layer backed by block geometry.
