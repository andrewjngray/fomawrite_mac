# iA Writer menu map and Omawrite gap audit — 23 September 2026

This is the planning baseline after Omawrite Cycle 47 (76 tests), updated through Cycle 54a source and partial native checks on 24 September. Cycles 48–51 are complete. It transcribes the menu screenshots in `screenshots/` and compares them with the source. It is an interface and behavior audit, not a claim that similarly named commands behave identically.

The screenshots contain personal filenames, locations and tags. This document records commands only. The images remain untracked reference material and should not be committed. Two files are empty duplicate captures: `3.48.34 PM(2)` and `3.49.37 PM(2)`. The duplicate `1.57.39 PM(2)` and `3.50.00 PM(2)` images add no distinct menu evidence.

## Status language

- **Match** — Omawrite has a close working equivalent in the current native menu or the named surface.
- **Partial** — Omawrite has useful behavior, but the scope, placement or semantics differ materially.
- **Gap** — no corresponding working behavior was found.
- **Omawrite+** — useful Omawrite behavior with no visible equivalent in these iA screenshots.
- **Unverified** — a label is visible, but its submenu or behavior was not captured or exercised.

Menu availability in iA depends on selection and document state. Disabled commands are still part of the observed menu. macOS supplies much of the Window menu; it should be integrated through AppKit rather than recreated as a custom window manager.

## Screenshot evidence

| Capture | Evidence |
|---|---|
| 17 Sep 10.14.18–10.14.55 | File, Edit, Format, View, Focus, Go and Window top-level menus |
| 23 Sep 15.46.46 | File → Open Recent |
| 23 Sep 15.48.17 | Edit top-level menu, confirming the earlier capture |
| 23 Sep 15.48.28 | Format → Headings |
| 23 Sep 15.48.34 | Format → Lists |
| 23 Sep 15.48.40 | Format → Structure |
| 23 Sep 15.48.51–15.49.55 | View submenus: sorting, options, text size, preview, template, title bar and toolbar statistics |
| 23 Sep 15.50.00 | Focus top-level menu |
| 23 Sep 15.50.04 | Go top-level menu |
| 23 Sep 15.50.11–15.50.17 | Window → Move & Resize and Full Screen Tile |

No image opens the iA Writer application menu, Authors menu, Help menu, Edit submenus, File Revert/Share/Print submenus, View → Preview → PDF, or the Help search field. Those details remain unverified by this folder. The older 10 September inspection found **Set Up Authorship** under Authors and standard Help destinations, but that is older evidence.

## iA Writer menu tree from the screenshots

### File

- New in Library in Window; New in Library; New
- Open…; Open Recent → recent documents, Clear Menu
- Close
- Save; Duplicate; Rename…; Move To…; Revert To → *(children not captured)*
- Show in Finder; Show in Library
- Share → *(targets not captured)*
- Export…
- Print → *(children not captured)*
- Page Setup…

### Edit

- Undo; Redo
- Cut; Copy; Copy Formatted; Copy HTML; Copy Markdown; Paste
- Paste As → *(children not captured)*
- Paste Edits From → *(children not captured)*
- Mark As → *(children not captured)*
- Delete; Select All
- Find → *(children not captured)*
- Spelling and Grammar → *(children not captured)*
- Substitutions → *(children not captured)*
- Transformations → *(children not captured)*
- Speech → *(children not captured)*
- AutoFill → *(children not captured)*
- Start Dictation; Emoji & Symbols

### Format

- Headings → Heading 1, Heading 2, Heading 3, Heading 4, Heading 5, Heading 6
- Lists → List, Task List, Ordered List, Ordered Task List; Mark Task as Completed
- Blockquote; Body
- Structure → Indent, Outdent, Move Line Up, Move Line Down
- Bold; Italic; Strikethrough; Highlight
- Code; Code Block
- Add Link; Add Wikilink; Add Footnote; Add Content Block; Add Hashtag
- Add Date; Add Table; Add Table of Contents
- Add Horizontal Rule; Add Page Break
- Clear Styles

### Authors

