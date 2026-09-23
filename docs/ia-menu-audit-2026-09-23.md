# iA Writer menu map and Omawrite gap audit — 23 September 2026

This is the planning baseline after Omawrite Cycle 47 (76 tests), updated through the Cycle 59 implemented-subset checkpoint on 24 September. Current source passes 99 tests; Cycles 48–51 are complete and later cycles retain explicit acceptance gaps. It transcribes the menu screenshots in `screenshots/` and compares them with the source. It is an interface and behavior audit, not a claim that similarly named commands behave identically.

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

The screenshot folder does not contain the application, Authors or Help menus, several Edit and File children, or View → Preview → PDF. A [native accessibility follow-up on 24 September](../research/menu-audit-2026-09-24/native-submenus.md) captured those menu labels. A later [isolated sample-input check](../research/cycle-54/native-substitutions.txt) observed Smart Quotes typing and Undo. Post-authorship setup items, other substitution triggers, dynamic OS states and exact behavior remain unverified.

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

- Not opened in this screenshot set. The 24 September native inspection showed **Set Up Authorship…** only before setup. Its sheet has required Name and optional Identifier; post-setup items remain unknown. Human/AI/Reference categories were observed earlier in Settings.

### View

- Hide Library; Hide Organizer
- Hide Sort Bar; Hide Filter Bar
- Sort Files By → Date Modified, Date Created, Name, Extension; A to Z, Z to A; Pin Folders to Top
- View Options → Show Date → Date Modified, Date Created, None; Show Text Excerpts; Navigation → Tree, List
- Text Size → Make Text Bigger, Make Text Normal Size, Make Text Smaller
- Show Completions
- Reload Preview
- Preview → Full, Split; Web; PDF → Fit Page, Fit Width (PDF children confirmed in the 24 September native inspection)
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

These menus were not opened in the screenshot folder. The 24 September native inspection confirmed About, Check for Updates, Settings, Services, Hide and Quit/Quit and Close All Windows in the application menu; Help search, iA Writer Help, What's New and online support in Help. Their destinations and enabled-state behavior were not exercised.

## Current parity map

### File

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| New / New in Library / in Window | **Match** | Current-window/new-window behavior exists with guarded unsaved changes. |
| Open / Open Recent / Close / Save | **Match** | Recents are bounded local history with clear action. |
| Duplicate / Rename / Move To | **Match** | Separate-volume Move To and physical failure cases still need hardware acceptance; relative assets are not relocated. |
| Revert To / versions | **Partial** | Omawrite can create automatic/manual macOS versions and restore one into the editor with one-step Undo. It does not expose the native Browse All Versions interface, and authorship sidecars are not versioned. |
| Show in Finder / Show in Library | **Match** | Implemented for the active saved file. |
| Share | **Partial** | File → Share Markdown sends current editor text; iA's native menu exposes system share targets including AirDrop/Mail/Messages/Notes and extensions. Target availability is dynamic; equivalent cross-app delivery is unverified. |
| Export | **Partial** | HTML and PDF plus portable raster HTML exist. iA format/options parity and broad complex-document export acceptance remain open. |
| Print / Page Setup | **Match** | Rendered printing, Markdown-source printing, paginated preview and persistent page setup exist; physical printer acceptance remains open. |

