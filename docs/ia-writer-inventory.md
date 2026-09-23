# iA Writer reference inventory

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


**Updated reference, 17 September 2026:** Andrew supplied seven menu screenshots. The [command-level parity log](ia-menu-parity-2026-09-17.md) records current implementation status and new Go-menu evidence. The [staged development plan](development-plan.md) supersedes speculative future cycle numbers below; historical inspection notes remain for context.

Inspected in the running Mac app with computer use on 10 September 2026. All ten main menus were opened, including the nested items exposed by accessibility. All nine Settings categories and all three Markdown settings tabs were inspected. No writing, account, or preference changes were made. This records interface behavior, not proprietary implementation. Personal filenames, account details and document contents are intentionally excluded.

Status: **Have** = implemented in Omawrite; **Partial** = some behavior exists; **Planned** = not implemented. Menu availability can depend on selection and document state. This is not a claim that every command was executed.

## Library and layout

| iA Writer capability | Omawrite status | Next cycle |
|---|---|---|
| Organizer with multiple locations | Have: local folder shortcuts | 4 |
| Favorites | Have: files and folders | 4 |
| Recents | Have: 20 recent opened/saved documents | 4 |
| Smart folders | Planned | 6 |
| Hashtag organization | Planned | 6 |
| Tree/list navigation | Partial: expandable tree | 5 |
| Sort by name, modified, created, extension | Have | 4 |
| Ascending/descending; folders pinned first | Have | 4 |
| Dates and text excerpts in file list | Partial: modified dates and bounded source snippets with visibility toggles; compact default and top-level Previews button; no date-format options | 9 |
| Separate organizer, file list, editor, preview | Have; organizer collapses below 1,000 px | 4 |
| Folder/file creation | Have: within selected root | 1 |
| Back/forward library navigation | Planned | 5 |
| Filter library | Partial: expanded folders only | 5 |
| Swipe to reveal panes | Planned | Later |
| Synchronized editor/preview scrolling | Planned | 7 |
| Auto-resize when showing library | Planned | Later |
| Hide sort/filter bars | Have: persistent library dropdown actions; native View-menu exposure pending | 10; proposed 14 |
| File extensions preference | Planned | 5 |

## Main menus observed

### iA Writer

About; Check for Updates; Settings; Services; Hide/Hide Others/Show All; Quit/Quit and Close All Windows. Omawrite has basic app lifecycle/menu integration; update delivery and coordinated multi-window quitting remain separate work.

### File

New in Library in Window, New in Library, New; Open; Open Recent; Close/Close All; Save/Save As; Duplicate; Rename; Move To; Revert To → Browse All Versions; Show in Finder/Library; system Share; Export; Print/Print Plain Text; Page Setup.

Omawrite has open/save/save-as/new-window/new-library-file, recovery and print. Recents arrive in cycle 4. Duplicate/rename/move, native version browsing, sharing, export presets and full multi-document lifecycle remain planned. iA's Files settings use automatic saving and optional keep/revert-on-close; Omawrite currently uses explicit Save plus recovery snapshots. Changing this needs its own data-safety cycle.

### Edit

Undo/redo; cut/copy; Copy Formatted/HTML/Markdown; paste; Paste As/Paste Edits From/Mark As (authorship setup); delete/select all. Find submenu: find, next/previous, use selection, replace/replace all, filter library. Spelling/grammar checking, automatic corrections; smart copy/paste/quotes/dashes/text replacement; upper/lower/capitalize/title-case transformations; speech; macOS AutoFill, dictation, emoji/symbols.

Omawrite has core editing, plain-text paste and find/replace. Rich clipboard, transformations and platform writing services remain planned (cycles 8–9). System services should use macOS APIs where appropriate.

### Format

Heading levels 1–6; unordered/ordered lists and task variants; complete task; blockquote/body; indent/outdent, move line up/down; bold/italic/strikethrough/highlight; inline/fenced code; link/image/wikilink/footnote/content block/hashtag/date; table/TOC/horizontal rule/page break; clear styles.

Omawrite has bold/italic/link shortcuts and basic list editing. Preview displays common Markdown structures. Rendering a table is distinct from having a table editing command. The rest belongs in cycle 8, with undo and round-trip tests.

### Authors

Set Up Authorship was the available menu command. Settings support authors with names/identifiers and Human/AI/Reference categories. No authorship tracking exists in Omawrite; plan separately after the core writing workflow. Do not implement a visual switch that implies provenance is tracked when it is not.

### View

