# Cycle 50: File and Edit menu hierarchy

Synthetic sample: `sample/Menu-actions.md`.

File and Edit now use the captured top-level grouping while retaining Omawrite-only file actions in an explicit Extras group. Copy Formatted, Copy HTML and Copy Markdown require selected editor text; ordinary edit commands continue to target the focused editable field.

Verification: `./bin/build` and `./bin/test` passed with 81 tests. Native Dev inspection confirmed the File/Edit hierarchy, selected-text copy enablement, source preservation, and dirty File → Close → Cancel. The disposable QA draft was discarded through the normal prompt before a normal quit.

The visible screenshots do not capture the children of several iA submenus. This cycle does not infer them. Native inspection showed AppKit-injected AutoFill, Dictation and Emoji entries; Omawrite's duplicate Emoji item was removed. Cross-application clipboard, actual share/print jobs, and dark/narrow/keyboard/VoiceOver QA remain open.

## Phase 50b — captured Transformations

A later [native iA capture](../menu-audit-2026-09-24/native-submenus.md) established the four Transformations labels. An [isolated iA sample](ia-capitalize-reference.txt) showed Capitalize changing `tEST of THE wORLD` to `Test Of The World`; Undo restored the original. Omawrite now has the four captured labels in order and a Capitalize action with that observed plain-text result. It retains the selected range, refuses code/link-destination selections through the existing safe path, and is reversible in one Undo.

`./bin/build` and the full native-access `./bin/test` pass with **102 tests, zero failures and zero skips**. The focused regression covers order/labels, Unicode text, selection, Undo and protected-link refusal. The [refreshed Dev check](native-capitalize.txt) verified the menu, result and Undo in a disposable draft; the draft was discarded and clean Second.md restored. The ordinary app was repackaged and reopened a clean saved document. Both bundles passed strict signature verification. Logs: [build](build-50b.log) and [tests](test-50b.log). No screenshot file was persisted.

iA Make Title Case produced a different result on the same sample; that gap was addressed in Phase 50c below. Locale/punctuation nuances, native protected-context checks, accessibility and the broader File/Edit state matrix remain open. Optional usability exercise: select `tEST of THE wORLD` in a disposable draft, Capitalize, Undo, then compare Make Title Case and tell us how you expect small words to read.

## Phase 50c — distinct Make Title Case

The [second iA sample](ia-title-case-reference.txt) and earlier sample establish an observed English subset distinct from Capitalize. The implementation and focused regression preserve selection, one-step Undo and protected-link refusal. Full `./bin/build` and native-access `./bin/test` pass **102/0/0**. [Native Dev verification](native-title-case.txt) reproduced the second sample and Undo in a disposable draft, then discarded the draft and returned to clean Second.md. The ordinary app was repackaged and reopened clean README.md; both app signatures verified. Logs: [build](build-50c.log), [tests](test-50c.log). No screenshot was persisted. Unobserved punctuation, locale and style rules remain open. Optional exercise: try the second sample in a disposable draft, Undo, and tell us how you want minor words and acronyms treated.
