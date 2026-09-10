# iA Writer reference inventory

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
| Dates and text excerpts in file list | Planned | 5 |
| Separate organizer, file list, editor, preview | Have; organizer collapses below 1,000 px | 4 |
| Folder/file creation | Have: within selected root | 1 |
| Back/forward library navigation | Planned | 5 |
| Filter library | Partial: expanded folders only | 5 |
| Swipe to reveal panes | Planned | Later |
| Synchronized editor/preview scrolling | Planned | 7 |
| Auto-resize when showing library | Planned | Later |
| Hide sort/filter bars | Planned | 5 |
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

The menu opened but its accessibility tree exposed no commands in the current state. Main toolbar visibly provides back/forward library navigation. Go-menu contents need another check with a suitable document/location state; no commands are inferred here.

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
