# Fomawrite workspace UI redesign plan

Planning date: 3 October 2026. Original baseline: `52af519`. Cycles 90–100 and 102 implemented the workspace and editing foundations, but Andrew’s 4 October visual comparison rejected the remaining gap between the approved concept and the shipped app. Cycle 103 follows the concept’s actual geometry, typography, icons and writing presentation; Cycle 104 refines the folder heading and formatting tools and adds quick text sizing. The current target is `0.3.0-dev5`, macOS `0.3.0 (104)`. [Native screenshots and verification](../research/cycle-104/README.md). Visual sign-off from Andrew remains pending.

The approved direction makes Fomawrite a composed writing workspace. Andrew's preferred reference is the Ulysses window in his three-app comparison: a quiet gray organizer, clean document list, warm writing canvas, and clearly grouped controls belonging to each column. Preserve Fomawrite's blue folders, local Markdown files and existing editing protections.

The largest change is structural. Pane ownership, layout state and reusable controls now have separate implementations. The standards and acceptance matrix below remain the reference for reviewing the complete workspace.

## Approved implementation checkpoint

The approved direction is implemented as one integrated candidate, with the cycle boundaries used to describe scope rather than separate accepted releases. This is a workspace redesign and an implemented subset of editing behavior; it does not establish complete Ulysses, Typora or iA Writer parity.

| Cycle | Implemented scope | Remaining acceptance |
| --- | --- | --- |
| 90 — Design foundation | Approved composition, pane ownership, blue folders, control vocabulary and responsive fit rules. | Judge the result in the native app against the complete composition. |
| 91 — Shared components | Semantic palette roles, round toolbar actions, capsule groups and navigation/control states. | Physical VoiceOver, full contrast audit and actual display-scale review. Cycle 98 verifies the inactive toolbar focus treatment. |
| 92 — Column-owned shell | A shared 52-pixel header follows real pane geometry; document history, formatting and workspace controls belong to the document column. | Cycle 98 verifies native full screen and tabs; title-bar/traffic-light feel and broader compact review remain human acceptance. |
| 93 — Responsive column state | Independent desired visibility, remembered widths, temporary navigation drawers, ordered contraction, 48-pixel restoration margin and per-window checkpoint state. | Cycle 98 verifies native contraction, tab detach/regroup and independent checkpoints. Arbitrary-workspace quit/reopen and physical multi-display checks remain. |
| 94 — Writing finish | Editorial, Manuscript and Book presentation, explicit Source / Visual Edit / Preview choices, word-count control and 150% source line height. The permanent duplicate formatting footer is removed. | Long-document caret/scroll behavior, Undo/Redo and Andrew's writing review. |
| 95 — Navigation finish | Organizer hierarchy, contextual removal, blue folder icons, compact and two-line preview rows, scoped folder/file controls. | Long/Unicode names, unavailable locations and keyboard traversal. |
| 96 — Supporting UI sweep | Shared control treatment extends to compact menus, search actions, outline, preview and dialog buttons; existing Export band scrolling remains. | Complete supporting-surface inventory at narrow widths and in dark mode. |
| 97 — Candidate handoff | Version identification, regression and native verification, evidence and review checklist. | Verified candidate is built and synchronized to Dev, package and Applications; Andrew’s broad acceptance remains pending. See the Cycle 97 evidence record. |
| 98 — Keyboard/native acceptance | F6/Shift+F6 region traversal, compact Drawer focus/return, hidden-pane focus rescue and inactive toolbar rings. | 111 native assertions across Cycles 98/100 pass, with synthetic dark/inactive/full-screen captures. Physical VoiceOver/display scaling remain. |
| 99 — Safe visual structures | Simple table cells, inline code, image alt text/local thumbnails, list-end Return/task continuation/empty-item exit, grapheme safety and caret tracking. | All 134 tests and native Visual Edit checks pass. Complex structures, arbitrary multiline paste, splitting within a list item and spatial image/grid editing remain Source work. |
| 100 — Open-document links | Cross-window fragment transfer into live Markdown, missing-heading feedback and stale-preview completion protection. | Native fragment/dirty/Cancel/Undo checks pass. The final suite passes; Dev, package and Applications are synchronized to build 100. |

