# Cycle 54a: explicit local word completions (native QA pending)

The captured View menu shows **Show Completions**, but the Edit → Substitutions and AutoFill submenu children were not captured. This phase adds only an explicit completion request, not automatic correction or a claim of iA completion parity.

View → Show Completions uses Unicode words in the current document. It excludes code spans/blocks, indented code and URL/link destinations. Nothing is inserted until a suggestion is accepted; acceptance rechecks the caret and unchanged prefix, then makes a single atomic replacement for Undo. IME preedit is excluded. Work is bounded to the first 50,000 UTF-16 units and 256 matching occurrences, with at most 12 choices; later words may be omitted.

`./bin/build` and the full native-access `./bin/test` suite pass with **89 tests, zero failures and zero skips**. Focused tests cover the candidate rules, stale-caret refusal, Undo and a long-document time bound. The refreshed Dev app displayed two suggestions for `lan`; Return accepted one and Command-Z restored the prefix in a disposable temporary file. [Native log](native-menu.txt). Popup visuals, arrow traversal, Escape, dark/narrow appearance, IME and accessibility remain unverified because screenshot capture was unavailable. Use only [the synthetic sample](sample/Completion.md) for further QA.

The remaining Cycle 54 scope needs screenshot evidence for Edit submenu children and a native behavior decision before substitutions/correction are implemented. Dictation/AutoFill are AppKit-provided and remain OS-dependent.
