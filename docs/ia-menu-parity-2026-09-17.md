# iA Writer menu parity log — 17 September 2026

## Closeout implementation overlay — 21 September 2026

Cycles 20b–27 now have working subsets; final build and 48 tests pass. This does **not** close the baseline parity rows wholesale. See the [closeout audit](closeout-audit.md) for per-cycle implemented behavior, native evidence and unfinished acceptance criteria. In particular, basic output fonts are not full templates; manual annotations are not author-aware paste/provenance; saved queries are not full organizer smart folders; selection review is not complete live linguistic services. Older tables/overlays remain historical snapshots.


## Cycles 18a–20a implementation overlay — 21 September 2026

| IDs | Implemented local subset / remaining gap |
|---|---|
| M01–M04 | Heading/body, lists/tasks, blockquote, indentation and line moves with atomic undo. Nested Markdown/fence variants remain partial. |
| M05–M10 / M18 | Inline toggles, backtick-safe code, atomic link insertion, fenced code block and horizontal rule. Highlight remains future work; not every iA delimiter rule is verified. |
| M15–M16 / M20 / E13 | Date, basic table, Unicode case and surrounding inline style removal. Full Clear Styles is not implemented. |
| E03–E06 | Ordinary Markdown/HTML/formatted copy and plain/HTML-to-Markdown paste. MIME and native round-trip pass; cross-app interoperability and authorship paste remain pending. |
| G01–G05 | Separate session document/library histories, enclosing folder, basic source links and bounded recursive filename quick-open. Content indexing, persistent histories and advanced links remain future work. |

Final 39 tests pass; representative native commands, clipboard undo, quick-open/link navigation, cursor restoration and dirty-navigation Cancel verified. Both bundles refreshed. [Per-cycle records and limits](build-cycles.md). Next 20b; 15 listed increments remain plus QA. Historical tables below are baseline observations; this overlay takes precedence for current implementation status.


Source baseline: Omawrite commit `47d8e3c`, with the existing working-tree documentation changes preserved. This audit reads seven screenshots supplied by Andrew and the current source; it does not execute iA commands or retest the running Omawrite app. Codex remains the visual reference; iA is the functional reference.

## Evidence and status rules

Screenshots are in `screenshots/`, named `Screenshot 2026-09-17 at … AM.png`:

| Reference | Time | Menu |
|---|---|---|
| S1 | 10.14.18 | File |
| S2 | 10.14.24 | Edit |
| S3 | 10.14.31 | Format |
| S4 | 10.14.38 | View |
| S5 | 10.14.45 | Focus |
| S6 | 10.14.49 | Go |
| S7 | 10.14.55 | Window |

The supplied images contain surrounding personal content. This log transcribes commands only, not document names, location names, tags or connected-device names. No copies of the images have been added to research or staged for commit.

**Have**: corresponding basic command exists in source. **Elsewhere**: behavior exists, but not in the matching native menu. **Partial**: useful subset exists, with stated limitations. **Add**: no corresponding implementation found. **Inspect**: reference detail or native availability is unverified. “Have” is not a claim of exact iA semantics, shortcut parity or fresh native verification. Disabled entries still count as observed commands. Submenu arrows establish the parent command only; hidden children are not inferred from these images.

The cycle column refers to the proposed plan in [staged development plan](development-plan.md). Cycles 14–15 are implemented; Cycle 15 builds and passes 31 tests, with principal native file workflows verified and remaining checks explicitly listed. Cycle 16a is also implemented as described in its overlay; subsequent increments remain planned. The tables below retain the initial audit baseline; completion overlays record current progress.

## Cycle 14 completion overlay

Implemented in source and covered by automated checks; native sign-off remains pending:

| IDs | Current result |
|---|---|
| M06 / M08 | Native Format commands added; shared formatting undo fixed. General delimiter/toggle semantics remain Cycle 18 work. |
| V01–V03 / V05 | Existing pane/bar/sort/text-size functionality now exposed in View and synchronized with toolbar/library settings. |
| V04 | Current dates/excerpts exposed; uninspected tree/list/date-format options still partial. |
| V07 | Explicit Reload Preview forces Markdown reparse; local-image reload needs native checking. |
| V08 | Existing Editor/Split/Preview layouts exposed; synchronized scroll and paginated output remain future work. |
| V09 | Existing typeface choices exposed under Preview Typeface, not claimed as templates. |
| V13 | Native label follows enter/exit state; existing fullscreen shortcut retained. |
| C01 / C03 / C04 | Existing independent paragraph/typewriter controls exposed in Focus; master-switch semantics and sentence focus remain partial. |