### Edit

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Undo/Redo; Cut/Copy/Paste/Delete/Select All | **Match** | Core native-menu actions track the focused field. |
| Copy Formatted / HTML / Markdown | **Partial (Cycle 50)** | Commands now appear directly in Edit and enable for an editor selection. Cross-application clipboard acceptance remains open. |
| Paste As | **Partial** | Plain text and Markdown-from-HTML exist. Before authorship setup, iA showed only Set Up Authorship…; post-setup conversions are unknown. |
| Paste Edits From | **Gap** | No authorship-aware merge/paste workflow. |
| Mark As | **Partial** | Manual Human/AI/Reference/Unknown range annotations exist in a dialog, with a sidecar and metadata export. They are assertions, not verified provenance, and are not exposed as iA-style menu commands. |
| Find | **Partial** | Find, replace, next/previous and use selection exist. iA also has Filter Library… in Find; Omawrite routes library filtering/search elsewhere. |
| Spelling and Grammar | **Partial** | Explicit macOS dictionary/grammar review with suggestions, Replace and Undo exists. iA's menu also exposes while-typing spelling/grammar and automatic correction; Omawrite has no equivalent inline controls. |
| Substitutions | **Partial (Cycles 54b–54c)** | iA exposes Show Substitutions, Smart Copy/Paste, Smart Quotes, Smart Dashes and Text Replacement. Omawrite has independent persisted opt-in Smart Quotes and a bounded spaced-prose Smart Dashes rule. Native typing and first-Undo restoration were checked on disposable samples. Other children, full input-method coverage and exact iA heuristics remain open. |
| Transformations | **Partial (Cycle 50)** | Uppercase, lowercase and title case appear under Edit → Transformations. iA's Make Upper Case/Lower Case/Capitalize/Make Title Case hierarchy is now captured; Capitalize remains missing. |
| Speech | **Match** | Speak Selection and Stop Speaking use macOS speech; audible quality is not acceptance-tested. |
| AutoFill / Dictation | **Partial (Cycle 50)** | iA's AutoFill shows Contact/Passwords/Credit Card and AppKit exposes Dictation. Omawrite relies on platform injection; availability depends on macOS and focused control. |
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
| Wikilink | **Partial (Cycle 58a)** | Insert/render/open works, including explicit local heading fragments and duplicate suffixes. Nearest-match library resolution, cross-window fragment transfer and broader parser semantics remain open. |
| Footnote | **Partial** | Multiline/repeated footnotes and navigation work; complete Markdown grammar and every return-link case are not claimed. |
| Content Block | **Partial** | Bounded Markdown, CSV, code and local-image inclusions exist. Full rebasing, title syntax and cycle/grammar parity remain incomplete. |
| Hashtag | **Match** | Insert plus bounded saved-file tag indexing/navigation exist. |
| Date / Table / TOC / Horizontal Rule / Page Break | **Partial** | Each has a working command. Table editing is basic, TOC is generated rather than maintained live, and paginated edge cases remain. |
| Clear Styles | **Partial (Cycle 49)** | Explicit selections can clear supported outer inline wrappers and simple whole-line heading, quote and list/task markers in one undoable action. Code, links and ambiguous or nested Markdown are refused without mutation; this is not a complete block-and-inline reset. |

### Authors and Focus

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Set up authors / categories | **Partial (Cycle 56a)** | macOS now has Authors → Set Up Authorship… with a persistent required Name and optional Identifier. Automated coverage verifies validation, Save/Cancel/reopen persistence and complete document/sidecar/recovery isolation. Native QA observed the menu and blank sheet, then entered a synthetic Name to confirm Save became enabled and cancelled without changing source/status. Native Save/reopen was not exercised to avoid retaining a fake profile. The profile does not assign labels or verified provenance. Post-setup iA behavior, categories, Mark As and Paste Edits From remain unknown or unimplemented. |
| Enable Focus Mode | **Partial (Cycle 55a)** | Native Focus menu now nests Sentence, Paragraph and Typewriter under this label. There is no separate master Focus switch; exact iA activation semantics remain unobserved. |
| Sentence / Paragraph / Typewriter | **Partial (Cycle 55a)** | Existing controls are grouped in the captured order. Sentence/Paragraph are exclusive, Typewriter remains independent; visual dimming/scroll and checked-state native acceptance remain open. |
| Syntax: parts of speech | **Partial** | Writing Review lists macOS lexical classes while open. It does not color words live in the editor or provide per-class visibility toggles. |
| Style Check | **Partial (Cycles 55b–55c)** | Focus → Enable Style Check has persisted, independent live Fillers and Custom toggles. Fillers matches `very`, `really`, `quite` and `just` as case-insensitive Unicode whole words. Both categories share the first-50,000-UTF-16-unit and 1,000-result caps and exclude fenced/indented/inline code, URL destinations and raw tags. Custom supports at most 32 unique terms of 64 units each and wins an exact overlap. A later live view visibly confirmed yellow Fillers highlights in prose and a link label, with no yellow on `veryish`, URL destination, code or raw tag attributes; status stayed unchanged. No screenshot file was persisted. Clichés, Redundancies and Show Syntax are absent. |
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
| PDF preview / fit controls | **Partial (Cycle 51)** | iA's PDF submenu has Fit Page and Fit Width. Omawrite routes through Paginated Preview to existing fit controls, so hierarchy differs; output/shortcut acceptance remains open. |
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
| Open Link | **Partial (Cycle 58a)** | Explicit local `.md`, `.markdown`, `.mdown`, `.txt` and `.text` links, `[[wikilinks#fragments]]` and same-file heading fragments route from source, Go → Open Link and preview. Duplicate headings use suffixed slugs such as `#same-1`; fragments do not become part of file identity. Native Dev opened the synthetic target cleanly with no `#` in its title; a later live screenshot displayed the second `Same` heading with the caret at its start, but no file was persisted. Preview scroll and dirty Cancel remain automated only; cross-window fragment transfer, missing-anchor feedback and broader parser/output parity remain open. |
| Quick Search | **Partial (Cycle 53; native menu checked)** | Go exposes Quick Search and New Smart Folder opened its blank dialog. It rereads bounded saved files rather than maintaining an incremental index; full search workflow remains unverified. |
| Command Palette | **Partial** | Searchable workspace command registry exists, but it does not yet contain every menu command. |
| Locations / Add Location | **Partial (Cycle 53; native menu checked)** | Go listed saved local locations and Add Location. Parent/child overlaps are rejected; cloud folders are ordinary local paths. Unavailable-state checks remain. |
| Recents / Smart folders | **Partial (Cycle 53; native menu checked)** | Go listed recent files, saved queries and New Smart Folder; the latter opened a blank search. Its nested Recents submenu differs from the captured direct action; iA smart-folder semantics remain broader. |
| Hashtags | **Partial (Cycle 53; native small-root checked)** | A one-file root showed #alpha and added #beta automatically after an external edit; Go listed both. Large, inaccessible or rejected roots state that manual refresh is needed, but those cases remain unverified natively. |

