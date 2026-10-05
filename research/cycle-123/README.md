# Cycle 123 — Code appearance and Manuscript alignment

Build target **0.3.0-dev19 / macOS 0.3.0 (123)**.

Andrew requested a fourth writing appearance for code, headings whose Markdown markers hang beside the body column in Manuscript, and continuous backgrounds for fenced code. Code is available through the Source Aa menu, compact Writing appearance menu, native View → Writing Appearance menu and command palette. Selecting Code reveals Source while retaining the Single/Split choice.

Code uses Menlo at a 15px base with 135% line spacing, line numbers and guides every four columns. Long lines retain their shape with horizontal scrolling. Tab/Shift-Tab indent or outdent the current line or selected lines; the editor infers existing tab/space indentation. Return preserves leading indentation and adds a level after `{`, `[`, `(` or `:`. Code retains straight quotes, hyphens and plain pasted URLs even when prose smart punctuation is enabled. Appearance changes preserve the document's literal text and saved UTF-8 bytes.

Lightweight syntax colours cover JavaScript/TypeScript, Python, JSON, C/C++, shell, CSS and HTML, selected from a supported file suffix or Markdown fence language. Comments and strings retain their own colours across supported multiline forms; unknown languages remain literal. This is a bounded lexer: language-server integration, autocomplete, linting and debugging are outside this cycle.

Manuscript reserves a seven-character gutter in each block so Qt can display heading markers to the left without clipping. The heading's negative text indent aligns its words and wrapped lines with ordinary body text. Existing list/task/quote hanging indents share that body column. Fenced-code backgrounds extend across every block, including empty lines and closing fences, in all writing appearances. Inline code keeps a compact character background.

## Verification — PENDING

The planned gate is **189 regression cases** and **151 actual-executable checks per bundle**. These are expected counts, not completed results. Build/test outcomes, native screenshot review, embedded resource/version comparisons, strict signatures, refreshed runnable copies and verified source identity remain **PENDING**. Root will fill this section after the final integration gate.

Added focused checks cover real heading/wrap geometry, continuous fenced backgrounds, compact inline code, Code typography, exact saves, appearance cycles, language/string/comment highlighting and atomic indentation/Return Undo. The isolated native harness dispatches synthetic Qt events through the shipped window and captures `manuscript-hanging-headings-code-blocks.png`, `code-style-light.png` and `code-style-dark.png` across Single/Split layouts. Those captures and logs remain local and ignored.

Runnable targets after verification: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app` and `/Applications/Fomawrite.app`. Their refresh and identity checks are **PENDING**. No public release or notarization is included.

## References and limits

Reviewed official [VS Code basic-editing conventions](https://code.visualstudio.com/docs/editing/codebasics) for familiar indentation behaviour and [Qt QSyntaxHighlighter documentation](https://doc.qt.io/qt-6/qsyntaxhighlighter.html) for block formatting and state. No external code or new dependency was copied. Fomawrite's implementation remains in its existing QML/C++ editor.

As in Cycle122, typography refresh defers while Redo is pending, preserving text history until the next safe edit/reload. Syntax colours and indentation inference are conservative presentation/editing helpers. Physical keyboard/IME, VoiceOver, multi-display testing, the earlier native activation limitation and broader Markdown/parser/export acceptance remain open. [Optional review exercise](../usability/cycle-123.md).
