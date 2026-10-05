# Cycle 122 — literal, readable Source

Build 0.3.0-dev18 / macOS 0.3.0 (122).

Source consistently shows literal Markdown, including emphasis, code and link markers. Legacy hidden-marker preferences no longer produce a hybrid Source view. Visual Edit remains the formatted editing choice. Manuscript uses the bundled iA Writer Mono font at an 18px base with 155% line spacing; user zoom and other appearance choices remain available. Markers have clearer contrast and code has a subtle background. Wrapped lists, tasks and quotations align beneath their text, retaining original spaces and exact saved Markdown.

**185 regressions passed, 0 failed, 0 skipped**, including setup/cleanup. Dev, demo and Applications each pass **150 native executable checks**, including literal Source visibility, real prefix typing, app Undo/Redo and repeated editing/layout changes. All three copies match the current UI/version, have valid strict signatures and report zero QML warnings. [Verified build identities](verified-builds.json). Local logs and synthetic screenshots remain ignored. No public release or notarization.

The app’s Undo/Redo routes keep typography transparent while preserving intentional authorship formatting and branches after Undo. Tests also cover exact UTF-8 saves, clean state, selection, nested lists/code and actual wrapped-line geometry.

Verified source commit: `a8f2c7902c011d670dae127a19bf28ef2d0124be`.

Known boundary: changing font or zoom while Redo is pending defers the pixel-based hanging-indent refresh until the next safe edit/reload, to preserve Redo. Source indentation is a conservative presentation helper, not a new Markdown parser. Existing physical input, IME, VoiceOver, multi-display and broader parser/export limitations remain open.
