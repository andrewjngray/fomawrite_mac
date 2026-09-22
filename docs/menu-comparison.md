# Menu comparison and incremental plan

**17 September update:** the iA/Omawrite comparison and future build sequence are now in [the screenshot parity log](ia-menu-parity-2026-09-17.md) and [staged plan](development-plan.md). This earlier three-app draft remains historical; Typora inspection is still incomplete.

Working draft, 12 September 2026. **Capture audit is incomplete.** iA File was inspected today; other iA entries come from the native audit on 10 September. Typora's individual commands await permission to inspect its open window. Omawrite is verified against current source, not a freshly captured Dev menu. Do not treat blank/pending cells as missing features in Typora.

Status: **Have** means the behavior exists; **Partial** means narrower support; **Missing menu** means behavior exists elsewhere in the app; **Not implemented** means it needs development. “Proposed” is a recommendation for Andrew to accept or change.

## Top-level organization

| Menu purpose | iA Writer | Typora (names supplied by Andrew) | Omawrite now | Proposed Omawrite destination |
|---|---|---|---|---|
| App preferences/lifecycle | iA Writer | Typora app menu: inspect | Native app menu, basic lifecycle | Omawrite |
| Documents and filesystem | File | File | File, plus library dropdown | File |
| Editing and search | Edit | Edit | Edit, search toolbar | Edit |
| Block structure | Format | Paragraph | Limited commands outside menu | Paragraph |
| Inline formatting/insertion | Format | Format | Format has Bold/Italic/Link | Format |
| Provenance | Authors | Inspect | Not implemented | Defer Authors |
| Workspace/preview | View | View | View only fullscreen; toolbar has much more | View |
| Appearance/templates | View | Themes | Writing options, basic preview styles | View → Appearance initially |
| Writing concentration | Focus | Inspect View | Paragraph focus/typewriter outside menu | Focus |
| Navigation | Go | Inspect | Outline, library and recents outside menu | Go, after useful commands exist |
| Native window management | Window | Window | New/Close in File, fullscreen in View | Window using native behavior |
| Documentation | Help | Help | Keyboard Shortcuts | Help |

Proposed eventual menu bar: **Omawrite · File · Edit · Paragraph · Format · View · Focus · Go · Window · Help**. Add a menu only when it has working commands. Keep Authors and a standalone Themes menu out of the first passes.

## Capability decisions

Typora column intentionally remains pending inspection for every row; this is a decision framework, not a completed three-way parity claim.