Show/hide library, organizer, sort bar, filter bar; sorting and tree/list/excerpt/date options; text size; completions; preview show/reload; full/split, Web/PDF and fit page/width; templates Modern Sans, Classic Serif, Manuscript Mono/Duo/Quattro, GitHub; title/toolbar visibility or fading; stats-only toolbar; characters, no-space characters, words, sentences, reading/speaking time, tasks and authorship counts; tabs/all tabs; fullscreen.

Omawrite has pane visibility/layouts, writing size, simple Sans/Serif/Mono preview and some statistics. These are not iA's HTML/PDF templates. Template/pagination work is cycle 7; remaining stats and toolbar behavior are later polish. Native tabs require multi-window architecture work.

### Focus

Sentence/paragraph/typewriter focus; syntax categories adjectives, nouns, adverbs, verbs, conjunctions; style checking for fillers, clichés, redundancies and custom patterns; show/hide Human/AI/Reference authors.

Omawrite has paragraph focus and typewriter scrolling. Sentence focus, linguistic syntax highlighting, style checking and provenance are planned separately. “Show Markdown syntax” in Omawrite means visible markup, not iA's parts-of-speech coloring.

### Go

The 10 September accessibility inspection was inconclusive. Andrew’s 17 September screenshot now confirms Back/Forward, Back/Forward in Library, Enclosing Folder, Open Link, Quick Search, Command Palette, Locations/Add Location, Recents/New Smart Folder and Hashtags. Omawrite has local locations/recents in its organizer and preview link activation; history, command palette, recursive search, smart folders and hashtags remain unimplemented. See the dated parity log for exact limitations.

### Window

Minimize/zoom/fill/center; move/resize halves and quarters; fullscreen tiling; move to display; bring/arrange windows; previous/next tab, move tab to window, merge windows, open-window list. Most are native macOS window services. Omawrite has a basic window/fullscreen implementation; do not build a second custom window manager.

### Help

Help search, iA Writer Help, What's New, online support. Omawrite needs its own feature guide, release notes and usability entry point.

## Settings details

- **General:** system/light/dark appearance; title/toolbar fading; library auto-resize and swipe; synchronized preview scroll; extensions; URL commands/Shortcuts.
- **Publishing:** publishing account management. No provider was connected or added; supported providers not inferred.
- **Files:** preferred New vs New in Library action; default plain-text extension; automatic save and optional keep/revert prompt; reopen behavior.
- **Library:** organizer sections; sort/filter bars; folder pinning; excerpts; sorting; tree/list.
- **Editor:** typeface, text size, typography, line-length limit, indentation; sentence focus scope; syntax/style categories and custom patterns; smart substitutions and spelling; predictive text; highlight colors; fade/strike completed tasks; matching-bracket insertion/selection wrapping.
- **Authors:** identities and author categories; no data changed.
- **Templates:** custom templates; inverted web preview colors; PDF/print heading numbering/centering, paragraph indentation, title page, headers/footers.
- **Markdown:** metadata; preferred wikilink/content-block paths; output handling for wikilinks/hashtags/autolinks; single-return paragraphs; smart punctuation.
- **Account:** account management screen exists; private account details are not recorded.

## Boundaries of this inspection

Context menus, every popup option, export formats, publishing provider choices, and commands requiring content modification or account setup were not exhaustively exercised. Future cycles should inspect the relevant controls using our sample documents before implementation. The complete product remains a multi-cycle target, not a one-build parity claim.

### Cycle 6 visual refinement

Implemented original line icons for workspace controls and file rows, with Andrew's requested navy-blue active states. These are visual affordances for existing actions; no additional iA menu functionality or feature parity is implied. Dark appearance still needs a manual visual audit.

### Cycle 7 visual alignment

Unified macOS title/toolbar area, wider navigation, balanced source/preview, smaller type, file snippets, bottom preview controls, and persistent status are implemented. Qt preview template details still differ from iA (tables, code/quote blocks and heading separators). This is closer layout alignment, not full UI or feature parity.

### Cycle 8 sorting menu
Rounded compact dropdown with Date Modified/Date Created/Name/Extension, exclusive A to Z/Z to A, folder pinning and persistent Show Date/Show Text Excerpts. Navigation and date-format submenus remain future work.

### Cycle 10 menu update

Library dropdown now includes New File/New Folder, Sort By and View Options submenus, and persistent hide/show actions for sort and filter bars. View Options currently supports dates and excerpts only; other iA navigation and date-format options remain deferred.

### Cycle 11 visual update

Original filled blue folder/white document icons, blue outline location markers, macOS system interface font and larger header labels. Native outer corners are unchanged; this does not claim exact asset or window-shape parity.

### Cycle 12 organizer update

Favorites and Recents have persistent collapsible sections; favorite actions are left-aligned. macOS uses a native unified title-bar style. Exact traffic-light geometry/corner matching remains unverified while the sharing badge covers the controls.

