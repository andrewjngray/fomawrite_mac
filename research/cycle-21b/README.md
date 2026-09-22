# Cycle 21b — Saved queries and tags

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Persist up to 30 root-scoped queries; remove/reopen them in Quick Open. #tag queries match case-folded tags outside basic fences/inline/indented code. Open content searches refresh every five seconds. Hashtag insertion added.

## Tests and native verification

Saved-query persistence and code exclusion tests pass. Native #sample search and Save query passed; persisted query survived restart.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Saved queries are in Quick Open, not organizer smart-folder rows. Nested Markdown code cases and a browsable tag index remain; these are partial smart-folder semantics.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Search #sample with content enabled, save the query, and reopen it.
