# Development guide

## First session

Run `./bin/build`, then `./bin/run examples/Welcome.md`. Edit and save a disposable copy. Run `./bin/test` before committing a change to editor behaviour. `./bin/package-mac` creates `dist/Omawrite.app` with Qt bundled.

The initial Homebrew-based bundle declares macOS 14 as its minimum. This checkout was built on Apple Silicon/macOS 26.6.2; compatibility with macOS 14 and Intel is not yet tested. Apple's Command Line Tools are sufficient for this qmake build; full Xcode is optional.

## Where to change things

`src/Main.qml` owns the window, editor, search, menus and save/discard/cancel flow. `src/EditorMutations.js` contains insertion and formatting operations. `src/backend.cpp` handles disk I/O, atomic saves, file watching and recovery. `src/markdownhighlighter.cpp` renders Markdown through text formatting. `src/systemtheme_mac.cpp` reads macOS appearance through Qt. `src/main.cpp` wires these together and receives Finder file-open events.

Keep content as plain Markdown. Changes to visual formatting must not silently alter the saved source. When changing document switching, preserve the unsaved-change confirmation path.

## Verification

Automated Qt tests exercise word counting, links, filenames, Markdown ranges, theme parsing, file watching, insertion behaviour, QML loading, footer actions and text scaling. The test runner uses an offscreen Qt platform; macOS application services still need to be accessible.

For a manual smoke test:

1. Open `examples/Welcome.md`, save a copy and reopen it.
2. Try Command-B/I, paste, undo and redo.
3. Use Find and Find/Replace, including Cancel and empty matches.
4. Open another document while modified; verify Cancel, Discard and Save separately.
5. Close while modified; verify cancelling keeps the window.
6. Open a file from Finder using Open With; test cold launch and an existing window.
7. Change system appearance; check readability and selection contrast.
8. Launch `dist/Omawrite.app` after packaging, checking that menus and file pickers work.

Each new document feature should add a meaningful regression test for the behaviour it can break. Prioritise data preservation over broad visual snapshots.

## Git workflow

Work in this repository and commit small increments. Fetch upstream updates with `git fetch upstream`, inspect the difference, then merge selected changes deliberately. The initial fork preserves upstream's `master` branch and history. Personal application settings use a separate organisation namespace so the fork does not share recovery state with upstream Omawrite.