Existing stored theme and writing choices take precedence. New installations start with Studio and Editorial; existing users can choose **Studio writing layout** explicitly. Layout preferences are per window and do not live-mirror another window. Source remains canonical plain UTF-8 Markdown; Visual Edit still protects unsupported structures. Private sample documents and pre-existing dirty files are outside this cycle's edits.

After Cycles 103–104’s visual and toolbar feedback, the next product work remains **review-led block editing ergonomics**: review the combined build in normal writing, then improve the most useful image/table/list interactions while preserving exact-source checks and visible unsupported-edit fallback. Complete physical VoiceOver/display and arbitrary-workspace relaunch acceptance alongside that work. **Cycle 101 remains an optional distribution decision**, not an automatic public release. Current evidence: [Cycle 98](../research/cycle-98/README.md), [Cycle 99](../research/cycle-99/README.md), [Cycle 100](../research/cycle-100/README.md).

## Scope and evidence

- Reference: Andrew's supplied three-app screenshot, with Fomawrite behind iA Writer and Ulysses in front. It supports visual observations, not claims about the other apps' internal behavior. Its physical pixels, display scaling and inactive-window colors are not reliable design measurements.
- Current source: `Main.qml`, `OrganizerPane.qml`, `LibraryPane.qml`, `PreviewPane.qml`, `ChromeButton.qml`, `LineIcon.qml`, `WorkspaceCommands.qml` and the backend. Two independent planning reviews covered implementation constraints and visual design.
- Existing guidance: [editor surface work](editor-surface-redesign.md), [dialog guide](dialog-style-guide.md), [build log](build-cycles.md), [acceptance ledger](release-acceptance.md).
- Baseline validation: Cycle 89 records 118 passing tests and native window-routing checks. Those are historical results, not tests rerun for this document. Installed, packaged and Dev copies must be identified by commit before future comparisons; their presence alone does not establish that they match.
- The interactive concept demonstrates composition, blue folders, control grouping and writing appearances. Its simplified widths, breakpoints and appearance selector are illustrative; the written fit rules govern implementation. It is not native layout, complete feature or accessibility acceptance.
- The original planning pass produced a plan and illustrative concept without changing app code. Andrew subsequently authorized the implementation checkpoint above; current verification and deployment status belong in the Cycle 104 handoff; the Cycle 97 record preserves its earlier verified checkpoint.

## What exists and what must change

| Area | Already present at the baseline | Proposed change |
| --- | --- | --- |
| Workspace tones | Optional Studio palette separates organizer, file list and canvas. | Make those surfaces a complete system extending through their headers, menus and inactive states. |
| Fonts | System UI font on macOS; Manuscript, Editorial and Book writing presets. | Consistent hierarchy and spacing; tune the complete composition instead of adding another preset. |
| Folder identity | Blue outline folders in library and organizer. | Retain the recognizable symbol and blue meaning; normalize icon size, weight and alignment. |
| Toolbar | Formatting, Find, outline, appearance, preview and Export commands exist. | Replace global spacing arithmetic with pane-owned headers, round actions and real capsule groups. |
| Panels | Draggable SplitView and persisted visibility flags. | Independent desired visibility, remembered widths, accessible dividers and predictable narrow-window behavior. |
| Navigation | Separate document and folder histories; guards for unsaved files. | Make history discoverable without confusing its scope or bypassing those guards. |
| Writing | Source remains canonical; bounded Visual Edit; 140% source line spacing. | Refine appearance and mode labels while keeping unsupported editing explicit. Live line-spacing changes need separate safe handling. |
| Pop-ups | Reusable compact menus and an Export layout guide. | Apply a single control vocabulary across search, formatting, appearance, outline, statistics and dialogs. Retain the working Export resize/scroll behavior. |

## Proposed workspace composition

Three primary columns: **Organizer → Files → Writing**. Rendered preview is an optional fourth column with its own local controls. Existing outline/statistics surfaces are restyled; creating a new Ulysses-like inspector is not a prerequisite.

