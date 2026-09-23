# Remaining build plan — 23 September 2026

Cycle 33 themes is complete. Cycle 34a closes recovery-baseline and cancellable-Quit defects, but the remainder of Cycle 34 is still open. There are **9 planned cycles remaining (34–42)**, counting the unfinished part of 34. This is an estimate for the agreed personal-editor scope, not a guarantee of complete iA Writer parity. Larger features or failures can require another increment. The inventory and closeout audit remain the acceptance backlog.

| Cycle | Deliverable | Completion criteria |
|---|---|---|
| 33 (complete) | Themes | Persisted whole-workspace presets: light, dark and warm paper; editor, library, organizer, dialogs and preview share readable colors. Keep blue folders and gray document icons. Follow-system option, reset, keyboard focus/selection contrast and restart checks. |
| 34 | Recovery, autosave and versions | Multi-window crash/relaunch and quit-cancel matrix; external-write conflicts, unavailable disks and save failures; agree version/sidecar behavior. Close remaining session edge cases, including minimized/fullscreen and display changes. |
| 35 | Authorship workflows | Clipboard/export behavior, paste/edit/undo and recovery checks; clear provenance limitations and visualization. |
| 36 | Markdown and document navigation | Finish agreed footnote/content-block/link-resolution cases; nested relative assets, anchors and long-document scroll checks. |
| 37 | Output templates and portability | Persisted output styles, page breaks and pagination, headers/footers, portable assets; native picker cancellation and multipage PDF checks. |
| 38 | Library navigation | Organizer smart folders, browsable tags, nested code exclusions, bounded large-library behavior and performance. |
| 39 | Writing services | Spelling suggestions/correction, agreed grammar/substitution/speech support and useful live analysis with code exclusion. Validate supported languages. |
| 40 | Menu and interaction completion | Resolve reference-menu/settings/context-menu gaps, command coverage and enabled states; exact tooltip timing and file-operation edge cases. |
| 41 | Usability and accessibility | Keyboard and accessibility review; dark, narrow and external-display checks; cross-app clipboard and outstanding native workflow tests. |
| 42 | Release closeout | Fix final blockers, update user guide, reproducible bundles and GitHub release archive, final acceptance ledger. Public Developer ID signing/notarization is separately gated on credentials and distribution intent. |

## Theme boundaries

Themes are an Omawrite product addition, independent of iA menu parity. Initial scope is whole-workspace colors plus a matching preview, with typography controls retained. Theme preferences never rewrite Markdown or authorship sidecars. Print/export styles remain an explicit output choice so dark screen colors do not accidentally produce dark printed pages.

Start with bundled presets using shared color tokens, not scattered per-control colors. Keep future local theme import possible through a validated declarative format. Direct Typora CSS compatibility and arbitrary executable theme code are outside this initial cycle; this Qt Quick application does not share Typora's styling architecture.

## Counting rule

Close a cycle only when its agreed behavior, automated checks, native verification and known limits are recorded. A working subset is described as partial; an implemented menu item alone does not close the corresponding feature. Re-estimate after Cycle 37, when output complexity is clearer.

## Cycle 34b progress

Safe version restoration is implemented: unlabelled historical text, one-step Undo of text/authorship, explicit Save before autosave resumes, and persisted recovery pause. Failed Save As preservation is regression-tested. 61 tests pass. Cycle 34 remains open for automatic history, disk/device failures and native display/OS-shutdown acceptance; the remaining count stays nine.