- Not opened in this screenshot set. The older inspection saw **Set Up Authorship**. Author identities and Human/AI/Reference categories were observed in Settings, but the current submenu must be recaptured before exact parity work.

### View

- Hide Library; Hide Organizer
- Hide Sort Bar; Hide Filter Bar
- Sort Files By → Date Modified, Date Created, Name, Extension; A to Z, Z to A; Pin Folders to Top
- View Options → Show Date → Date Modified, Date Created, None; Show Text Excerpts; Navigation → Tree, List
- Text Size → Make Text Bigger, Make Text Normal Size, Make Text Smaller
- Show Completions
- Reload Preview
- Preview → Full, Split; Web; PDF → *(children not captured; older inspection indicates fit modes)*
- Template → Modern (Sans), Classic (Serif), Manuscript (Mono), Manuscript (Duo), Manuscript (Quattro), GitHub, Helvetica, MLA Style, Palatino
- Title Bar → Fade In/Out, Always Show
- Toolbar → Fade In/Out, Always Show, Hide; Default; Stats Only → Characters, Characters Without Spaces, Words, Sentences, Reading Time, Speaking Time, Tasks, Human, AI, Reference
- Show Tab Bar; Show All Tabs
- Enter Full Screen

### Focus

- Enable Focus Mode → Sentence, Paragraph, Typewriter
- Show Syntax → Adjectives, Nouns, Adverbs, Verbs, Conjunctions
- Enable Style Check → Fillers, Clichés, Redundancies, Custom
- Hide Authors → Human, AI, Reference

The dash/check marks in this capture are state indicators, not separate commands.

### Go

- Back; Forward
- Back in Library; Forward in Library; Enclosing Folder
- Open Link
- Quick Search; Command Palette
- Locations → dynamic locations; Add Location…
- Smart Folders → Recents; New Smart Folder…
- Hashtags → dynamic tag list

Personal location and hashtag names are deliberately omitted.

### Window

- Minimize; Zoom; Zoom All; Fill; Center
- Move & Resize → halves: Left, Right, Top, Bottom; quarters: Top Left, Top Right, Bottom Left, Bottom Right; arrange: Left & Right, Right & Left, Top & Bottom, Bottom & Top, Quarters; Return to Previous Size
- Full Screen Tile → Left of Screen, Right of Screen
- Move to *display*; Bring All to Front; Remove Window from Set
- Show Previous Tab; Show Next Tab; Move Tab to New Window; Merge All Windows
- Dynamic open-window list

### iA Writer application menu and Help

These menus were not opened in this folder. The older inspection recorded the standard About, Check for Updates, Settings, Services, Hide and Quit family in the application menu, and Help search, iA Writer Help, What's New and online support in Help. Treat those labels as older evidence until recaptured.

## Current parity map

### File

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| New / New in Library / in Window | **Match** | Current-window/new-window behavior exists with guarded unsaved changes. |
| Open / Open Recent / Close / Save | **Match** | Recents are bounded local history with clear action. |
| Duplicate / Rename / Move To | **Match** | Separate-volume Move To and physical failure cases still need hardware acceptance; relative assets are not relocated. |
| Revert To / versions | **Partial** | Omawrite can create automatic/manual macOS versions and restore one into the editor with one-step Undo. It does not expose the native Browse All Versions interface, and authorship sidecars are not versioned. |
| Show in Finder / Show in Library | **Match** | Implemented for the active saved file. |
| Share | **Partial** | File → Share Markdown sends current editor text; the library context action shares a saved file. Exact iA targets and options were not captured. |
| Export | **Partial** | HTML and PDF plus portable raster HTML exist. iA format/options parity and broad complex-document export acceptance remain open. |
| Print / Page Setup | **Match** | Rendered printing, Markdown-source printing, paginated preview and persistent page setup exist; physical printer acceptance remains open. |