Each visible column carries its background through its header. All headers align to a common 52 logical-pixel band and their boundaries align with the dividers below. The native macOS traffic lights and tab strip remain native. The first implementation checkpoint must prove that this works with tabbed windows, full screen and title-bar dragging before the full toolbar is rearranged.

| Column | Header and actions | Content treatment |
| --- | --- | --- |
| Organizer | Collapse organizer; a small Add/organize menu beside its heading. Native window controls occupy their proper reserved region. | Clear section headings; consistent icon/text rows; selected location evident. Remove-shortcut actions move to contextual/overflow controls instead of a permanent row of Xs. |
| Files | Current folder and path access; New document plus related dropdown; list options and Search files. Folder traversal belongs here. | Two existing choices, Compact list and document previews, with the same type scale and selection treatment. Folder rows remain blue-icon rows. |
| Writing | Document Back/Forward capsule; Find in document; filename when space permits; B / I / Link / Paragraph capsule; Appearance, Export and Workspace access. | One continuous warm canvas, centered writing measure and a quiet accessible statistics control. |
| Preview | Explicit Preview title and local preview actions, plus Hide preview. | Rendered output remains distinguishable from editable text. Export style remains an output setting. |

Only document history gets the prominent Back/Forward capsule in the writing header. The library uses a breadcrumb/path menu and labeled folder-history commands; a second identical history pair must not be placed beside it without clear scope. Document history uses the existing guarded navigation path.

**Toolbar sizing and overflow:** first remove the optional filename from the toolbar (the document still has a title elsewhere), then move secondary actions into a labeled More menu, then collapse formatting into one Format control. Preserve Workspace, current editing mode and access to Find and navigation. Do not shrink icons or targets until they become hard to hit, wrap toolbars into accidental second rows or let controls cross a pane divider. Calculate fit from the width of the column, not the whole window. Subtract native-control reservation, insets, inter-group gaps and separators before allocating controls. The traffic-light reservation follows the first visible column when Organizer is hidden; preserve a usable native drag region. A compact first document header can budget 80 for native controls, 64 for history, 32 each for Find, Format and Workspace, 40 for gaps and 24 for insets: 304 total before a drag region. Put the explicit document-mode selector in a separate stable document-local strip when it cannot fit; do not crowd it into this budget.

**Duplicated controls:** remove the permanent duplicate formatting footer once the new header/overflow is reachable at every supported width. A contextual bottom formatting bar may remain as a user option; it must not appear alongside the same always-visible controls by default. Preserve existing toolbar fade/hide/statistics preferences, with a reliable keyboard/menu reveal route.

## Shared visual standards

All measurements below are proposed logical pixels, to be reviewed in the actual app. They are starting values, not sampled Ulysses specifications.

| Element | Proposed standard |
| --- | --- |
| Spacing | 4-unit scale; 8 between related items; 12 between groups; 12–16 pane inset. |
| Main interface type | Platform UI font, 13 regular; 13 medium for important labels; 12 for secondary information. |
| Section headings | 12 medium, sentence case; 16 above and 8 below. Avoid very thin or tiny gray text. |
| Organizer rows | 28 high; 8 outside inset; 6 radius; 16 icon; 8 icon-to-label gap. Offer a comfortable density if needed without changing writing size. |
| File rows | Compact rows follow the same rhythm. Preview rows have a title, optional date and at most two snippet lines; no decorative cards around every file. |
| Toolbar actions | 32 × 32 target, circular resting surface, 16–18 icon. |
| Related-action groups | 32 high capsule; 30–32 wide segments; one group background and border, not four individual pills touching. |
| Task-dialog actions | Keep the existing 36 high standard and equal Cancel/primary dimensions. Toolbar circles do not replace labeled dialog buttons. |
| Splitters | 1 visible line with an 8-wide drag region; resize cursor; keyboard/menu alternative. |
| Scrollbars | Remain within their own scrolling pane, with reserved clearance from controls. Keep the existing minimum 16 clearance for large dialogs. |

### Surface and state tokens

