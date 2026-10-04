# Acceptance ledger — current follow-through and Cycle 59 history

## Current review build — Cycle 121

**0.3.0-dev17 / macOS 0.3.0 (121)** exposes consistent Fade In/Out and Always Show choices. [Verification and limits](../research/cycle-121/README.md).

## Previous review build — Cycle 120

Build **0.3.0-dev16 / macOS 0.3.0 (120)** refines persistent edge reveal and Studio pane tones. [Cycle120 handoff and verification](../research/cycle-120/README.md). Historical acceptance gaps below remain open.

## Previous review build — Cycle 119

Build **0.3.0-dev15 / macOS 0.3.0 (119)** puts editing controls under the left editor and publishing controls under the read-only right output pane. Web and PDF reuse the export renderer. Auto-hide document bars preserve geometry and recover through edge hover, menus and keyboard access. Dev, demo and Applications each pass **149 actual-executable checks**, matching UI/version, strict signatures and zero QML warnings. The full regression run passed 181 cases with one stale label expectation; the corrected final focused rerun passed **8/8**. No known failure remains. See the [Cycle119 handoff](../research/cycle-119/README.md) and [verified identities](../research/cycle-119/verified-builds.json). Prior physical input, accessibility and parser/export limitations remain open.

## Previous verified review build — Cycle 118

Build **0.3.0-dev14 / macOS 0.3.0 (118)** separates editing mode from document layout. Source / Visual Edit chooses the editable surface; Single / Split chooses whether read-only output Preview is alongside it. The two footer groups remain fixed, and changing either choice preserves the other. Preview-only is explicit reading mode; its unselected editing/layout controls return to the retained editor and editing layout.

Implementation is complete. **178 regressions pass, 0 failed, 0 skipped**, including setup/cleanup. Dev, ordinary/demo and Applications each pass **147 actual-executable checks**, matching embedded resources/version, valid strict signatures and zero QML warnings. All three copies are refreshed to build118. **12 focused Cocoa checks also pass, 0 failed, 0 skipped**, including setup/cleanup. [Verified build identities](../research/cycle-118/verified-builds.json). Cycle117 remains the previous verified baseline below. [Cycle118 status](../research/cycle-118/README.md), [review exercise](../research/usability/cycle-118.md). The completed gate covers all four combinations, repeated switching/divider movement, readable compact Aa labels, inactive editing focus, hidden-Source clipboard/completion refusal, Find transitions, source selection and draft/Undo, separate editor/Preview zoom, version1/version2 state migration and read-only Preview behavior. These gates do not close the older physical/native acceptance gaps.

## Previous verified review build — Cycle 117

Build **0.3.0-dev13 / macOS 0.3.0 (117)** implements Andrew’s pane-footer, Preview identity and compact library feedback. Source appearance belongs to Source, the template picker belongs to Preview, and Visual Edit centres/clamps within the rendered pane; only Source / Split / Full retain fixed right-hand positions. Preview headers show the actual filename and Edited state. Library rows show document icons, actual filenames, short grey dates and compact grey excerpts with real tree indentation.

All **176 regressions and 12 focused native checks pass, zero failures/skips**; the native count includes setup/cleanup. Dev, ordinary/demo and Applications each pass 124 footer states, seven daily-writing workflows, five zoom workflows, six acceptance groups and three pane-chrome groups, with zero QML warnings. All three copies are refreshed to build117, with matching resource/version identities and valid strict signatures. [Build identities and gate results](../research/cycle-117/verified-builds.json). [Cycle117 status](../research/cycle-117/README.md), [review exercise](../research/usability/cycle-117.md). This UI work does not close the prior native foreground-activation blocker or the physical input/IME, VoiceOver, display, multi-window and parser/export gaps below.

## Previous verified baseline — Cycles 98–116

Previous verified local review build: **0.3.0-dev12 / macOS 0.3.0 (116)**. Cycles113–116 add Source formatting ownership, safe inline-link editing, wider actual-executable writing acceptance and keyboard navigation within supported visual table cells. Dev, ordinary/demo and Applications each pass 124 footer states, seven daily-writing workflows, five pane-zoom workflows and six new acceptance groups. All three pass embedded-resource/version comparisons and strict signatures with zero QML warnings. All 173 regressions and 13 focused native checks (11 workflows plus setup/cleanup) pass with no failures/skips; final identities and limits are in the [Cycle116 handoff](../research/cycle-116/README.md). This gate covers affected workflows, not the complete historical menu matrix.

