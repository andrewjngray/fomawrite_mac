# Cycle 54 usability (phases 54a–54b)

- [ ] View → Show Completions opens suggestions only on explicit request; ordinary typing and restart do not turn on automatic insertion.
- [ ] Type `lan` in the synthetic sample and accept a suggestion explicitly; verify one Undo restores the prefix.
- [ ] Dismiss a suggestion, paste text and use IME composition; source should remain unchanged until acceptance.
- [ ] Try code fences, inline code and a link destination; no completion should be offered there.
- [ ] Report whether the suggestion UI feels quiet enough for daily writing.
- [ ] Confirm Edit → Substitutions → Smart Quotes starts off, remains checked after relaunch once enabled, and can be disabled again.
- [ ] With Smart Quotes enabled, type `"hello"`; confirm only the typed quotes become curly and one Undo removes/restores the final insertion as expected.
- [ ] Paste the same text and type a quote during IME composition; pasted/composing input should not be transformed by Omawrite.
- [ ] Type straight quotes in inline/fenced/indented code, a bare URL, a link destination and a raw HTML tag; they should remain straight.
- [ ] Try a non-US keyboard layout and report any quote key that bypasses or unexpectedly triggers the substitution.