### Edit

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Undo/Redo; Cut/Copy/Paste/Delete/Select All | **Match** | Core native-menu actions track the focused field. |
| Copy Formatted / HTML / Markdown | **Partial (Cycle 50)** | Commands now appear directly in Edit and enable for an editor selection. Cross-application clipboard acceptance remains open. |
| Paste As | **Partial** | Plain text and Markdown-from-HTML exist. The iA submenu was not captured, so exact conversions are unknown. |
| Paste Edits From | **Gap** | No authorship-aware merge/paste workflow. |
| Mark As | **Partial** | Manual Human/AI/Reference/Unknown range annotations exist in a dialog, with a sidecar and metadata export. They are assertions, not verified provenance, and are not exposed as iA-style menu commands. |
| Find | **Partial** | Find, replace, next/previous and use selection exist. Library filtering/search is elsewhere; exact iA submenu parity is unverified. |
| Spelling and Grammar | **Partial** | Explicit macOS dictionary/grammar review with suggestions, Replace and Undo exists. No inline-as-you-type spelling/grammar UI. |
| Substitutions | **Gap** | No smart quotes/dashes, text replacements or automatic-correction controls. |
| Transformations | **Partial (Cycle 50)** | Uppercase, lowercase and title case now appear under Edit → Transformations. Capitalize and exact native submenu parity are missing. |
| Speech | **Match** | Speak Selection and Stop Speaking use macOS speech; audible quality is not acceptance-tested. |
| AutoFill / Dictation | **Partial (Cycle 50)** | AppKit injects AutoFill and Start Dictation into Edit. Native availability depends on macOS; Omawrite does not provide a separate workflow. |
| Emoji & Symbols | **Match** | Opens the macOS character palette. |

### Format

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Headings 1–6 and Body | **Match (Cycle 49)** | Body now follows the Headings submenu at top level. Native menu order and Markdown behavior were checked. |
| Lists and task completion | **Partial (Cycle 49)** | List, Task List, Ordered List, Ordered Task List and completion now share the captured submenu structure; numbered checkbox creation and completion work with Undo. Deep nested-list behavior remains partial. |
| Blockquote; Indent/Outdent; line movement | **Match** | Selection-aware operations use atomic undo. |
| Bold, Italic, Strikethrough, Highlight | **Match** | Portable `==highlight==` behavior depends on Omawrite’s renderer; delimiter edge cases remain bounded. |
| Inline code / code block | **Match** | Backtick-safe inline code and fenced block insertion exist. |
| Link | **Match** | Includes clipboard URL insertion. |
| Wikilink | **Partial** | Insert/render/open works, but nearest-match library resolution and a complete cross-file fragment matrix are open. |
| Footnote | **Partial** | Multiline/repeated footnotes and navigation work; complete Markdown grammar and every return-link case are not claimed. |
| Content Block | **Partial** | Bounded Markdown, CSV, code and local-image inclusions exist. Full rebasing, title syntax and cycle/grammar parity remain incomplete. |
| Hashtag | **Match** | Insert plus bounded saved-file tag indexing/navigation exist. |
| Date / Table / TOC / Horizontal Rule / Page Break | **Partial** | Each has a working command. Table editing is basic, TOC is generated rather than maintained live, and paginated edge cases remain. |
| Clear Styles | **Partial (Cycle 49)** | Explicit selections can clear supported outer inline wrappers and simple whole-line heading, quote and list/task markers in one undoable action. Code, links and ambiguous or nested Markdown are refused without mutation; this is not a complete block-and-inline reset. |

### Authors and Focus

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Set up authors / categories | **Partial** | Manual author name and Human/AI/Reference range labels exist. No dedicated Authors menu, reusable author registry or verified provenance. |
| Enable Focus Mode | **Partial** | Sentence, paragraph and typewriter controls exist individually; there is no iA-style master Focus switch/state model. |
| Sentence / Paragraph / Typewriter | **Match** | Useful native-menu equivalents exist. Sentence boundaries use bounded heuristics rather than a full language grammar. |
| Syntax: parts of speech | **Partial** | Writing Review lists macOS lexical classes while open. It does not color words live in the editor or provide per-class visibility toggles. |
| Style Check | **Partial** | Writing Review supports custom words plus a small built-in watch-word list. It is not iA’s live Fillers/Clichés/Redundancies rule set. |
| Hide Authors by category | **Gap** | Annotations can be listed and selected, but there is no live category overlay or visibility filtering. |