Outline, statistics and Markdown markup visibility are also exposed in View. See [Cycle 14 evidence](../research/cycle-14/README.md). No full row is promoted to verified iA parity solely from automated menu tests.

## Cycle 17 implementation overlay

E01/E02/E09/E10: focused-field Undo/Redo/Cut/Copy/Paste/Delete/Select All states; Find submenu with Find, Replace, Next/Previous and selection search. Literal case-insensitive matching and single-step replacement undo tested; native menu/replacement/wrapping verified. 34 tests pass. Hidden reference options remain unverified; rich clipboard remains Cycle 19. [Evidence](../research/cycle-17/README.md).

## Cycle 16b implementation overlay

**F10 Move To — implemented local workflow; verification partial.** Select a folder, retain filename/dirty buffer/undo, copy saved bytes exclusively, verify destination/recheck source before removal, update recovery/watcher/organizer paths. Failed removal leaves both files and original active. 33 tests pass including picker acceptance and permission failure. Live picker navigation/collision/cancel pass; live successful move and separate-volume testing remain pending. Qt picker used on macOS because native Open stayed disabled. Relative assets are not relocated. [Evidence and limits](../research/cycle-16b/README.md).

## Cycle 16a completion overlay

- **F08 Duplicate — implemented local subset:** explicit sibling filename; copies current buffer including unsaved changes; leaves original active; refuses overwrite. Native collision/success and automated UTF-8/dirty-state checks pass.
- **F09 Rename — implemented local subset:** same-folder rename retains text/undo and updates active URL, watcher, recovery, recents and favorites. Native collision/cancel/dirty rename/save and automated Unicode/recovery/watch checks pass. Case-only rename on insensitive filesystems is refused; cross-window coordination and recovery relaunch remain unverified.
- **F10 Move To — see Cycle 16b overlay above.** No autosave/version/native-title integration claim.

Build and 32 tests pass. [Cycle 16a evidence](../research/cycle-16a/README.md). Audit-baseline rows below remain historical; this overlay records current support.

## Cycle 15 completion overlay

| IDs | Current result |
|---|---|
| F01–F03 | New/current-window and named library creation in current/new window implemented; existing New Window retained. Native current-window creation and unsaved Cancel verified; new-window launch remains unverified. |
| F05, G07–G08 | Shared File/Go recent menus and Go locations implemented; native recent opening/location selection verified. Recents are still a bounded list, not query-based smart folders. |
| F12 | Finder reveal implemented for existing saved files; native selection not confirmed after inspection stalled. |
| F13 | Explicit reveal clears filter, expands ancestors, switches outside-root location and scrolls to row. Automated nested/outside-root coverage and native filter-clear check passed. |
| F04/F06/F07 | Existing open/save/close retained; native sample Save passed. A picker attempt was inconclusive; no blanket native lifecycle sign-off. |

New Folder is also exposed in File. Build and 31 tests pass. No claim of version/autosave/duplicate/rename/move support. See [Cycle 15 evidence](../research/cycle-15/README.md).

## File — S1

| ID | iA command | Omawrite now | Work needed | Cycle |
|---|---|---|---|---|
| F01 | New in Library in Window | Partial: New Window opens a blank process; library creation is separate | Create in selected library and open in a new window; define save/cancel semantics | 15 |
| F02 | New in Library | Elsewhere: library New File dialog | Expose in File; verify destination and dirty-document switching | 15 |
| F03 | New | Partial: New Window only | Define untitled-document behavior distinctly from library creation | 15 |
| F04 | Open… | Have: native picker and guarded document switch | Retain; verify shortcut and cancellation | 15 |
| F05 | Open Recent submenu | Elsewhere: 20 deduplicated sidebar recents | Shared native submenu; missing-file handling and clear action | 15 |
| F06 | Close | Have: Close Window with unsaved prompt | Verify dirty/untitled/save-failure cases | 15 |
| F07 | Save | Have: explicit save | Retain; autosave is a separate lifecycle change | 15 / 25 |
| F08 | Duplicate | Add: Save As exists but changes active path | Independent duplicate operation, defined unsaved-buffer behavior | 16 |
| F09 | Rename… | Add | Safe rename; update active URL, watcher, recovery, recents and favorites | 16 |
| F10 | Move To… | Add | Safe move, collision/failure handling and path-state updates | 16 |
| F11 | Revert To submenu | Partial: recovery and external-change reload, no version browser | Inspect children; native versions and intentional revert workflow | 25 |
| F12 | Show in Finder | Add | Native reveal of saved current file | 15 |
| F13 | Show in Library | Partial: open automatically expands ancestor folders in current root | Explicit reveal, scroll/selection and filtered/outside-root behavior | 15 |
| F14 | Share submenu | Add | Inspect available targets; macOS share integration | 24 |
| F15 | Export… | Add | Inspect formats/options; explicit export preserving source | 23 |
| F16 | Print submenu | Partial: rendered Markdown print dialog | Inspect children; separate rendered/plain-text printing | 23 |
| F17 | Page Setup… | Add | Persistent page settings used by print/export | 23 |

