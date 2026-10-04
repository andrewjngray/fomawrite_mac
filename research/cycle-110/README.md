# Cycle 110 — searchable document outline

Date: 4 October 2026. Integrated review target: **0.3.0-dev10 / macOS 0.3.0 (111)**.

## Scope and changes

Document Outline now has a search field, result count, explicit heading levels and relative indentation. A marker identifies the section containing the Source caret when that is the active writing context. A rendered-only context does not falsely mark the stale Source caret as the current section.

Search matches case-insensitive words in heading titles. Up/Down chooses a heading; Home/End in the list goes to its first/last result. Enter activates the exact Markdown heading offset after the drawer closes. Escape or Close restores writing focus without changing the selection. Empty documents and searches without results have distinct messages. Reopening resets the filter.

Heading activation uses the shared view route. From narrow Full Visual, Source becomes visible before focusing its caret; wide Full can show Source in Split. Search controls follow the same 34px field and toolbar treatment as Find.

## Verification

Unicode filtering, fenced/YAML exclusion, keyboard selection/cancellation, current-section markers and exact narrow Visual→Source jumps all pass. The revision baseline is captured after the prior view typography settles. The full integrated suite passes **159 tests**; focused native writing/footer workflows pass **11 including setup/cleanup**. All three refreshed build111 bundles pass124 footer states and seven daily-writing workflows with matching embedded UI and version identity. See the [combined Cycle111 handoff](../cycle-111/README.md) for logs, captures, the diagnostic rendering correction and exact artifacts.

## Limits and review

The outline is a heading navigator, not a document restructuring tool. It uses the existing Markdown heading parser and does not add drag reordering or arbitrary parser parity. Physical VoiceOver and display-scale acceptance remain open. [Optional exercise](../usability/cycle-110.md).