### View

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Library/organizer/sort/filter visibility | **Match** | State is shared with toolbar/sidebar controls. |
| Sort Files By | **Match** | Same four fields, direction and folders-first option. |
| Show Date | **Match (Cycle 48)** | Native View → View Options → Show Date and library/context menus offer Date Modified, Date Created and None. A synthetic file displayed distinct dates; sorting remained independent. See [Cycle 48 evidence](../research/cycle-48/README.md). |
| Text excerpts | **Match** | Bounded source excerpts with a compact default. |
| Tree/List navigation | **Partial (Cycle 51)** | Tree and current-folder List both reach nested files; List uses existing folder history and Show in Library. Tree expansion is session-only and resets on root changes. |
| Text size | **Match** | Larger, smaller and reset equivalents. |
| Show Completions | **Partial (Cycle 54a; native sample checked)** | A manual current-document popup offered two candidates; Return accepted one and one Undo restored the prefix. iA candidate source/ranking and remaining popup/IME/accessibility behavior remain unverified. |
| Reload Preview | **Match** | Forces Markdown reparse. |
| Preview Full/Split/Web | **Partial (Cycle 51)** | View → Preview now routes Full/Split/Web to existing continuous preview layouts. Web is not a separate renderer; exact iA behavior remains unverified. |
| PDF preview / fit controls | **Partial (Cycle 51)** | View → Preview → PDF → Paginated Preview opens the existing page/fit controls. iA's PDF submenu children were not captured, so exact structure and shortcut parity remain open. |
| Templates | **Partial** | Shared preview/export presets: Modern Sans, Classic Serif, Manuscript Mono, GitHub, Helvetica, Palatino and MLA Draft, plus JSON Custom. Duo/Quattro are missing; MLA Draft is not complete MLA Style; iA template-package import is unsupported. |
| Title bar modes | **Partial (Cycle 52; menu checked)** | Native View menu exposes Fade In/Out and Always Show for the QML filename in the 44 px top strip; exact fade feel, window controls and accessibility remain unverified. |
| Toolbar modes and stats-only display | **Partial (Cycle 52; menu checked)** | Native menu exposes Fade/Always/Hide and ten independent Stats Only metrics; one metric changed the footer and Default restored it. Stats Only appears in Omawrite's footer, unlike iA's top toolbar. Visual and keyboard acceptance remain open. |
| Statistics set | **Partial (Cycle 52; fixture checked)** | Native dialog reported sentence, speaking, task and manual Human/AI/Reference counts for a synthetic fixture. Live updates after edits/annotations and visual placement remain unverified. |
| Tab bar / all tabs / full screen | **Partial** | Native tab controls, tab overview and fullscreen lifecycle exist, but placement/labels differ and some platform action enabled states remain imperfect. |

### Go

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Document Back/Forward | **Match** | Session history restores cursor and guards dirty navigation. |
| Library Back/Forward / Enclosing Folder | **Match** | Separate library navigation history exists. |
| Open Link | **Partial** | Opens a source link at the cursor and preview links. Complete fragments/non-Markdown/nearest-match semantics remain open. |
| Quick Search | **Partial (Cycle 53; native menu checked)** | Go exposes Quick Search and New Smart Folder opened its blank dialog. It rereads bounded saved files rather than maintaining an incremental index; full search workflow remains unverified. |
| Command Palette | **Partial** | Searchable workspace command registry exists, but it does not yet contain every menu command. |
| Locations / Add Location | **Partial (Cycle 53; native menu checked)** | Go listed saved local locations and Add Location. Parent/child overlaps are rejected; cloud folders are ordinary local paths. Unavailable-state checks remain. |
| Recents / Smart folders | **Partial (Cycle 53; native menu checked)** | Go listed recent files, saved queries and New Smart Folder; the latter opened a blank search. Its nested Recents submenu differs from the captured direct action; iA smart-folder semantics remain broader. |
| Hashtags | **Partial (Cycle 53; native small-root checked)** | A one-file root showed #alpha and added #beta automatically after an external edit; Go listed both. Large, inaccessible or rejected roots state that manual refresh is needed, but those cases remain unverified natively. |

