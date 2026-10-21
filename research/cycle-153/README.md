# Cycle 153 — Stability and verification

22 October 2026. Review build `0.3.0-dev49` / macOS `0.3.0 (153)`.

## Scope

Exercise the actual macOS app, fix observed failures, enforce the editor checks in CI, reconcile the status notes, and synchronize the three local app bundles. No new product feature or public release.

## Findings and changes

| Finding | Resolution |
| --- | --- |
| First Live use could advance the source document revision without changing a byte. The native footer matrix recorded one revision change and 120 consequent failures. | Harper readiness switches the active checker. `MarkdownHighlighter::refreshSpelling()` called Qt's full `rehighlight()`, which increments the document revision. It now refreshes the spelling/grammar layout marks through the existing revision-neutral restyling path. |
| Native Claude Like checks still expected the old cream/Georgia design after the approved 11 October sans-serif/near-white change. | Updated explicit appearance expectations while retaining document semantics, pagination, margins, actual-pane paint, and theme-switch assertions. The old serif theme remains in `docs/themes/claude-serif.css`. |
| A theme reload probe dereferenced an absent heading during navigation. | Its DOM read now tolerates a temporarily missing heading and keeps waiting for the expected color. |
| The native suite switched into Live but did not type there. | Two required groups send Qt key events into the real WebEngine editor and check its canonical mirror, Source/Live switching, exact UTF-8 save/reopen, and Undo/Redo. They verify actual page caret and focus before input. |
| Bundle checks covered QML but not the embedded editor bundle or built-in themes. | Added hashes for the editor HTML/CSS/JS and both built-in theme stylesheets; the wrapper requires them. |
| CI omitted the editor's separate tests and build. | An independent pinned-Node job runs lockfile install, pinned Harper download, 321 editor tests, typecheck, build, and committed-asset freshness validation. |
| Status notes listed shipped features and rejected experiments as outstanding. | Reconciled the status, roadmap introduction, Live friction list, spelling decision, and CI architecture description. |

## Regression evidence

`sourceSpellingRefreshDoesNotAdvanceDocumentRevision` fails on the old implementation: expected revision 3, actual 4 after a spelling refresh. It passes with the layout-only fix, including a checker-engine change. Logs are local in this cycle's evidence folder.

A broader Source/Live transition test passed the old code because the delayed checker signal was not deterministic there; it was removed rather than presented as regression evidence. The existing native matrix provides integration coverage for the observed timing.

## Verification

- `./bin/build`, `./bin/package-mac`, `./bin/prepare-dev-app`, `./bin/install-mac`: passed.
- Full Qt suite: **299 passed, 0 failed, 0 skipped**. Editor: **321 passed**, typecheck passed and committed assets reproduced byte-for-byte.
- Full `bin/check-document-views` against **Dev, distribution release and installed release**: all passed, no QML warnings. Each exercised **124 footer states, 5 daily-writing, 5 pane-zoom, 7 editor, 3 pane-chrome, 5 editing-layout and 12 publishing-theme checks**, including 8 preview-paint checks. Current document identity and actual Web/PDF pane paint remain required.
- The two new native Live groups passed in all three bundles: keyboard typing, canonical/page agreement, Source/Live switching, exact UTF-8 save/reopen and Undo/Redo.
- All three bundles report build **153** and version **0.3.0-dev49**, pass signature verification and share all **25 checked embedded resource hashes**. Distribution and installed release executables are byte-identical; Dev has a separate signed identity. See [verified-builds.json](verified-builds.json).
- `bin/check-publishing-startup`: **cold and restored passed**, with two document windows, matching preview contents, visible PDF page paint and normal quit. This harness compiles a separate instrumented binary from the same source and uses disposable persisted settings; it is distinct from the three packaged-app checks.
- Visually reviewed the native Web theme and PDF document-B captures. No wrong-document or blank-pane regression reproduced in these runs. Transient Qt timer/compositor diagnostics occurred without failed paint/content checks; this is not a claim of warning-free stderr.
- Initial baseline: 298 Qt tests and 321 editor tests passed, but native acceptance failed 124 assertions (120 consequences of one revision change, plus 4 stale theme assertions). That baseline was not accepted as a pass.

The first green CI run exposed one avoidable skip: the editor job lacked the pinned Harper WASM fixture. The job now downloads and requires that fixture before testing. The GitHub macOS runner skips 12 native grammar-service tests because its OS has no grammar pass for the test language; all 12 passed locally.

Logs, raw reports and captures are retained locally in `research/cycle-153/evidence/` (ignored by Git). CI results are tied to the pushed commit in [GitHub Actions](https://github.com/andrewjngray/fomawrite_mac/actions/workflows/ci.yml); the new editor job must pass alongside the macOS job before handoff.

## Limits

Native acceptance uses synthetic Qt pointer and keyboard events in exposed Cocoa app windows with disposable files/settings. It is not a human physical-input or VoiceOver/multi-display certification. No private writing is used or committed. Public GitHub releases and the separate website PR are outside this cycle.