Proposed Studio light values: organizer `#ECEDEF`, files `#FAFAF9`, canvas `#F6F3EE`, primary text `#34363A`, secondary text `#62666D`, divider `#D8DADD`, row selection `#D7DADE`. Retain the existing folder-blue identity, adjusting only if rendered contrast needs it. These are design candidates, not an exact color match.

Define semantic tokens for organizer, files, canvas, header, popover, field, control rest/hover/pressed/selected, text, muted text, selection, focus, divider and folder. Provide explicit light, dark and inactive-window mappings. Avoid local hardcoded colors in search or pop-ups.

Every action must have these distinguishable states:

| State | Required treatment |
| --- | --- |
| Rest | Quiet visible fill and edge: it is identifiable as a control without hovering. |
| Hover | Stronger neutral fill; tooltip names the action and scope. |
| Pressed | Stronger temporary treatment than hover. |
| Selected or toggled | Persistent treatment distinguishable from hover, with an accessible state. |
| Keyboard focus | Clear 2-pixel focus indicator independent of selected state. |
| Disabled | Clearly unavailable; no misleading hover treatment; understandable reason where needed. |
| Inactive window | Softer emphasis while retaining readable text and visible selection. |

Validate normal functional text at 4.5:1 contrast and meaningful control/focus boundaries at 3:1 against adjacent surfaces. Decorative dividers can stay subtle. A warm background must not force tiny low-contrast gray text.

### Folder and icon rules

Keep blue for real filesystem folders, monochrome document outlines for files, search symbols for saved queries and tag symbols for tags. A favorite folder still looks like a folder; a small badge or section placement communicates favorite status. Unavailable locations need an explicit unavailable state, not a misleading ordinary selection.

Use one original line-icon family with consistent visual weight and baseline. Preserve familiar existing folder artwork; do not import Ulysses icons or mix emoji, arbitrary Unicode glyphs and line icons as permanent controls. Custom per-folder artwork and folder colors are a later option, not needed to solve the current hierarchy.

### Toolbar refinements — Cycle 104

The folder heading is a labeled, tonal button with the same blue folder glyph as filesystem rows. Its tooltip names the current folder and the chooser action; clicking retains the native folder picker. Do not imply an inline menu with a disclosure chevron when the action opens a system picker.

Formatting uses typographic **B**, italic **I** and **¶** to communicate the result, with the existing line-link glyph. These are semantic formatting symbols, rather than a second decorative icon family. A separate two-segment **− / +** capsule sits between formatting and **Aa**, sharing the persisted writing-size commands and their 12–32 bounds. Its change must be visible in Source, Preview and Visual Edit without modifying document bytes or export point size. At narrow widths, preserve zoom/Aa and pane restoration; collapse formatting and omit the history capsule when necessary. History remains in the native Go menu and its existing shortcuts.

## Collapsing, resizing and restoring columns

**Desired layout and effective layout are different state.** Save the user's requested visibility and last useful widths; calculate temporary responsive hiding separately. This also applies to desired split/one-surface layout and the Source/Visual Edit mode: automatic fallback never changes that intent. A resize must not overwrite those preferences.

| Pane | Default width | Minimum docked width | Upper range |
| --- | --- | --- | --- |
| Organizer | 208 | 184 | 288 |
| Files | 288 | 240 | 420 |
| Writing | Remaining space | 480 when alongside auxiliary columns | Writing measure stays capped inside a wider pane. |
| Preview | 420 | 320 | Adjustable without starving the active editor. |

The window's current 720-pixel minimum remains initially. A lower window minimum is a separate acceptance decision after the compact layout works; planning examples below that width do not promise a shipping change.

**Manual hide:** a local collapse action hides only that pane and returns focus to an appropriate visible control or the active editor. Retain width, selection, expanded sections and scroll position. Organizer must be usable independently of the file list. Hiding a column never destroys or reloads the document editor.

**Restore:** an always-available Workspace control in the writing area presents Organizer, Files and Preview toggles, plus a Restore layout action. Native View commands and shortcuts remain available. When the source pane is hidden in a rendered-only view, the Workspace control moves to the surviving document header. The UI must never hide both document surfaces or strand the restore control.

