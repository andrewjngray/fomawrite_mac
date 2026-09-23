# Cycle 59 integrated acceptance ledger

**24 September follow-up — Cycle 54b:** A later opt-in Smart Quotes increment passes `./bin/build` and **100 native-access tests, zero failures and zero skips**. Refreshed ordinary and Dev bundles pass strict signature verification. Native Dev typing produced curly quotes, Command-Z restored the final straight quote, disabling the option left new input straight, and the disposable draft was discarded. This narrows the Cycle 54 gap below; it does not change the Cycle 59 implemented-subset classification. See [Cycle 54](../research/cycle-54/README.md).

24 September 2026. This is an **implemented-subset integrated checkpoint**, not a declaration of complete iA Writer parity or a final menu closeout. The tested product code is commit `5f8d64b` on macOS 27.0.

## Current artifact and automated gate

- `./bin/build` passed and the full native-access `./bin/test` suite passed with **99 tests, zero failures and zero skips**. Logs: [build](../research/cycle-59/build.log) and [tests](../research/cycle-59/test.log).
- `./bin/package-mac` succeeded. Both `dist/Omawrite.app` and `dist/Omawrite Dev.app` were refreshed and passed strict local signature verification.
- Both bundles are local ad-hoc signed artifacts. They are not Developer ID signed or notarized. The GitHub Mac 0.2.0 RC1 remains the older Cycle 42 downloadable artifact.
- The running ordinary app was quit normally after its open `README.md` status was saved. The refreshed ordinary bundle reopened that saved file and exposed the Authors menu. Dev showed the synthetic `research/cycle-55/sample/Fillers.md` fixture with clean status before the disposable safety check, then returned to saved Second.md.

## Integrated native evidence

Native checks across Cycles 48–58 inspected every top-level menu family: application, File, Edit, Format, Authors, View, Focus, Go, Window and Help. Representative evidence includes guarded file/edit flows, menu hierarchy, date/navigation/preview choices, fixture statistics, bounded tags and completions, Authors setup, Focus groups, live Fillers highlighting, bundled Help pages, local fragment navigation and Window → Center.

The latest checks visibly confirmed yellow Fillers matches and exclusions without changing status, opened both offline Help routes, showed the source caret at the duplicate `# Same` heading after Go → Open Link, and invoked Window → Center without changing the target document status. No screenshot file was persisted. These checks do not constitute a complete saved/untitled/dirty state matrix for all ten menus.

A scoped dirty-close check used disposable `/private/tmp/omawrite-cycle59-native.BqTAbc/Safety.md`, initial SHA-256 `e1b1f555bc821da7942d3c43ea63945f6c118a5bab48ae8ff33e9d30365db2c2`. After typing ` Scratch edit.`, File → Close offered Cancel, Discard and Save. Cancel retained the starred title, dirty text and Unsaved status; a later Close → Discard closed only the disposable draft and returned to saved Second.md. The final disk hash was identical. No user writing was edited or discarded.

## Implemented subsets that remain partial

| Cycle / area | Explicit remaining behavior |
|---|---|
| 54 — writing input | Smart Quotes has a bounded opt-in implementation. Smart Dashes, Text Replacement, Smart Copy/Paste, automatic correction and the remaining Substitutions behavior remain unimplemented. Completion popup arrow/Escape/IME and broader accessibility acceptance remain open. |
| 55 — Focus/style | Clichés, Redundancies and parts-of-speech Show Syntax are absent. Hide Authors, full Custom visual/overlap acceptance and focus/typewriter feel remain open. |
| 56 — Authors | Post-setup iA semantics remain unknown. There is no iA-style Mark As or Paste Edits From workflow, reusable author registry, automatic assignment or verified provenance. |
| 57 — platform menus | Window Zoom All, Fill, Move & Resize, Full Screen Tile, explicit move-to-display, window sets and verified dynamic window list remain open. Application menu destinations/enabled states, OS Help search and Online Support remain incomplete or unverified. Center geometry is automated offscreen only. |
| 58 — navigation/output | Cross-window fragment transfer and missing-anchor feedback remain open. Broader Markdown parser, preview/output parity and cross-application clipboard behavior are not established. |

## Acceptance still required

| Area | Still required |
|---|---|
| State matrix | Systematic saved, untitled and dirty-document checks for all applicable menu actions, including enabled/disabled transitions. |
| Accessibility and appearance | VoiceOver, high contrast, keyboard-only traversal, dark and narrow layouts, fullscreen and exact fade/hover behavior. |
| Hardware and OS geometry | Multi-display Center/move behavior, attach/detach, minimized/zoom restoration, disk-full/device removal, separate-volume moves and OS shutdown. |
| Interoperability | Cross-application formatted/HTML/Markdown clipboard, sharing targets, physical printing and other applications stripping authorship metadata. |
| Scale and grammar | Large real-library indexing/watch behavior, complete nested Markdown/content-block grammar and semantic rather than proportional scroll alignment. |
| Distribution | Developer ID signing, notarization and another-machine installation. |

The current local apps are useful integrated artifacts for the implemented subset. Exact iA Writer parity, complete ten-menu acceptance and hardware certification remain open.