### Window, application and Help

| iA capability | Omawrite status | Remaining difference |
|---|---|---|
| Minimize / Zoom / Bring All to Front | **Match** | Routed through AppKit. |
| Zoom All / Fill / Center | **Partial (Cycle 57b)** | Center is present after Zoom and invokes AppKit’s native Center action for a normal window. Native Dev showed it enabled and invoked it without changing synthetic Target.md status. Available-screen geometry is tested offscreen only; OS geometry and multi-display behavior remain unverified. Zoom All and Fill are absent. |
| Move & Resize / Full Screen Tile / window sets | **Gap** | Current macOS provides these native actions, but Omawrite does not expose the captured hierarchy. Use supported AppKit integration rather than custom geometry logic. |
| Move to display | **Partial** | Saved display selection/restoration exists for sessions, but there is no Window-menu move-to-display command. Multi-display hardware acceptance remains open. |
| Previous/Next tab, detach, merge, tab overview | **Match** | Native actions exist; labels differ and state-dependent enablement needs polish. |
| Dynamic open-window list | **Unverified** | Multi-window ownership exists, but the source does not define a dynamic Window-menu document list. Check what AppKit injects in the packaged app. |
| Application menu | **Partial** | iA's About/Updates/Settings/Services/Hide/Quit/Quit and Close All Windows are now captured. Omawrite has native lifecycle and Quit; remaining destinations/actions need a packaged-app audit. |
| Help | **Partial (Cycle 57a)** | Omawrite offers bundled offline Omawrite Help and What’s New in Omawrite in a read-only Markdown view, plus Keyboard Shortcuts. Native accessibility verified both pages and Close/Escape without changing a synthetic document. Screenshot capture was unavailable; dark/narrow/VoiceOver and OS Help search were not checked. Online Support remains absent because its destination is unverified. |

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

Cycle 59 integrates the implemented subset at code commit `5f8d64b`: 99 tests pass, both local bundles are refreshed/signed, and accumulated native evidence has inspected every top-level menu family. It is not a full closeout. The largest remaining clusters are:

1. **Cycle 54 writing input:** Smart Quotes and the observed Smart Dashes rule are verified bounded subsets; Text Replacement, Smart Copy/Paste, automatic correction and the remaining completion popup/IME/accessibility matrix remain.
2. **Cycle 55 writing intelligence:** Clichés, Redundancies, live parts-of-speech Show Syntax, Hide Authors and the remaining Custom/focus visual matrix.
3. **Cycle 56 authorship semantics:** post-setup iA behavior, reusable authors, Mark As/Paste Edits From, category display and a truthful provenance model.
4. **Cycle 57 platform integration:** Zoom All, Fill, Move & Resize, tiling, move-to-display, window sets/dynamic list, application destinations, OS Help search and Online Support. Center geometry still needs real screen/multi-display acceptance.
5. **Cycle 58 deep correctness:** cross-window/missing-anchor fragment behavior, complete Markdown/content-block grammar, parser/output parity, semantic scrolling and cross-application clipboard acceptance.
6. **Integrated acceptance:** a systematic saved/untitled/dirty matrix, enabled-state transitions, VoiceOver/high contrast, dark/narrow/fullscreen, multiple displays, hardware failures and notarized distribution.

Show Date, Tree/List navigation, native Go access, title/toolbar modes and the expanded statistics set are implemented subsets and no longer belong in the missing-feature list. Their row-level limitations remain explicit above. Labels and representative workflows do not establish exact iA Writer parity.
