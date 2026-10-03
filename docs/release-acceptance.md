# Acceptance ledger — current follow-through and Cycle 59 history

## Current follow-through — Cycles 98–104

The current review target is application `0.3.0-dev5`, macOS `0.3.0 (104)`. Cycle 104 refines the folder heading and formatting controls, and adds quick text sizing to the native workspace implemented in Cycle 103. See [Cycle 104](../research/cycle-104/README.md) for evidence. This records the affected workflows; it does not recertify the full historical menu matrix.

| Area | New bounded evidence | Still open |
| --- | --- | --- |
| Workspace/accessibility | Cycle 98 native fixture passes F6/reverse traversal, named Qt accessible button/text interfaces, compact Drawer Escape/focus, inactive toolbar focus, full screen, tab detach/regroup and independent pane checkpoints. | Physical VoiceOver listening/reading order, actual display scaling/multiple displays, arbitrary-workspace quit/relaunch, complete control-state/contrast inventory. |
| Visual editing | Cycle 99 source implements simple table-cell and inline-code edits, image alt text with local thumbnails, supported list continuation/task reset/empty-item exit, grapheme-safe replacements and caret tracking. All 134 tests and native Visual Edit checks pass. | General WYSIWYG, arbitrary multiline paste, splitting within a list item, complex/nested structures, spatial image placement and a full table grid. |
| Cycle 58 navigation | Cycle 100 transfers fragments to another open window/tab and resolves its live unsaved heading; missing/invalid headings give feedback. Native Unicode/dirty-target/source-Cancel/Undo checks pass within the 111-assertion integration run. Revision-guarded preview refresh prevents stale missing-heading feedback. | Broader parser/output parity and cross-application clipboard behavior. |
| Pane/template controls | Cycle 102 restores hidden panes directly from the surviving header, orders and names View toggles consistently, and synchronizes the Preview dropdown with native template selection. Pointer, compact, draft/Undo and native checks are in its handoff. | Andrew’s everyday usage and physical VoiceOver review. |
| Distribution | Local review packaging remains separate from product acceptance. | Developer ID signing, notarization, another-machine installation and an explicit public-release decision. |

The next product cycle is review-led image/table/list ergonomics after the current UI review, with remaining human accessibility/display acceptance alongside it. Cycle 101 is an optional distribution track.

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