Save As exists in Omawrite but is not visible in S1; retain it. Earlier observations include modifier-dependent commands such as Close All and Save As, but these screenshots do not establish their current behavior. Do not reassign Omawrite’s Shift-Command-S (Save As) to Duplicate without reviewing the complete shortcut map.

## Edit — S2

| ID | iA command | Omawrite now | Work needed | Cycle |
|---|---|---|---|---|
| E01 | Undo / Redo | Have | Menu enabled states; single-step undo for every new mutation | 17–19 |
| E02 | Cut / Copy / Paste / Select All | Have: Paste explicitly inserts plain text | Retain ordinary source operations | 17 |
| E03 | Copy Formatted | Add | Rich clipboard from selection, preserving source | 19 |
| E04 | Copy HTML | Add | HTML clipboard output with defined selection rendering | 19 |
| E05 | Copy Markdown | Partial: ordinary Copy copies Markdown source | Explicit command; inspect rich-to-Markdown expectations | 19 |
| E06 | Paste As submenu | Inspect / Add: plain paste only | Inspect children; separate ordinary conversion from authorship | 19 / 27 |
| E07 | Paste Edits From submenu | Add | Inspect authorship-aware merge behavior before design | 27 |
| E08 | Mark As submenu | Add | Authorship ranges and identities, not just menu labels | 27 |
| E09 | Delete | Elsewhere: editor deletion keys | Native action with selection and enabled-state rules | 17 |
| E10 | Find submenu | Partial: find, next/previous, replace/all UI; native menu only Find | Expose existing actions; inspect selection search/filter variants | 17 |
| E11 | Spelling and Grammar submenu | Add / Inspect native services | Inspect options and Qt/macOS integration | 26 |
| E12 | Substitutions submenu | Add | Inspect options; controllable Markdown-safe substitutions | 26 |
| E13 | Transformations submenu | Add | Inspect case actions; Unicode-aware selection transformations | 19 |
| E14 | Speech submenu | Add / Inspect native services | Native speech actions and selection behavior | 26 |
| E15 | AutoFill submenu | Inspect | Check OS-provided availability; no custom identity store | 26 |
| E16 | Start Dictation | Inspect | Check native input integration before calling absent | 26 |
| E17 | Emoji & Symbols | Inspect | Check system character picker and menu exposure | 26 |

## Format — S3

