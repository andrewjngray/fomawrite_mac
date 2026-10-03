# Cycle 97 — workspace redesign candidate

3 October 2026. Andrew approved the complete workspace direction in the [workspace UI plan](../../docs/workspace-ui-redesign-plan.md). Cycles 90–96 are integrated in this candidate; broad acceptance remains pending.

Candidate identity: app `0.3.0-dev1`, macOS bundle `0.3.0`, build `97`. This record is committed with the source used by the candidate. Bundle identities and executable hashes are recorded in [artifacts.json](artifacts.json).

## Implemented scope

- Cycle 90: approved composition, pane ownership, blue folders and fit rules.
- Cycle 91: semantic palette roles, round toolbar actions, capsule groups and navigation/control states.
- Cycle 92: aligned pane-owned headers, guarded document history and grouped document/workspace actions.
- Cycle 93: independent desired/effective pane state, remembered user widths, ordered contraction, 48-pixel restoration margin, temporary navigation and per-window checkpoint persistence.
- Cycle 94: existing writing appearances refined, explicit editing/rendering modes, reserved statistics control, 150% source line height and removal of duplicate permanent formatting actions.
- Cycle 95: organizer hierarchy/contextual removal and consistent compact/preview file rows with blue folder identity.
- Cycle 96: shared control treatment across menus, search actions, outline, preview and dialog buttons; Export's band scrolling and fixed actions retained.

Existing theme and writing preferences take precedence. New installations use Studio and Editorial; existing users can select **Studio writing layout** explicitly. Markdown remains the canonical UTF-8 source; Visual Edit still has bounded supported edits. Private writing and pre-existing dirty sample documents are not cycle fixtures and remain untouched.

## Verification record

The final source passed the full suite and native workspace checks. Native captures were inspected and two visual defects (header safe-area duplication and truncated preview controls) were corrected. An independent review found the Full/Visual Edit mode regression; Full and Split now preserve editing mode, with a regression check.

| Check | Current evidence/status |
| --- | --- |
| Production build | Passed `./bin/build`; [build log](build.log). Incremental builds now explicitly regenerate Info.plist so version updates do not leave old bundle metadata. |
| Full regression suite | `./bin/test`: **122 passed, 0 failed, 0 skipped**; [test log](tests.log). Export fixture later explicitly sets its initial window size; focused recheck passed. |
| Layout behavior | Passed contraction/restoration, desired widths, independent visibility, active-surface fallback, malformed state and separate-instance checks. |
| QML/native workspace interactions | Native workspace/header checks passed at 1440,1120,900,720 and back. Real Export grip/divider/horizontal-scroll drags pass. Compact drawer cancellation and narrow Find pass; [isolated native check](compact-native.log). |
| Editing preservation | Appearance regression preserves source/cursor/undo/output style and saves/reopens exact UTF-8. Native resize preserves selected text and dirty draft; actual Undo restores exact source. Source/Visual Edit/Full/Split checks pass. |
| Window/native tabs | Production-main native fixture passes New Window, New Tab, ordinary-open reuse, per-window/tab independence, checkpoint save/restore, and canceling all three print dialogs. Finder/share dispatch accepted without sending or printing; [log](native-workspace.log). Detach/full screen/multi-display remain acceptance checks. |
| Visual evidence | Inspected [Editorial](screenshots/editorial.png), [compact](screenshots/workspace-compact.png), [split](screenshots/workspace-wide.png), [dark](screenshots/workspace-dark.png) and Book/narrow writing variants. These are synthetic app-only renders; no private writing. |
| Bundle identity and local signing | Dev and package rebuilt; Applications refreshed while closed, with previous copy backed up under `/private/tmp`. All three pass strict/deep ad-hoc signature verification. Package and Applications executable hashes match; Dev linkage/signature intentionally differ. See artifacts.json. |

The review target is the refreshed `dist/Fomawrite Dev.app`, bundle ID `io.github.andrewjngray.fomawrite.dev`. The ordinary `dist/Fomawrite.app` and `/Applications/Fomawrite.app` have the same implementation. The old public RC1 download remains unchanged. To apply the reference composition, choose **Aa → Studio writing layout**.

Native test note: the combined control run caught an extra `g` character during a frontmost disposable draft check (see controls-native.log); the unchanged isolated compact check passed, as did the offscreen regression. The export fixture initially inherited a narrow native window size; explicitly setting its wide starting size fixed that test setup. These observations are retained rather than presented as an uninterrupted all-green native run.

## Remaining acceptance and next scopes

Andrew's writing and complete-composition review, keyboard/VoiceOver, actual display scaling, inactive/dark states, native title-bar/full-screen behavior and broad tab/window restoration remain acceptance work. General editable image cards, arbitrary list/table structures, full parser/export parity and public signing/notarization are not claimed by this candidate. Earlier Cycle 88 and [acceptance-ledger](../../docs/release-acceptance.md) gaps remain open unless narrowed by specific evidence.

Recommended separate increments: Cycle 98 user feedback/accessibility/scale/tabs; Cycle 99 safe visual blocks and image/list/table projection; Cycle 100 selected workflow/parser/export gaps; optional Cycle 101 distribution. These recommendations do not promise completion of the entire backlog.

[Optional usability exercise](../usability/cycle-97.md).
