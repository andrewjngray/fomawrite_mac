# Cycle 7 — UI matching, paused at Andrew's request

## Intent
Compare the actual open iA Writer window and refine Omawrite until layout, typography, spacing and control placement are substantially closer. Retain Andrew's navy accent preference. Do not claim matching menu labels are matching features.

## Work in progress
- Read iA's native UI and observed organizer ~190px, file list ~260px, editor/preview balanced in a 1160px capture.
- Added wider navigation panes, equal writing/preview proportions, pale-gray editor, 16px default source font and 14px preview, lower content top inset.
- Simplified top controls; moved Split/Full choices to preview footer and formatting into editor footer.
- Added file dates, row separators, document/folder icons and left-aligned organizer rows. File secondary text currently says Markdown document; actual excerpts are NOT implemented.
- Added macOS-only title hiding helper and expanded client area to remove the duplicate title/toolbar treatment.
- Font migration changes only previous default 20 to 16, leaving custom sizes intact. Actual user preferences currently include 22px source and paragraph focus ON, so UI comparison must account for these rather than silently assuming 16px.

## Iterations and checks
1. Initial revision caught a QML delegate property error; corrected it. Font-scaling test updated for intended default. All 20 tests then passed (logs/tests.log).
2. Native screenshot (screenshots/iteration-1.png) revealed overlapping title chrome and content. Reworked toolbar into explicit content layout and added native title hiding. Second build succeeded (logs/build.log), runtime log `/tmp/c7b-ui.log` empty. This second revision HAS NOT been visually verified or regression-tested after the chrome changes.

Current reference: screenshots/ia-reference.png contains only the shared sample with library hidden. iA library was hidden for this capture and has not yet been restored. No user writing was edited.

## Runnable state
Uncommitted working changes. Last committed/package checkpoint is Cycle 6, d946174. `dist/Omawrite.app` is still Cycle 6; do not describe it as Cycle 7.
Latest development build: `build/Omawrite.app`.
Second QA copy: `/tmp/Omawrite-Cycle7b.app` with unique bundle identifier `io.github.andrewjngray.omawrite.cycle7b`; launched with examples/Preview-check.md.
First QA copy `/tmp/Omawrite-Cycle7a.app` is still open. Older user windows, including unsaved ones, were not closed.

## Resume next
- Inspect second QA window (confirm runtime log first), raise window before screenshot, then compare actual 16px/no-focus view against iA sample using app controls as needed.
- Verify toolbar alignment, native close/minimize/fullscreen/drag, preview switching, find and formatting/undo. Fix any remaining overlap.
- Preserve persistent error discoverability: current draft replaces status footer with a timed tooltip and filename accessibility description; reconsider this before shipping.
- Consider real bounded/lazy file excerpts and richer preview styling; these are only ideas, not implemented.
- Run build/tests again after final changes, package/sign, save safe screenshots and short usability checklist, update inventory/docs and then commit/push.
- Restore iA library visibility via native UI after safe captures.

Paused on Andrew's instruction so he can sleep the computer. No background continuation or automation was scheduled.

## Resumed and completed — 11 September

The paused notes above are a historical checkpoint. Final changes also include real one-line file excerpts (read on demand from at most 1 KB; capped at 160 characters), a persistent compact status line, accessible Save/Open actions in the footer's More menu, and larger preview headings. The final toolbar uses zero padding to correct its vertical placement. Existing custom writing sizes remain preserved; the comparison used Reset text size (16px) and paragraph focus off.

Final checks: `./bin/build`, `./bin/test` (22 passed), `./bin/package-mac`, and `codesign --verify --deep --strict dist/Omawrite.app` succeeded. Two added regression tests cover bounded excerpts/source preservation and font/focus changes not marking a saved document modified. Footer menu open/save actions are covered by the updated UI test. The final native runtime log is empty. The sample file has no Git changes.

Native review verified corrected title/content placement, readable typography and excerpts, library hide/show, Split/Full preview switching and opening Find from the toolbar. iA's library was confirmed visible again. Older unsaved windows were preserved. An intermediate QA window had recovered unsaved sample content; it was not saved or discarded. The final QA window opened the sample cleanly.

Evidence: `screenshots/final-four-panes.png` is the final build; `split-review.png` is the immediately preceding heading-size/padding revision; `ia-reference.png` contains the same sample in iA, with private library details hidden. Different window sizes affect screenshot scaling, so these are visual review records, not a pixel-diff parity claim.

Runnable artifact is now **dist/Omawrite.app (Cycle 7)**. Final QA copy `/tmp/Omawrite-Cycle7-final.app` is open for inspection. Source and bundled app now match the final revision.

Remaining differences: Qt preview still has different table borders, code/quote styling, heading separators and task-list marks from iA's GitHub template. Hashtags, sync scrolling, full formatting toolbar and native tabs remain future work. Dark appearance, narrow-window control fit, and native drag/minimize/fullscreen/close interactions were not exhaustively retested. Excerpts are lightweight source snippets and refresh when library entries are refreshed; they are not a full Markdown rendering/index.

UI automation suffered several multi-minute stalls and a final approximately 106-minute attachment stall despite the requested timeout. This substantially delayed native verification; builds/tests themselves completed normally.
