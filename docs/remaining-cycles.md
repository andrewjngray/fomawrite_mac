# Remaining build plan — 23 September 2026

Five additional build checkpoints are delivered: 34c automatic previous versions, 35 authorship clipboard, 36 richer Markdown, 37 output templates/portability and 38 organizer queries/tags. Build and 66 tests pass, with representative native sample checks. **Four untouched planned cycles remain (39–42), plus acceptance gaps in 34–38.** Under the strict counting rule below, nine cycle areas still contain open work; it would be misleading to say only four fully verified cycles remain. The next feature area is Cycle 39. Full iA Writer parity is not claimed.

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


## Five-checkpoint follow-through — 23 September

| Area | Delivered | Remaining acceptance / limits |
|---|---|---|
| 34 | Previous-version-on-save opt-in, safe explicit-save restoration | Physical disks/devices, OS shutdown, fullscreen/minimized/display matrix; native Versions browser and historical sidecars outside delivered subset |
| 35 | Validated labelled clipboard, range selection, metadata export | Cross-app loss/stripping matrix, row-focus timing, full live attribution visualization |
| 36 | Multiline/repeated footnotes, bounded CSV/code/image blocks, nested relative assets | Long-document/anchor clicks, complex link/container syntax and nearest wiki resolution |
| 37 | Persisted styles, headers/footers, page breaks, portable HTML raster assets | Custom title-page/printer matrix, paginated/fit-page preview; native PDF Cancel passed |
| 38 | Sidebar queries, explicit asynchronous tag index, active-window shortcuts | Large-library performance at limits, incremental refresh/full container grammar |

Hardware-dependent checks should be grouped into Cycle 41's acceptance session. Deferred product capabilities still need explicit acceptance or scope decisions at Cycle 42; they are not silently counted as complete.
