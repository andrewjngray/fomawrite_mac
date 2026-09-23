# Menu closeout plan — 23 September 2026

Starting point: Cycle 47, commit `0617158`, 76 passing tests, with the [screenshot audit](ia-menu-audit-2026-09-23.md) as the command inventory. This plan starts at Cycle 48. It aims for the observed iA Writer menu structure and useful, verified behavior, while retaining Omawrite's local Markdown workflow and its additional commands. Matching a label is not enough to close a row.

## Definition of done

For each observed iA command, the [audit](ia-menu-audit-2026-09-23.md) must identify its current menu path, working behavior, enabled/checked state, shortcut where captured, and a verification result. A row closes when the behavior works on sample local files, preserves source and undo where relevant, and the native menu is correctly assembled. An item whose children were never captured remains **unverified** until new evidence exists. Platform-owned items may be marked satisfied by an AppKit action after native inspection; we will not draw duplicate custom controls just to match a screenshot.

Each cycle is separately buildable and reviewable: update `docs/build-cycles.md`, `docs/ia-writer-inventory.md` and the audit; keep synthetic samples, logs and safe screenshots in `research/cycle-NN/`; update `research/usability/`; run `./bin/build` and `./bin/test`; close the Dev app normally, prepare `dist/Omawrite Dev.app`, and check the affected native workflow. Record gaps and an optional short exercise. The ordinary app is refreshed at the end of a stable cycle, without changing the Dev bundle identity. Never commit `build/`, `build-tests/`, `dist/` or private screenshot content.

## Cycle sequence

| Cycle | Focus | Completion evidence | Depends on |
|---|---|---|---|
| 48 | **View date choices:** Show Date → Date Modified, Date Created, None in the native menu and all library menus. Migrate the old on/off preference; keep sorting independent. | Distinct sample creation/modification dates display correctly, only one choice is checked, choice survives restart, None hides dates, and source stays clean. | Captured View submenu |
| 49 | **Format structure:** align observed order and labels, move Body and Page Break to their captured positions, add Ordered Task List and a deliberately scoped Clear Styles implementation. | Each mutation has expected UTF-8 Markdown and one-step Undo; menu screenshot matches captured groups; no unsupported command is labeled complete. | 48 build baseline |
| 50 | **File and Edit menus:** align top-level order and captured command grouping; put Close, Copy formats, Find, Share/Export/Print in appropriate paths while retaining Omawrite safety controls. Consolidate shared action state where it prevents drift. | Saved/untitled/dirty/selection states and shortcuts verified; file operations preserve unsaved prompts, recovery, paths and source. Hidden submenu children are added only after capture. | 49; missing submenu evidence for exact children |
| 51 | **View navigation and preview:** Tree/List presentation with equal file reachability; View → Preview → Full/Split/Web/PDF routes to existing continuous/paginated output; keep accurate template names and document limitations. | Selection, keyboard and folder navigation survive mode change/restart; PDF fit/page commands route correctly; narrow/dark sample QA. | 48; 50 shared actions |
| 52 | **View chrome and statistics:** title/toolbar display modes, Default/Stats Only, sentence/speaking/task/author counts with explicit counting rules. | Counts match fixture and update after edits/annotations; hidden chrome remains keyboard accessible; fullscreen, restart, high contrast and VoiceOver checks. | 51 |
| 53 | **Go and organizer:** dynamic Locations, Smart Folders/Recents and Hashtags menu groups; create/edit/delete saved queries, plus bounded incremental refresh for changed local files. | Rename/move/delete/external edits update queries/tags; inaccessible roots and watcher overflow fall back safely; no stale results presented as current. | 50 shared actions |
| 54 | **Writing input:** completions, opt-in smart punctuation/text substitutions and controlled automatic correction based on captured Edit children. | Typing, paste, IME and Markdown code/URL cases; each automatic edit is reversible in one Undo; disabled setting never mutates source. | Edit submenu capture; 50 |
| 55 | **Focus and review:** one master focus state; live per-class syntax display and Fillers/Clichés/Redundancies/Custom checks, built on the existing macOS word-class review. | Highlight layers do not change source, cursor, selection, undo or recovery; toggles update while typing within performance bounds. | 54 input pipeline |
| 56 | **Authors:** dedicated setup and author identities; Mark As, Paste Edits From, Human/AI/Reference visibility, and truthful assertion-only metadata semantics. | Annotate/edit/undo/copy/save/reopen/version restore matrix; category hiding never changes Markdown; missing/untrusted sidecars clearly indicated; cross-app clipboard tests. | Authors/Edit submenu capture; 55 decoration model |
| 57 | **Native Window, application and Help:** inspect AppKit-injected menus, then expose supported Zoom All/Fill/Center/tiling/display/window list actions and standard app/Help destinations. | Two-window/tab state, fullscreen/display/minimized restoration, native menu enabled states, keyboard and VoiceOver checks on the packaged app. Unsupported OS actions documented. | 50 shared actions; current macOS inspection |
| 58 | **Markdown/output depth:** close reproduced gaps in Clear Styles, links/fragments, wikilinks, content blocks, TOC, complex tables, PDF/HTML and cross-app clipboard. | Byte-for-byte Markdown/Undo fixtures; multi-page rendered output inspected; failures documented with exact inputs. | 49, 51, 56 |
| 59 | **Integrated acceptance:** repeat the full menu matrix with sample saved/untitled/dirty windows, keyboard navigation, narrow/dark/VoiceOver and available hardware. Update guides and release ledger. | Every audit row has evidence or a specific residual; build/tests pass and native screenshots/logs correspond to current code. | 48–58 |

The current plan has **12 cycles, 48–59**. Cycle 48 is complete; Cycle 49 source and tests are integrated but its final native gate is pending, so **11 planned cycles remain**, including 49. The count is provisional: newly captured submenus can reveal additional work. Public notarization and developer signing remain a separate distribution decision because they require credentials and another-machine acceptance.

## Evidence needed before exact submenu work

The current screenshots do not show File → Revert/Share/Print children; Edit → Paste As/Paste Edits From/Mark As/Find/Spelling/Substitutions/Transformations/Speech/AutoFill; View → Preview → PDF; the Authors, application and Help menus; or the dynamic Window document list. Capture them using a sample document and record state-dependent disabled items, menu order and shortcuts. This can happen while Cycle 48 proceeds, but an uncaptured child is not invented as part of a later cycle.

## Parallel work within one sequential cycle

The planning agent sets scope and acceptance before implementation. Lower-cost agents can take bounded source changes, test fixtures, reference inspection or documentation review that do not edit the same files. The lead agent integrates changes, resolves conflicting evidence, runs the full build/test/native gate, and updates the cycle ledger. Start the next cycle only after the current cycle's behavior and remaining gaps are recorded.

## Risks that shape the order

- Menu layout work must share action state with toolbar, context menu and command palette; otherwise checked and disabled values drift.
- Live review, completions and authorship overlays touch editor ranges. Keep decorations separate from Markdown content and test UTF-16 offsets, cursor/selection, undo and recovery.
- Library watching needs bounded fallback when a volume disappears, files move or a watcher overflows.
- AppKit's Window and application menus vary by macOS version and Qt ownership. Inspect the packaged app before adding duplicates.
- Manual authorship labels are user assertions, not verified provenance; cross-app transfer can lose them.
- Physical disk failure, external displays, printing and another-machine installation cannot be inferred from unit tests.
