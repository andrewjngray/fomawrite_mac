# Cycle 21a — Bounded saved-content search

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Recursive filename/content search off the UI thread; a per-query in-memory content index is rebuilt from saved files. Limits: 100 results, 20,000 entries, 256 KiB per file, 32 MiB total; unreadable/oversized files reported. Hidden/symlink/build/dependency folders excluded.

## Tests and native verification

Automated saved edit/rename/delete and stale-query checks pass. Live exact-tag content search found the sample.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Not a persistent index: rereads bounded files. Unsaved buffers are excluded. Tree/list/date options not confirmed beyond existing controls; native large-library stress remains.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Quick Open → Search saved file contents too; search a phrase absent from filenames.