**Narrowing:** the definite order is (1) reduce auxiliary columns to their minima, (2) temporarily collapse Organizer, (3) temporarily collapse Files, and (4) if the remaining document split still cannot fit, switch the split to one document surface. Preserve a requested split for as long as its source and preview minima can fit. Preserve the actively edited surface. A side-by-side preview falls back to a one-surface mode with an explicit Source / Visual Edit / Preview selector; the user can switch without losing content or position. Retain separate source/projection cursor and scroll positions. An explicit mode selection changes desired mode; automatic fallback does not. With no auxiliary panes, the editor may use the available compact width below 480.

**Temporary navigation:** restoring an auxiliary pane when it cannot dock opens an opaque temporary navigation panel. Escape or its close action dismisses it; selecting a file closes it only after guarded navigation succeeds. A canceled unsaved-change prompt must leave the previous document and navigation state intact. Keyboard focus returns predictably; overlays require proper accessibility and focus handling.

**Keyboard regions (Cycle 98):** F6 and Shift+F6 cycle visible Organizer, Files, Source, Preview and toolbar regions. Tab remains writing input in Source. Pane contraction rescues focus from a newly hidden region; modal navigation takes focus on Close and Escape restores the initiating control if it remains available. A keyboard-focused toolbar reveals faded controls, and inactive windows suppress the active focus ring. These shortcuts do not change saved layout intent.

**Growing again:** restore only panes that were temporarily hidden, including the requested split document layout without silently toggling Visual Edit. Never reopen a manually collapsed pane. Use a small restoration margin, initially 48 pixels, to avoid repeated hide/show near a boundary.

Do not introduce horizontal scrolling for the whole writing workspace. Export is a different task surface: its horizontal band scrolling at narrow widths is an intentional existing behavior and must remain available.

**Persistence:** pane visibility and widths are per window/workspace, restored with that window. New windows inherit reasonable defaults rather than live-mirroring another window's changes. Migrate the current shared settings once, without changing theme, writing appearance, root folder or document mode. Integrate a versioned pane-state object into the existing per-window checkpoint and restoration in `src/main.cpp`, with defaults for older checkpoints. Persist desired widths before responsive contraction, never the temporarily constrained widths. Native tab grouping does not make layouts shared; untitled recovery retains its existing policy. Remembered widths are clamped to the current display. Validate new-window, tab-detach and workspace-restoration behavior explicitly.

## Writing appearance and mode clarity

Use the existing three appearances. Presentation, editing mode, workspace layout and export style stay independent.

| Preset | Proposed visual target | Purpose |
| --- | --- | --- |
| Editorial | System sans around 19; roughly 1.5 line height; 68–74-character comfortable measure, capped near 720; warm neutral canvas. | Recommended Ulysses-inspired everyday writing view. |
| Manuscript | Bundled mono around 17; roughly 1.5 line height; about 66 characters. | Typewriter feel and visible Markdown structure. |
| Book | Verified serif around 20; roughly 1.5 line height; 64–70-character measure, capped near 720. | Long-form drafting and reading. |

These are proposed starting values. Preserve existing saved preferences, font-size choices and export templates. Show the proposed Studio + Editorial combination as an explicit appearance choice for existing installations; make it the default for new users only after Andrew's acceptance.

Use around 64 top inset in roomy windows, 32 in shorter ones, and at least 24 side gutters. Center the text measure while leaving navigation controls in their proper column. Paragraph rhythm comes from rendering, not inserting blank lines into Markdown. Keep code legible and retain the source editor's existing wrapping in this pass; do not shrink the whole page. Per-block horizontal scrolling inside the canonical plain TextEdit needs different rendering/editing architecture and is deferred to the separate source-mapping work.

Markdown markers should recede without disappearing or making caret placement mysterious. Selection and syntax around the caret must be clear. The statistics chip lives in a reserved strip above the text, cannot obscure content and behaves as a real keyboard-accessible button.

The mode names must say what they do: **Source** for Markdown, **Visual Edit** for the supported editing projection, **Preview** for read-only rendered output. Split is a layout choice, not an editing mode. Replace the present mixed labels only through a documented migration that preserves current state. Formatting controls must target the active editable surface or visibly explain why an operation is unavailable.