| Area | Current bounded evidence | Still open |
| --- | --- | --- |
| Formatting focus | Shared guard across Source toolbar, shortcuts, native Format, transformations and insertions; positive/negative field-focus regressions and actual-executable focus checks pass. | Physical IME and keyboard-only usability; broader Edit-menu state matrix and VoiceOver. |
| Inline links | Focused syntax/dialog regressions and actual-bundle insert/edit/cancel/Undo and narrow/dark stale-session groups pass. | Complex/nested/reference/image link editing and full Markdown parser parity. |
| Visual editing | Supported table Tab/Shift-Tab navigation preserves source/Undo and refuses boundaries/stale selections. Earlier exact cell/inline edits, image alt text and simple-list Return splitting remain included. | Row/column restructuring, spatial image placement, arbitrary multiline paste, protected inline/nested list edits and general WYSIWYG. |
| Workspace/native acceptance | Actual-executable repeated views, zoom, continuous resize and narrow export groups pass in all three review copies. Separate native-window smoke passes 79 assertions covering migration, window/tab routing, Unicode heading navigation, independent checkpoints and resize preservation. | macOS refused foreground activation, so current native F6, later full-screen/inactive/Drawer/tab-detach and print/share scenarios are not passed. Physical VoiceOver, multiple displays/scaling and arbitrary-workspace relaunch remain open. |
| Navigation and panes | Earlier Cycle100 cross-window fragment support and Cycle102 direct pane/template access remain implemented; current smoke verifies live Unicode heading transfer, missing-heading feedback and dirty-source Cancel. | Broader parser/output and cross-application clipboard behavior; exhaustive native state restoration. |
| Distribution | Local ad-hoc review packaging is separate from acceptance. | Developer ID signing, notarization, another-machine installation and an explicit public-release decision. |

The next priority is Andrew’s normal composition and active-app keyboard review, followed by one bounded structural editing improvement selected from that feedback. Typewriter is Source-only. Earlier Cycle98/100 native passes remain historical evidence; the current activation-blocked run does not recertify their later keyboard/full-screen/print/share scenarios. [Optional review exercise](../research/usability/cycle-116.md). Cycle101 remains optional distribution work.

## Historical Cycle 59 checkpoint and subsequent September follow-ups

**Cycle 60 product identity:** Current source and local bundles use Fomawrite/Fomawrite Dev. Historical Cycle 59 evidence and the iA parity gaps below remain as recorded; the rename is not a parity claim. See [rename notes](product-rename.md).

**24 September follow-up — Cycle 50b:** Edit → Transformations now has all four captured labels and a directly observed Capitalize subset. The current `./bin/build` and full native-access `./bin/test` pass **102/0/0**. Refreshed Dev verified `tEST of THE wORLD` → `Test Of The World` and first Undo on a disposable draft; ordinary and Dev bundles were refreshed and strict-signature verified. Cycle 50c subsequently separated Make Title Case and matched two observed iA samples, with the same 102/0/0 suite; unobserved title-style rules remain open. [Capitalize record](../research/cycle-50/native-capitalize.txt), [Title Case record](../research/cycle-50/native-title-case.txt).

**24 September follow-up — Cycles 54b–54c:** Independent opt-in Smart Quotes and Smart Dashes subsets passed `./bin/build` and **101 native-access tests, zero failures and zero skips** at that checkpoint. Refreshed ordinary and Dev bundles passed strict signature verification. Native Dev direct typing and first Undo verified both transformations. The ordinary app reopened a clean saved document. A UI bridge timeout initially left the final Dev disposable-draft close state unverified; Phase 50b later found clean Second.md before replacing Dev. This narrows the Cycle 54 gap below; it does not change the Cycle 59 implemented-subset classification. See [Cycle 54](../research/cycle-54/README.md).

24 September 2026. This is an **implemented-subset integrated checkpoint**, not a declaration of complete iA Writer parity or a final menu closeout. The tested product code is commit `5f8d64b` on macOS 27.0.

## Cycle 59 artifact and automated gate

