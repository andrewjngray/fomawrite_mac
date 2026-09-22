# Cycle 27 — Manual authorship annotations

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Explicit Human/AI/Reference labels with optional author; Unknown clears labels. QTextDocument formatting ranges participate in undo and edits. Hidden .<filename>.omawrite-authors.json sidecar stores ranges with SHA-256 source match; external text changes invalidate them. Recovery snapshots include annotations.

## Tests and native verification

Native selected Reference label and Save passed. Tests cover mark undo/redo, sidecar save/reopen and external-edit invalidation.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Annotations are assertions, not verified provenance. Inserted text can inherit nearby labels. Clipboard/export, Duplicate/Move/Rename sidecar migration, paste-edits/author-aware merging and full attribution visualization remain. Native versions do not version sidecars.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Select sample words, mark Reference, Save; keep the hidden sidecar with the Markdown file.
