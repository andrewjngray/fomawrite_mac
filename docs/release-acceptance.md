# Mac 0.2.0 RC1 acceptance ledger

23 September 2026. This is a personal-use release candidate, not a declaration of full iA Writer parity. Cycles 39–42 have a build/checkpoint; the outstanding acceptance criteria below remain open. Earlier cycle notes are historical snapshots; this ledger is the current status.

## Verified for this candidate

- Build and 70 automated tests pass. Tests cover UTF-8/undo/recovery, saved-file conflicts, version restore, clipboard annotations, source/preview operations, bounded library scans and command routing.
- Native spelling suggestions found one prose typo and excluded its fenced-code twin; Replace and Undo passed. Grammar checkbox completed a check. English dictionary suggestions automated; all installed languages are offered, but not all are acceptance-tested.
- Palette launches Spelling and Grammar and Paginated Preview. Existing two-tab Command-S remains verified in the preceding checkpoint; document shortcuts are scoped to the active window.
- Native paginated preview rendered both pages and exposed fit-page/fit-width, orientation and page-navigation controls. Custom PDF has three inspected pages: title page, page 2 header/image, page 3 header/body; counters are correct. Raster HTML embedding/failed-export preservation remain tested.
- Native fullscreen Quit/relaunch restored both tabs in fullscreen. Display selection by saved name and geometry clamping are implemented; off-screen placement is unit-tested.
- Long-document footnote forward jump reached the end (preview scroll 100%). Qt dropped empty anchors; generated-link fallback fixes this. Forward and both repeated return positions are regression-tested on the real rendered document. Native return-click result was inconclusive and is not counted as passed.
- Dark preview links use the theme focus color and underline. Source editor has a useful accessible name. Existing tests verify filter bounds/focus at widths 1100, 900 and 720. A native resize drag did not resize the window; it is not evidence of minimum-width acceptance.
- Tag cap test: 2,001 tags yields 2,000 entries and an explicit limit status; oversized file is reported skipped. Measured synthetic scan: approximately 50 ms on this host, not a general large-library benchmark.
- Speech Start/Stop commands invoked on sample text; audible quality was not independently verified.
- Build scripts refuse to replace running ordinary/Dev bundles. Release archive is made from committed compiled inputs, includes source commit/build instructions/checksums, and preserves MIT/font licenses. Bundles are ad-hoc signed.

## Remaining acceptance work

| Area | Still required |
|---|---|
| Safety / hardware | Physical disk-full/device removal, separate-volume move, external-display attach/detach and OS shutdown. Existing simulated failure/crash checks do not substitute for these. |
| Window / accessibility | Native minimized/zoom restoration matrix, minimum-width keyboard traversal, VoiceOver/high-contrast review and multiple displays. Fullscreen representative check passed. |
| Clipboard / annotations | Cross-application interoperability, immediate row-focus retest and full live attribution visualization. Manual labels are not verified provenance; other applications can strip metadata. |
| Markdown | Native return-click check, cross-file fragment matrix, complete nested Markdown grammar, nearest-match wiki search and semantic scroll alignment. Current synchronization remains proportional. |
| Writing tools | All-language grammar accuracy, audible speech, automatic substitutions/correction while typing, completions and full live syntax/style highlighting. Current review is explicit and bounded. |
| Menus / interaction | Exact hover timing under rapid transitions, remaining reference submenu/context-menu audit, toolbar/title fading modes and all platform tab-action enabled states. Path tooltips now use per-control two-second timers; stopwatch/native timing not certified. |
| Library | Incremental filesystem watching and performance across a representative large real library; the current explicit scan has documented bounds. |
| Distribution | Developer ID signing/notarization and another-machine install. RC is Apple Silicon/macOS 14+, locally ad-hoc signed. |

No zero-remaining-cycles claim is made. The usable implementation has advanced through the release-candidate checkpoint; outstanding feature scope and acceptance need follow-through before a final parity sign-off. Physical checks require suitable hardware and user-visible OS actions; notarization requires distribution credentials.
