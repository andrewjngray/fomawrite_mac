# Cycle 109 — split simple lists in Visual Edit

Date: 4 October 2026. Integrated review target: **0.3.0-dev10 / macOS 0.3.0 (111)**.

## Scope and changes

Return can split the body of a supported, single-line list item at the caret. The insertion keeps the existing bullet or numbered marker style, indentation, spacing and line endings. A new task item starts unchecked; an ordered item gets the next marker without renumbering the following source. The caret moves to the second half so typing can continue there.

The mapping validates the projected result before changing canonical Markdown. Splits through inline formatting, link or code contents, nested/continued list structures and Unicode graphemes are refused and retain the Source fallback. A related typography correction skips redundant block-format writes that otherwise discarded remaining Redo history.

## Verification

Exact-source marker/Unicode/CRLF cases, safe/refused split boundaries, actual Return, follow-on typing and atomic Undo/Redo all pass. The full integrated suite passes **159 tests**; focused native writing/footer workflows pass **11 including setup/cleanup**. All three refreshed build111 bundles pass124 footer states and seven daily-writing workflows with matching embedded UI and version identity. See the [combined Cycle111 handoff](../cycle-111/README.md) for logs, captures, the diagnostic rendering correction and exact artifacts.

## Limits and review

This remains bounded list editing. Shift-Return within lists, nested/multiline list restructuring, automatic renumbering of following source items and arbitrary multiline paste are not implemented. Public distribution is unchanged. [Optional exercise](../usability/cycle-109.md).
