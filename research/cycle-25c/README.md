# Cycle 25c — Opt-in autosave and saved versions

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Optional one-minute autosave for saved documents with disk-content comparison; pause on external changes. Explicit native NSFileVersion checkpoint of saved bytes under file coordination. Restore into editor as one undo step; disk changes only on Save.

## Tests and native verification

Autosave external-change protection tested. Native checkpoint creation, listing, restore and undo back to prior dirty sample passed.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

No automatic version capture per save or native Versions-browser UI. Versions cover Markdown, not authorship sidecars. Autosave has a remaining external-writer race between comparison and write; disk-full/disconnection/crash stress remains. Default is off.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Create a saved-file version; edit a sample; restore; undo before deciding whether to Save.
