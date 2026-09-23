# Cycle 57: bundled offline Help and Window Center (partial)

Help now includes Omawrite Help and What’s New in Omawrite while retaining Keyboard Shortcuts. The two pages are Markdown bundled inside the application and opened through a fixed allowlist in a read-only, scrollable in-app view. Close and Escape dismiss the view. Unknown or missing page IDs show a safe local error rather than reading an arbitrary path.

The help content describes current local Markdown behavior, safety boundaries and recent features. It does not contain private paths and does not use the network. No Online Support item was added because its destination was not verified, and Omawrite does not add a second OS Help search.

Phase 57b adds Window → Center after Zoom. It is enabled for normal windows and invokes AppKit’s native Center command without adding Zoom All, Fill, tiling, display movement or a dynamic document list. Automated two-window coverage checks available-screen centering, unchanged window sizes/other-window geometry and unchanged dirty document, modified and Undo state.

The combined 57b/58a `./bin/build` and full native-access `./bin/test` suite pass with **99 tests, zero failures and zero skips**. The stable `dist/Omawrite Dev.app` was refreshed with that source. Native accessibility inspection previously verified the Help menu, both bundled pages, Close and Escape; the Templates fixture’s source and status stayed unchanged. Native 57b QA showed Window → Center visible and enabled, and invoked it on synthetic Target.md without changing its clean status. An app-only live capture showed the target source and preview, but the crop did not establish on-screen centering and no screenshot was saved. See [`native-help.txt`](native-help.txt) and [`native-center.txt`](native-center.txt).

Help dark/narrow layout, pixel styling, link/scroll behavior, VoiceOver and OS Help search remain unchecked. Center’s exact OS geometry and multi-display behavior remain unverified. Online Support, Zoom All, Fill and the remaining Window/application-menu AppKit work remain open. [Optional usability exercise](../usability/cycle-57.md).