| Group / capability | iA Writer observed | Typora | Omawrite behavior / menu coverage | Proposed action |
|---|---|---|---|---|
| File: new document/window | Three New variants | Pending | Have: New Window; New File in library dropdown | Add New File to File; keep clear window distinction |
| File: new folder | Library menu | Pending | Have in library dropdown | Add File → New Folder |
| File: open/save/save as | Present | Pending | Have in File | Retain, add visible shortcut labels |
| File: recent documents | Open Recent | Pending | Have sidebar Recents; missing menu | Add Open Recent submenu |
| File: duplicate | Duplicate | Pending | Not implemented | Separate filesystem cycle |
| File: rename/move | Rename, Move To | Pending | Not implemented | Implement with dirty-state/overwrite protection |
| File: versions | Revert To → Browse All Versions | Pending | Recovery exists, native version browser does not | Defer; distinguish recovery from versions |
| File: reveal | Show in Finder/Library | Pending | No File-menu commands | Add Finder reveal and library reveal separately |
| File: export/share | Export, system Share | Pending | No dedicated export/share workflow | Later dedicated cycle |
| File: print | Print, Print Plain Text, Page Setup | Pending | Partial: Print | Retain; inspect output before expanding |
| File: close all | Close/Close All | Pending | Close Window; multiwindow close-all not implemented | Defer coordinated close-all |
| Edit: undo/clipboard | Core plus formatted/HTML/Markdown copying | Pending | Have core; plain-text paste, no rich clipboard modes | Retain core; rich copy separate |
| Edit: find/replace | Find submenu with next/previous/selection/replace | Pending | Partial: find/replace UI; menu has Find only | Expose existing behavior first |
| Edit: transformations | Case transformations | Pending | Not implemented | Later small text-mutation cycle |
| Edit: spelling/speech | macOS writing services | Pending | Not implemented as complete native integration | Investigate native APIs separately |
| Paragraph: headings | Levels 1–6 | Pending | Preview renders headings; no comprehensive insertion menu | Add block commands with undo tests |
| Paragraph: lists/tasks | Ordered/unordered/task variants, complete task | Pending | Partial list editing/rendering; no complete menu | Add commands, distinguish editing from rendering |
| Paragraph: quote/code/rule | Blockquote, fenced code, horizontal rule | Pending | Rendering exists; insertion menu incomplete | Add block transformations |
| Paragraph: line operations | Indent/outdent, move line up/down | Pending | Not a full menu workflow | Review existing keyboard behavior before adding |
| Format: inline styles | Bold, italic, strike, highlight, code | Pending | Bold/italic/link in menu; strike/code in More | Expose strike/code; defer unsupported highlight syntax |
| Format: links/media | Link/image/wiki/footnote/content blocks | Pending | Partial: link insertion; Markdown rendering | Add image/footnote only with tested source behavior |
| Format: tables/TOC | Insertion commands | Pending | Tables render; no table editor/TOC insertion workflow | Separate cycle |
| View: panes/layout | Library/organizer/preview layouts | Pending | Have through toolbar; missing native menu | First pass: expose existing controls |
| View: list display | Sort/filter bars, ordering, dates/excerpts | Pending | Have through library menus | Reuse the same state/actions in View |
| View: text size | Increase/decrease/reset | Pending | Have writing size controls | Expose existing controls |
| View: templates/PDF | Templates, Web/PDF, fit page/width | Pending | Partial basic Qt preview; no equivalent paginated templates | Defer preview renderer expansion |
| View: statistics | Many counts and display modes | Pending | Partial statistics popup | Expose current popup; expand separately |
| Focus: paragraph/typewriter | Present | Pending | Have in writing options | Add Focus menu using existing state |
| Focus: sentence | Present | Pending | Not implemented | Later independent cycle |
| Focus: syntax/style analysis | Parts of speech, style checks | Pending | Not implemented; Markdown syntax display is different | Defer |
| Go: navigation | Prior Go inspection inconclusive; toolbar back/forward | Pending | Outline/library navigation; no history | Inspect reference then build navigation history |
| Window: native layout/tabs | Window placement and tabs | Pending | Basic windows/fullscreen; no tab model | Native window actions first; tabs separate |
| Help: documentation | Search/help/news/support | Pending | Keyboard shortcuts only | Add local guide and release notes |
| Authors: provenance | Authorship setup | Pending | Not implemented | Defer until core editor mature |

## One menu at a time

| Pass | Menu | Scope | Evidence needed before calling it complete |
|---|---|---|---|
| 1 (recommended) | View | Expose existing library/organizer, sort/filter bars, Editor/Split/Preview, outline, statistics and writing-size controls | Checked states synchronize with toolbar; keyboard and narrow-window checks |
| 2 | File | New File/New Folder/Open Recent, consistent shortcuts; evaluate reveal commands | Unsaved prompts, cancelled dialogs, real file operations and recovery |
| 3 | Edit | Find/Replace/Next/Previous and clipboard clarity | Selection preservation, Unicode, undo/redo and search cases |
| 4 | Paragraph | Headings, lists, quote, fenced code and rule | Source round trip, multiline selections and single-step undo |
| 5 | Format | Expose strike/inline code; add chosen insertion commands | Selection/cursor behavior and undo |
| 6 | Focus | Existing paragraph/typewriter controls; sentence focus only if selected | Scroll/focus behavior in long documents |
| 7 | Go | Outline and document navigation; history as a distinct feature | Back/forward semantics across unsaved documents |
| 8 | Window | Native minimize/zoom/fullscreen/window list | Multiple windows, close cancellation and fullscreen transitions |
| 9 | Help | Guide, shortcuts and build notes | Links work offline where intended |

Why View first: most of its useful behavior already exists. It improves discoverability and lets us establish consistent menu styling/checkmarks without starting with risky file mutations. File can go first if Andrew prefers document management.

## Review notes for Andrew

For each row, choose **Keep**, **Add soon**, **Later**, or **Skip**. Before implementing a pass, finish the matching reference screenshots, select exact labels/shortcuts and agree on behavior. No new feature implementation has started in this audit.

Evidence folder: [menu audit](../research/menu-audit-2026-09-12/README.md). Previous iA detail: [inventory](ia-writer-inventory.md).