- `./bin/build` passed and the full native-access `./bin/test` suite passed with **99 tests, zero failures and zero skips**. Logs: [build](../research/cycle-59/build.log) and [tests](../research/cycle-59/test.log).
- `./bin/package-mac` succeeded. Both `dist/Omawrite.app` and `dist/Omawrite Dev.app` were refreshed and passed strict local signature verification.
- Both bundles are local ad-hoc signed artifacts. They are not Developer ID signed or notarized. The GitHub Mac 0.2.0 RC1 remains the older Cycle 42 downloadable artifact.
- The running ordinary app was quit normally after its open `README.md` status was saved. The refreshed ordinary bundle reopened that saved file and exposed the Authors menu. Dev showed the synthetic `research/cycle-55/sample/Fillers.md` fixture with clean status before the disposable safety check, then returned to saved Second.md.

## Integrated native evidence

Native checks across Cycles 48–58 inspected every top-level menu family: application, File, Edit, Format, Authors, View, Focus, Go, Window and Help. Representative evidence includes guarded file/edit flows, menu hierarchy, date/navigation/preview choices, fixture statistics, bounded tags and completions, Authors setup, Focus groups, live Fillers highlighting, bundled Help pages, local fragment navigation and Window → Center.

The latest checks visibly confirmed yellow Fillers matches and exclusions without changing status, opened both offline Help routes, showed the source caret at the duplicate `# Same` heading after Go → Open Link, and invoked Window → Center without changing the target document status. No screenshot file was persisted. These checks do not constitute a complete saved/untitled/dirty state matrix for all ten menus.

A scoped dirty-close check used disposable `/private/tmp/omawrite-cycle59-native.BqTAbc/Safety.md`, initial SHA-256 `e1b1f555bc821da7942d3c43ea63945f6c118a5bab48ae8ff33e9d30365db2c2`. After typing ` Scratch edit.`, File → Close offered Cancel, Discard and Save. Cancel retained the starred title, dirty text and Unsaved status; a later Close → Discard closed only the disposable draft and returned to saved Second.md. The final disk hash was identical. No user writing was edited or discarded.

## Historical implemented subsets and gaps at that checkpoint

| Cycle / area | Explicit remaining behavior |
|---|---|
| 54 — writing input | Smart Quotes and the observed spaced-prose Smart Dashes rule have bounded opt-in implementations. Text Replacement, Smart Copy/Paste, automatic correction and the remaining Substitutions behavior remain unimplemented. Completion popup arrow/Escape/IME and broader accessibility acceptance remain open. |
| 55 — Focus/style | Clichés, Redundancies and parts-of-speech Show Syntax are absent. Hide Authors, full Custom visual/overlap acceptance and focus/typewriter feel remain open. |
| 56 — Authors | Post-setup iA semantics remain unknown. There is no iA-style Mark As or Paste Edits From workflow, reusable author registry, automatic assignment or verified provenance. |
| 57 — platform menus | Window Zoom All, Fill, Move & Resize, Full Screen Tile, explicit move-to-display, window sets and verified dynamic window list remain open. Application menu destinations/enabled states, OS Help search and Online Support remain incomplete or unverified. Center geometry is automated offscreen only. |
| 58 — navigation/output | Cross-window fragment transfer and missing-anchor feedback were open at this checkpoint; Cycle 100 narrows these two gaps as recorded above. Broader Markdown parser, preview/output parity and cross-application clipboard behavior remain unestablished. |

## Acceptance still required

| Area | Still required |
|---|---|
| State matrix | Systematic saved, untitled and dirty-document checks for all applicable menu actions, including enabled/disabled transitions. |
| Accessibility and appearance | VoiceOver, high contrast, keyboard-only traversal, dark and narrow layouts, fullscreen and exact fade/hover behavior. |
| Hardware and OS geometry | Multi-display Center/move behavior, attach/detach, minimized/zoom restoration, disk-full/device removal, separate-volume moves and OS shutdown. |
| Interoperability | Cross-application formatted/HTML/Markdown clipboard, sharing targets, physical printing and other applications stripping authorship metadata. |
| Scale and grammar | Large real-library indexing/watch behavior, complete nested Markdown/content-block grammar and semantic rather than proportional scroll alignment. |
| Distribution | Developer ID signing, notarization and another-machine installation. |

The local apps at that checkpoint were useful integrated artifacts for the implemented subset. Exact iA Writer parity, complete ten-menu acceptance and hardware certification remain open.
