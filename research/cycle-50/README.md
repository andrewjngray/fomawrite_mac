# Cycle 50: File and Edit menu hierarchy

Synthetic sample: `sample/Menu-actions.md`.

File and Edit now use the captured top-level grouping while retaining Omawrite-only file actions in an explicit Extras group. Copy Formatted, Copy HTML and Copy Markdown require selected editor text; ordinary edit commands continue to target the focused editable field.

Verification: `./bin/build` and `./bin/test` passed with 81 tests. Native Dev inspection confirmed the File/Edit hierarchy, selected-text copy enablement, source preservation, and dirty File → Close → Cancel. The disposable QA draft was discarded through the normal prompt before a normal quit.

The visible screenshots do not capture the children of several iA submenus. This cycle does not infer them. Native inspection showed AppKit-injected AutoFill, Dictation and Emoji entries; Omawrite's duplicate Emoji item was removed. Cross-application clipboard, actual share/print jobs, and dark/narrow/keyboard/VoiceOver QA remain open.