### Cycle 13 reference change

Codex is now the visual reference for navigation icons, font treatment and spacing. This inventory remains the functional backlog; prior iA visual styling is no longer the target.

### Cycle 14 menu implementation

View now exposes existing pane/bar/sort/text-size/layout/preview-typeface controls plus reload, markup, outline and statistics. Focus exposes paragraph/typewriter; Format includes strike/inline code. A shared command registry keeps native and in-window controls aligned. Wrapping selected text now has single-step undo in C++. Build and 28 automated tests pass; native verification is pending safe Dev refresh. Templates, sentence/linguistic focus and hidden reference submenu details remain partial/future work.

### Cycle 15 file-access update

File now exposes New/current-window, New Window, library creation in current/new window, New Folder, shared Open Recent, Finder/library reveal. Go exposes locations and recents. New/create keep the unsaved guard; failed replacements retain recovery. Build/31 tests pass. Native current-window creation, recents, location switching, Cancel/Save and filtered reveal verified; Finder selection and new-window/folder/dialog completion remain unchecked. See [Cycle 15 records](../research/cycle-15/README.md).

### Cycle 15b folder color feedback

Andrew retains the current outline shapes but requests blue folders and gray documents. Locations, folder favorites, library rows and folder header now use blue folder outlines; neutral selections remain. Light native appearance verified with a sample; dark appearance still needs review. This is visual polish, not a functional parity change.


## Cycle 16a update — 20 September 2026

Local Duplicate and Rename are implemented through File. Duplicate copies the current buffer to a new sibling without changing the active document; Rename retains dirty text and undo while updating the path, recents/favorites, watcher and recovery. Collision errors and cancellation were checked natively; 32 tests pass, including Unicode, recovery metadata and external-change detection. [Evidence and limits](../research/cycle-16a/README.md). Move To remains future work (16b); untitled/symlink, case-only rename and cross-window/native lifecycle parity remain limited. Matching labels alone do not establish full iA behavior.


## Cycle 16b update — 20 September 2026

File → Move To is implemented for saved regular local files. It copies saved disk contents, verifies/rechecks before source removal, retains unsaved editor text/undo, and updates path, recovery, watcher, recents/favorites. Collision and cancellation were checked in the live app; successful picker acceptance, state preservation and removal-failure behavior pass in the 33-test suite. macOS uses Qt's folder picker after native Open remained disabled. Successful live move, separate-volume, crash/disconnection and cross-window coordination remain unverified; this is partial parity evidence. [Cycle 16b records](../research/cycle-16b/README.md).


## Cycle 17 update — 21 September 2026

E01/E02/E09/E10 local editing/search subset implemented: focused-field editing states/Delete, native Find/Replace/Next/Previous/selection search. Single/all replacements have atomic undo; literal case-insensitive search preserves original Unicode offsets. 34 tests pass and native sample replacement/undo/wrapping verified. Full hidden iA search submenu parity and advanced search options remain unclaimed. [Evidence](../research/cycle-17/README.md).


### Cycle 29 implementation update

Manual authorship annotations now follow Duplicate, Rename and Move, with collision guards and dirty-buffer/recovery preservation. 51 automated tests pass; native sample operations and resulting sidecar hashes were checked. This supersedes the earlier “no authorship tracking” baseline only for explicit manual annotations. It is not automatic provenance, author-aware paste/merge, or full iA Writer parity. See [Cycle 29 evidence](../research/cycle-29/README.md).

### Cycle 30 user-requested addition

Open by Path accepts a local Markdown/text file or folder path. Native menu/keyboard and unsaved-change checks pass; 53 tests total. This is an Omawrite workflow addition, not a claim of complete reference-app parity.

### Cycle 31 polish

Library filter now uses a fixed placeholder and bounded rounded field at narrow widths. 54 tests pass; native focused appearance checked. No change to filter matching semantics.

## Cycle 32 / themes follow-through

Saved-file windows now restore bounds, library roots and cursor positions; macOS tab groups/order and active document are restored. Repeated launches of the same installed executable forward paths and focus an already-open canonical path. Dev and ordinary bundles remain separate owners. Full lifecycle stress remains partial: fullscreen/minimized/display states, exhaustive multi-dirty quit cancellation and crash/disk failure cases still need coverage.

Themes are a new Andrew-requested product concept, planned for Cycle 33: shared workspace/preview presets and persisted selection. They are not implemented yet and are not evidence of iA theme parity. See [remaining cycle plan](remaining-cycles.md).

Cycle 33 implements Andrew's themes addition (not an iA parity claim): Follow system, Light, Dark and Warm paper; persisted, synchronized across windows and matching preview. Bundled presets only; output styles independent. Native/system accessibility audit remains.
