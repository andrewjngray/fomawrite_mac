# Cycle 116 — visual table navigation and integrated handoff

Date: 4 October 2026. Review target: **0.3.0-dev12 / macOS 0.3.0 (116)**.

Status: complete and verified in the stable Dev, ordinary/demo and Applications copies. This is a local review build, not a public distribution or complete reference-app parity claim.

## Included work

- Cycle113: Source formatting ownership across toolbar/menu/shortcut routes; auxiliary fields and rendered writing cannot mutate an old Source selection. Typewriter explicitly identifies its Source-only scope.
- Cycle114: consistent Insert/Edit link dialog with optional title, safe cancellation, exact-session validation, retained entries after refusal and canonical Undo/Redo.
- Cycle115: six additional acceptance groups inside the actual executable, including continuous resizing and narrow export access.
- Cycle116: Tab/Shift-Tab between supported Visual Edit table cells, including empty cells. Navigation leaves Markdown and Undo untouched; existing mapped typing edits only the intended cell.

At table boundaries, unsupported rows, cross-cell selections and stale projections, navigation refuses unsafe movement. It does not insert rows, change column counts or cross into a separate table. F6 leaves the writing surface. Complex table/list/image work still has an explicit Source route.

## Verification and artifacts

- Production build and packaging pass. Full regression suite: **173 passed, 0 failed, 0 skipped**.
- Focused native Cocoa suite: **13 passed, 0 failed, 0 skipped**, comprising 11 workflow slots plus setup/cleanup.
- Dev `dist/Fomawrite Dev.app`, ordinary/demo `dist/Fomawrite.app` and installed `/Applications/Fomawrite.app`: each passes **124 footer states, seven daily-writing workflows, five pane-zoom workflows and six additional acceptance groups**.
- All three report **zero QML warnings**, matching tested executable/resource identities and valid strict signatures for build116/dev12. [Verified build identities](verified-builds.json).
- The bundled-QML F6 regression passes actual shortcut dispatch offscreen and checks native focus routing without OS activation. The separate native-window smoke passes **79 assertions**, then stops because macOS refuses app activation. This does not certify native WindowShortcut dispatch or the later full-screen/inactive/Drawer/tab-detach and print/share workflows; [scope and diagnosis](../cycle-115/README.md#wider-native-window-check-activation-blocked).

Detailed reports, logs and synthetic screenshots remain local under the cycle research directories and are ignored. Commit only concise reviewed summaries and approved synthetic examples. Existing private writing and unrelated modified samples are outside this work.

## Limits and next work

Verification delivers synthetic Qt events in isolated native app windows. Next priorities:

1. Review normal composition, real keyboard/trackpad, IME and F6 traversal in an active app. Complete the blocked full-screen/inactive/Drawer/tab-detach and print/share scenarios, plus VoiceOver/display scaling and arbitrary restored workspaces.
2. Choose one structural improvement from that review: table row/column restructuring, image placement or richer safe lists. Keep exact Markdown and Undo/Redo acceptance for each.
3. Continue the historical parser/export/interoperability and multi-window state matrix separately. Typewriter remains Source-only; general WYSIWYG is not claimed.

No notarized/public app release is included.

[Optional writing exercise](../usability/cycle-116.md). After Andrew’s review, choose the next small writing inconvenience and verify exact Markdown, Unicode, Undo/Redo and narrow/dark behavior before broadening the supported structures.