Cycle 99 adds image alt-text editing with local thumbnails and bounded table/list interactions. Full spatial image cards, arbitrary nested lists/tables and Ulysses-style annotations remain separate source-mapping work. A prettier writing surface must not imply those features already work. Preview/export typography may intentionally differ from writing typography; do not force export styles to change when the editor appearance changes.

## Sweep through supporting surfaces

Apply the same tokens and component rules after the main shell is stable:

1. Organizer and file rows, context menus, selection, sort/filter and file previews.
2. Quick Open and library search, clearly distinguished from document Find/Replace.
3. Formatting, paragraph, link and Appearance popovers; consistent anchoring and focus return.
4. Outline and statistics surfaces, with explicit close actions and clear scope.
5. Export, style gallery and paginated-preview controls, preserving their resize, independent scrollbars, horizontal access and fixed footer.
6. Small app-owned dialogs, warnings, empty/loading/unavailable states and tooltips. Native file, print and share panels retain platform behavior.

Use round/capsule controls for toolbars, list-row selection for navigation, and labeled buttons for task actions. Consistency means sharing typography, states and spacing, not making every control the same shape.

## Proposed development cycles

Every cycle has a bounded result and an exit check. The table below preserves the original planned work units and review criteria. Andrew has authorized implementation; the checkpoint above records what is integrated, while the exit checks still require evidence and acceptance.

| Cycle | Deliverable | Exit check and review |
| --- | --- | --- |
| 90 — Design foundation | Approve composition, token/state tables, toolbar ownership, collapse rules and a same-document wide/narrow concept. Inventory every affected control and current build identity. | **Review A:** Andrew approves the complete direction and one compact state before product implementation. Resolve native title-bar feasibility first. |
| 91 — Shared components | Create semantic surface/type/spacing tokens plus round action, capsule group and separate navigation-row components. Establish a small native component gallery for states. | Compare rest/hover/pressed/selected/disabled/focus in light, dark and inactive windows. No unrelated layout rewrite. |
| 92 — Column-owned shell | Replace toolbar width arithmetic with actual pane headers; add grouped editing/history actions; maintain native traffic lights, tabs and title-bar drag area. | **Review B:** working wide layout, toolbar ownership and writing focus. Every shown action works through the existing guarded command path. |
| 93 — Responsive column state | Independent collapse/restore, remembered widths, usable splitters, temporary navigation, compact document modes and state migration. | Real resize/collapse/reopen tests; no stranded panels, preference loss, tab interference or unsaved-state loss. Review wide → narrow → wide. |
| 94 — Writing finish | Refine existing appearances, safe line spacing, margins, syntax visibility, statistics and explicit mode controls. Remove redundant chrome only after replacement access works. | **Review C:** Andrew writes in Editorial and Manuscript on his normal display. Preserve exact source, undo, cursor, selection, scroll, dirty/recovery state and export style. |
| 95 — Navigation finish | Sidebar hierarchy, blue-folder standards, compact/preview file rows, scoped search/history and context-menu consistency. | Exercise click/right-click/keyboard paths on saved, unsaved, unavailable and long-named files. New Tab/New Window routing remains correct. |
| 96 — Supporting UI sweep | Apply approved controls to popovers, outline/statistics, search, Export and app-owned dialogs. | Width/theme/state inventory shows no outlier control. Recheck narrow Export horizontal access and equal footer actions. No parser or export-style rewrite. |
| 97 — Acceptance and release handoff | Native visual/accessibility/state review, clean build/test, screenshot set, version identification and package synchronization after approval. | **Review D:** complete user workflow and compact layout accepted. Refresh Dev, package and Applications safely, verify they match, then commit/push and report the exact test build. |

Review A authorized the complete implementation direction. The native, accessibility and writing reviews remain acceptance work for the integrated candidate. Avoid two agents editing the same large `Main.qml` sections concurrently.

### Model and agent use

Use GPT-6.1 Sol for the main implementation and test work, with scoped tasks and small reviewable diffs. Use Astra for the initial architecture/design review, difficult native interaction questions and an independent final critique. Focused lower-cost agents can inventory controls, audit regressions or maintain evidence. Model choices depend on available controls and do not replace native visual testing. No model or speed setting is changed by this plan.

