# Cycle 26a — Native spelling subset

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Selection spelling review through NSSpellChecker and Emoji command. Native platform Edit menu also exposes system Dictation/AutoFill where available.

## Tests and native verification

Native selection spelling correctly reported the sample typo “quikc”. Build/QML tests pass. Emoji/dictation remain unverified.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Inline spelling/grammar, automatic substitutions/corrections, speech and completion integration remain. Selection spelling reports words only; no suggestions or replacement workflow.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Select disposable prose and run Check Selection Spelling.