| ID | iA command | Omawrite now | Work needed | Cycle |
|---|---|---|---|---|
| M01 | Headings submenu | Partial: styling, rendering and outline; no insertion menu | Inspect children; implement heading/body block transforms | 18 |
| M02 | Lists submenu | Partial: smart Return continues basic list markers | Inspect ordered/unordered/task variants; insert, convert, complete tasks | 18 |
| M03 | Blockquote / Body | Partial: quotes render and continue on Return; no explicit conversion | Selection-aware block conversion and removal | 18 |
| M04 | Structure submenu | Inspect / Add | Inspect children; prior inventory suggests indent/outdent and line moves | 18 |
| M05 | Bold / Italic | Have: wrap selection | Verify toggle semantics, empty/reversed selections and undo | 18 |
| M06 | Strikethrough | Elsewhere: More menu | Expose native action and shortcut | 14 |
| M07 | Highlight | Add | Define portable syntax and source/preview behavior | 22 |
| M08 | Code | Elsewhere: Inline code in More menu | Expose native action; handle embedded backticks | 14 / 18 |
| M09 | Code Block | Partial: fenced blocks render; no insertion command | Selection-aware fenced-code insertion | 18 |
| M10 | Add Link | Have: Markdown link insertion, including clipboard URL | Retain; test escaping and cursor behavior | 18 |
| M11 | Add Wikilink | Add | Path resolution, disambiguation, navigation and rendering | 22 |
| M12 | Add Footnote | Add | Identifier allocation, reference/definition insertion and rendering | 22 |
| M13 | Add Content Block | Add | Inspect syntax, includes, relative paths and cycle detection | 22 |
| M14 | Add Hashtag | Add | Token rules, insertion, indexing and navigation | 21 |
| M15 | Add Date | Add | Inspect format; simple insertion with undo | 19 |
| M16 | Add Table | Partial: tables render; no table insertion/editing workflow | Insert basic table first; define editing scope separately | 19 |
| M17 | Add Table of Contents | Partial: outline panel exists, no document TOC | Stable anchors and generated TOC update behavior | 22 |
| M18 | Add Horizontal Rule | Partial: Markdown rendering only | Block insertion and undo | 18 |
| M19 | Add Page Break | Add | Syntax plus paginated print/PDF semantics | 23 |
| M20 | Clear Styles | Add | Remove supported formatting without corrupting code/links | 19 |

## View — S4

| ID | iA command | Omawrite now | Work needed | Cycle |
|---|---|---|---|---|
| V01 | Hide Library / Hide Organizer | Elsewhere: toolbar toggles | Shared native actions and checked/visible states | 14 |
| V02 | Hide Sort Bar / Hide Filter Bar | Elsewhere: library dropdown, persisted | Reuse current state in native menu | 14 |
| V03 | Sort Files By submenu | Elsewhere: name/modified/created/extension, direction, folders first | Expose existing choices; inspect hidden reference options | 14 |
| V04 | View Options submenu | Partial: dates and excerpts; expandable tree only | Expose current options; inspect tree/list/date/extension options | 14 / 21 |
| V05 | Text Size submenu | Elsewhere: larger/smaller/reset | Reuse existing bounds/defaults and shortcuts | 14 |
| V06 | Show Completions | Add | Inspect completion sources and acceptance behavior | 26 |
| V07 | Reload Preview | Partial: automatic refresh and internal refresh function | Expose reload; verify local-image refresh | 14 |
| V08 | Preview submenu | Partial: Editor/Split/Preview layouts | Expose layouts; inspect Web/PDF, fit and sync details | 14 / 22–23 |
| V09 | Template submenu | Partial: Sans/Serif/Mono typography only | Independent template system; inspect template options | 23 |
| V10 | Title Bar submenu | Partial: native unified title bar | Inspect visibility/fading options; preserve native controls | 24 |
| V11 | Toolbar submenu | Partial: fixed controls, statistics popup | Inspect modes/fading/statistics choices | 24 |
| V12 | Show Tab Bar / Show All Tabs | Add | Document ownership and native tab model first | 25 |
| V13 | Enter Full Screen | Have: toggle fullscreen | Shared state and native transition checks | 14 / 24 |

Statistics currently include words, characters, characters without whitespace and estimated reading time; sentences, speaking time, task and authorship counts from the earlier inventory are not implemented. Hidden toolbar choices require inspection before specifying exact parity.

## Focus — S5

| ID | iA command | Omawrite now | Work needed | Cycle |
|---|---|---|---|---|
| C01 | Enable Focus Mode | Partial: separate paragraph/typewriter switches | Shared menu/state model; inspect master-switch semantics | 14 / 20 |
| C02 | Sentence | Add | Sentence boundaries and focus scope | 20 |
| C03 | Paragraph | Elsewhere: writing options | Native action using current highlighter | 14 |
| C04 | Typewriter | Elsewhere: centered cursor scrolling | Native action using current scroll behavior | 14 |
| C05 | Show Syntax; Adjectives / Nouns / Adverbs / Verbs / Conjunctions | Add | Linguistic analysis; Markdown syntax display is a different feature | 26 |
| C06 | Enable Style Check; Fillers / Clichés / Redundancies / Custom | Add | Rules, exclusions, custom patterns and understandable results | 26 |
| C07 | Hide Authors; Human / AI / Reference | Add | Persistent provenance and truthful display controls | 27 |

