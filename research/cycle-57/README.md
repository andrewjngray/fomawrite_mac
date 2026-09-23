# Cycle 57a: bundled offline Help (partial)

Help now includes Omawrite Help and What’s New in Omawrite while retaining Keyboard Shortcuts. The two pages are Markdown bundled inside the application and opened through a fixed allowlist in a read-only, scrollable in-app view. Close and Escape dismiss the view. Unknown or missing page IDs show a safe local error rather than reading an arbitrary path.

The help content describes current local Markdown behavior, safety boundaries and recent features. It does not contain private paths and does not use the network. No Online Support item was added because its destination was not verified, and Omawrite does not add a second OS Help search.

`./bin/build` and the full native-access `./bin/test` suite pass with **97 tests, zero failures and zero skips**. The stable `dist/Omawrite Dev.app` was refreshed through 57a. Native accessibility inspection verified the Help menu, both bundled pages, Close and Escape. The synthetic `research/cycle-47/sample/Templates.md` fixture’s source and status stayed unchanged. [`sample/Help.md`](sample/Help.md) is available for the optional usability exercise. Screenshot capture was unavailable. See [`native-help.txt`](native-help.txt).

Dark/narrow layout, pixel styling, link and scroll behavior, VoiceOver and OS Help search remain unchecked. Online Support and the remaining Window/application-menu AppKit work remain open. [Optional usability exercise](../usability/cycle-57.md).