### Window, application and Help

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Minimize / Zoom / Bring All to Front | **Match** | Routed through AppKit. |
| Zoom All / Fill / Center | **Gap** | Not present in Omawrite’s Window menu. |
| Move & Resize / Full Screen Tile / window sets | **Gap** | Current macOS provides these native actions, but Omawrite does not expose the captured hierarchy. Use supported AppKit integration rather than custom geometry logic. |
| Move to display | **Partial** | Saved display selection/restoration exists for sessions, but there is no Window-menu move-to-display command. Multi-display hardware acceptance remains open. |
| Previous/Next tab, detach, merge, tab overview | **Match** | Native actions exist; labels differ and state-dependent enablement needs polish. |
| Dynamic open-window list | **Unverified** | Multi-window ownership exists, but the source does not define a dynamic Window-menu document list. Check what AppKit injects in the packaged app. |
| Application menu | **Partial** | Native lifecycle, Quit and standard macOS integration exist. About/Settings/Updates/Services coverage needs a fresh packaged-app audit. |
| Help | **Gap** | Only Keyboard Shortcuts is currently exposed. There is no in-app guide, What's New, support destination or searchable help menu. |

## Omawrite capabilities beyond the captured iA menus

These are real differentiators or useful local-workflow additions, though some may exist elsewhere in iA and were simply not visible in this capture set.

| Omawrite addition | Current behavior |
|---|---|
| Open by Path | Opens a local Markdown/text file or folder from an entered path, with guarded unsaved changes. |
| Workspace themes | Follow System, Light, Dark and Warm Paper apply across the workspace and preview. This is separate from output templates. |
| Explicit safety controls | Toggle one-minute autosave, keep previous version on save, create a saved-file version, and restore a version with autosave paused until review. |
| Recovery and external-change protection | Crash recovery snapshots, original-disk baselines, external-change prompts and coordinated multi-document quit approval. |
| Custom output template | Load bounded JSON with font, point size, header, footer and title-page settings; preview/export share the selected preset. |
| Paginated preview | Page navigation, fit page/width, orientation and the same output renderer used by PDF/print. |
| Portable HTML raster assets | Export can embed local raster images rather than depending on the source path. |
| Manual authorship metadata export | Sidecar-backed range assertions can be exported separately as JSON; annotated clipboard transfer works between Omawrite windows. |
| Command palette | Gives keyboard access to a growing registry of document, layout, theme, search and output commands. |
| Local saved-query/tag workflow | Saved content queries and bounded tag counts operate on ordinary local Markdown without a server account. |
| Rich file/folder context menus | File and folder rows expose safe duplicate/rename/Trash, Finder, sharing, output, clipboard, creation, sorting and view actions. |

## Gap summary for the next planning pass

The remaining work is not evenly distributed. File management and core Markdown formatting are close. The largest functional clusters are:

1. **Live writing intelligence:** completions, substitutions/corrections, live parts-of-speech highlighting, real style-check categories and author visibility overlays.
2. **Authorship semantics:** reusable authors, iA-style Mark As/Paste Edits workflows, category display controls and a clearer truth model for provenance.
3. **Library presentation:** flat List navigation, selectable date field, native-menu smart folders/tags and incremental indexing/watching.
4. **View chrome:** title/toolbar fade modes, stats-only toolbar and the missing statistics.
5. **Platform integration:** current macOS Window actions, dynamic window list, complete application/Help menus and fresh enabled-state/accessibility QA.
6. **Deep correctness:** full Clear Styles, ordered task lists, advanced wikilink/content-block/Markdown edge cases, cross-app clipboard checks and hardware acceptance.

Before implementation planning, recapture the still-hidden iA submenus and the Authors/Application/Help menus. Their labels and behaviors should not be invented from the parent arrows.