## Go — S6

| ID | iA command | Omawrite now | Work needed | Cycle |
|---|---|---|---|---|
| G01 | Back / Forward | Add | Document history, cursor restoration and dirty-state cancellation | 20 |
| G02 | Back in Library / Forward in Library | Add | Separate folder navigation history | 20 |
| G03 | Enclosing Folder | Partial: selecting locations/tree folders | Explicit parent navigation with boundary rules | 20 |
| G04 | Open Link | Partial: preview hyperlinks; relative Markdown opens through guarded switch | Editor-cursor command, fragments and non-Markdown behavior | 20 / 22 |
| G05 | Quick Search | Partial: filter sees expanded folders only | Bounded recursive filename search first; inspect content-search scope | 20–21 |
| G06 | Command Palette | Add | Searchable shared action registry and keyboard operation | 20 |
| G07 | Locations entries / Add Location… | Elsewhere: saved local roots in organizer | Dynamic menu; cloud-synced folders remain normal local paths | 15 |
| G08 | Smart Folders: Recents | Partial: recent-file list, not query-based smart folder | Define recent query/order and menu access | 15 / 21 |
| G09 | New Smart Folder… | Add | Persist query definitions and evaluate against index | 21 |
| G10 | Hashtags entries | Add | Index tags, dynamic navigation and result views | 21 |

The tag list continues below the screenshot. Its individual personal values are data, not separate product features. A named cloud location does not establish a requirement for provider-specific cloud APIs.

## Window — S7

| ID | iA command | Omawrite now | Work needed | Cycle |
|---|---|---|---|---|
| W01 | Minimize / Zoom | Partial: normal native window controls; no authored Window menu | Expose native actions; inspect OS-injected behavior | 24 |
| W02 | Zoom All | Add | Coordinated window ownership, then native action | 25 |
| W03 | Fill / Center | Inspect | Use supported native window actions | 24 |
| W04 | Move & Resize submenu | Inspect | Inspect children and OS support | 24 |
| W05 | Full Screen Tile submenu | Inspect | Native integration; verify on current macOS | 24 |
| W06 | Remove Window from Set | Inspect: state-dependent OS behavior | Check native availability; do not emulate window sets | 24 |
| W07 | Move to [display] | Inspect: dynamic connected-display target | Native display routing, no hardcoded device name | 24 |
| W08 | Bring All to Front | Add / Inspect native integration | Coordinate all application windows | 25 |
| W09 | Show Previous Tab / Show Next Tab | Add | Native tab model and shortcut handling | 25 |
| W10 | Move Tab to New Window / Merge All Windows | Add | Document ownership, dirty-state and recovery-safe transfers | 25 |
| W11 | Open-window list | Add: separate process per window today | Coordinated document/window registry | 25 |

## Remaining reference capture work

No screenshots in this batch open the iA Writer application menu, Authors or Help. Keep the earlier [reference inventory](ia-writer-inventory.md) for these, with its older evidence date. None of the visible nested submenus are expanded. Before implementing the corresponding cycle, inspect those children using a sample document, including enabled/disabled states and modifier-key variants. Export formats, share targets, version behavior, completion behavior and settings are not established by a top-level label.

## Implementation evidence

- `src/Main.qml`: native menus at lines 381–427; More/writing options at 147–173; guarded switching at 184–211; selection/link/list helpers at 807–871; search/replace UI later in the file.
- `src/LibraryPane.qml`: persisted bars/date/excerpt settings and creation/sorting menus.
- `src/OrganizerPane.qml`, `src/filelibrary.h`, `src/filelibrary.cpp`: saved roots, favorites, recents, tree filtering, creation and limited ancestor reveal.
- `src/backend.cpp`: statistics at 222, rendered printing at 388, detached-process new window at 409; saving, recovery and file watching elsewhere in the same file.
- `src/PreviewPane.qml`: automatic refresh, Qt Markdown rendering and clicked-link signal.
- `src/windowchrome_mac.mm`: macOS window chrome; no complete document/tab controller.
- `tests/tst_omawrite.cpp`: existing source/preview, mutation, library and document regression coverage. Last cycle records 25 passing tests; no new run in this documentation-only audit.

## Change log

- 17 September: reviewed all seven new images; compared command families with source; recorded hidden reference details as unknown; added Go commands previously missing from the audit; created staged plan. No editor behavior changed.
