# Cycle 40 — command access

Expanded palette routes file/path, save, rename/move, export, theme, annotation and writing-review commands, with state guards. Palette dispatch waits for closure. Per-control hover timers gate path tooltips at two seconds, avoiding shared tooltip warm-start. Annotation selection waits for dialog closure. Paginated preview uses the same output painter as PDF/print and exposes fit controls.

Final combined 70-test suite and native palette → spelling/preview checks recorded under cycle-41/42. Exact rapid-hover timing, context-menu/submenu completeness and full annotation-row focus matrix remain unverified; no label-only parity claim.