## Implementation safeguards

- The planning baseline used pane-width arithmetic in `Main.qml`. The shared header now follows actual pane geometry; preserve that relationship rather than adding independent offsets.
- Keep toolbar actions and organizer rows separate: `ToolbarButton.qml` owns round header controls, while `ChromeButton.qml` retains row/text geometry. Preserve accessibility and hover/focus behavior in both.
- Keep Organizer visibility independent of Files. The explicit desired/effective state model replaces the old combined visibility and fixed 1000-pixel cutoff.
- `win.requestHistory()` handles document navigation, cursor restoration and unsaved-change prompts. Toolbar history must use it; folder history stays independent.
- `sourceFormattingAllowed()` protects against formatting a stale source selection while Visual Edit is focused. All new controls must respect the same active-surface rules.
- `Backend::applyDocumentTypography()` disables and reenables undo for a freshly loaded document. Do not call it to apply live line spacing to an active draft. Design a separate presentation-only mechanism and verify undo preservation.
- Keep hidden source/editor objects alive, along with source mapping, external-file conflict protection, recovery and the additional-window safeguards from Cycle 89.
- Review icon rasterization at actual device pixel ratios. Do not judge sharpness from browser mockups or resized screenshots.
- Faded toolbar controls need deliberate keyboard reveal and hit-testing rules; invisible-but-clickable controls are not acceptable.
- Keep macOS-specific chrome handling separate from Linux. This plan does not require a toolkit migration.

## Acceptance matrix and definition of done

Use the same synthetic document set, settings and known build in each comparison: short prose, long prose, nested folders, long/Unicode filenames, Markdown lists/code/tables/images, and an unsaved draft. Capture app-only screenshots, excluding private writing and the desktop.

| Dimension | Required evidence |
| --- | --- |
| Size | Approximately 1440, 1120, 900 and 720 logical pixels; short window at current 520 minimum; source/preview split and editor-only. A 500-wide concept is exploratory until a lower minimum is approved. |
| Appearance | Studio/Editorial, Manuscript, Book and dark; active and inactive windows; standard and scaled macOS display settings. |
| Controls | Every toolbar action, overflow route, menu equivalent, disabled state, pointer hover, keyboard focus and tooltip. No overlapping targets or controls crossing pane boundaries. |
| Layout | Manual hide, automatic hide, restore, divider drag, grow after shrink, quit/reopen, multiwindow and native tabs. Preserve desired layout independently of temporary presentation. |
| Editing safety | Change appearance and pane layout around edits; verify text bytes, selection/caret/scroll, Undo/Redo, modified state, save/reopen, canceled navigation and recovery. |
| Accessibility | Keyboard-only use, VoiceOver labels/roles/selected states, contrast, focus return from pop-ups, no keyboard traps, reduced motion. |
| Adjacent regressions | Right-click New Tab/New Window, external-change protection, search scope/history, narrow Export and native file/print/share dialogs. |

Retain meaningful existing regressions, especially writing appearance/state preservation, navigation/search independence, sidebar hide/restore, presentation commands and Visual Edit focus protection. Add behavior tests for the new state model rather than tests that merely assert implementation constants. Run the repository-required build and test suite after code changes, followed by native checks of affected workflows.

This UI pass is done when Andrew accepts the complete workspace and a compact layout, all shown controls are discoverable and functional, layout changes preserve documents, required native/automated checks pass, and the approved installed and test bundles can be identified as the same source revision. Record any residual limitation explicitly. Full general-purpose visual editing, total Ulysses feature parity and public notarized distribution remain separate milestones.

## Decisions ready for review

Approved direction: Studio + Editorial for the design reference; blue folders; round toolbar actions and capsule groups; independent Organizer/Files/Preview controls; a calm writing surface; temporary navigation at constrained widths. Preserve stored theme and writing choices.

Andrew's next review is the integrated native composition and wide → narrow → wide behavior, followed by writing, keyboard and multiwindow checks. Acceptance and replacement of each runnable bundle must be recorded against the actual build identity.
