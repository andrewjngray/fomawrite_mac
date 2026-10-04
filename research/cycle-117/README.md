# Cycle 117 — pane controls, document identity and compact library rows

Date: 5 October 2026. Review target: **0.3.0-dev13 / macOS 0.3.0 (117)**.

Status: implemented. All **176 regressions** and **12 focused native checks** pass with no failures or skips; the native count includes setup/cleanup. Dev, ordinary/demo and Applications are refreshed to build117 and each passes its actual-executable gate, embedded-resource/version comparison and strict signature verification.

## Changes requested in Andrew’s review

- Source writing appearance belongs below Source; the Preview template belongs below Preview. Both remain separate settings. Their positions follow the actual pane divider.
- Visual Edit sits toward the centre of the rendered pane and clamps to the space between the template picker and view controls. It becomes a compact icon when needed. Only **Source / Split / Full** form the fixed group at the far right; Source remains the route back to Markdown.
- Preview and Full headers identify the actual document with an icon, bold filename and a separate **— Edited** marker for unsaved work. Narrow headers elide the filename while preserving the dirty-state indication.
- Library excerpts use compact rows: a document icon, actual filename including extension, short grey date on the same line and a muted two-line snippet underneath. Blue folder icons remain. The existing date and excerpt preferences still apply.
- Tree children receive consistent indentation from the real model depth. In current-folder List navigation, rows are siblings and stay aligned; the UI does not invent parent-child relationships beneath unrelated folders. Snippets remain bounded reads of saved files; filenames no longer come from extracted Markdown headings.

File opening, context menus, selection, pane modes, independent zoom, canonical Markdown and Undo/Redo remain part of the acceptance gate. This cycle does not change file contents, navigation semantics or export typography.

## Verification gate

- Full regression suite: **176 passed, 0 failed, 0 skipped**.
- Focused native suite: **12 passed, 0 failed, 0 skipped**, including setup/cleanup.
- Stable Dev, ordinary/demo and Applications: each passes **124 footer states, seven daily-writing workflows, five zoom workflows, six acceptance groups and three new pane-chrome groups**, with **zero QML warnings**.
- All three match the build117/dev13 source and pass `codesign --verify --deep --strict`. Source commit: `218954cea1974f1802aac813feb308e84fbcb37f`. [Verified build identities and gate results](verified-builds.json).

The affected checks cover pane-local menu ownership, narrow Split and divider movement, repeated Source/Split/Full/Visual Edit transitions, long filenames and dirty-header state, compact excerpts and genuine tree indentation, file opening and exact draft/Undo preservation.

The previous separate native-window fixture stopped after 79 assertions when macOS refused foreground activation. This cycle does not close that gap: native keyboard/full-screen/inactive/Drawer/tab-detach and print/share scenarios, physical input, IME, VoiceOver, display scaling and arbitrary restored workspaces remain acceptance work. Synthetic Qt events in an isolated app window are distinct from physical OS input.

## Review artifacts and next work

All three locations are refreshed to build117: `dist/Fomawrite Dev.app`, ordinary/demo `dist/Fomawrite.app` and `/Applications/Fomawrite.app`. All three executable gates, resource/version comparisons and strict signatures pass. Confirm version and path in About before reviewing. These remain local ad-hoc review builds, not notarized public releases.

Use the [short review exercise](../usability/cycle-117.md) to assess the footer, document identity and library changes. Next scope should follow that feedback; richer image/table/list editing and the historical native/parser/export gaps remain separate work.
