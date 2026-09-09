# Omawrite Mac

Andrew Gray's personal Markdown editor, forked from [omacom/omawrite](https://github.com/omacom/omawrite). Built with Qt Quick, QML and C++.

The aim is a calm, extensible reading and writing tool inspired by the parts of iA Writer and Typora we use daily. This is an early macOS port of Omawrite, not a feature-complete replacement for either product.

## Build and run on macOS

Install Apple's Command Line Tools (`xcode-select --install`) and Homebrew, then:

```sh
brew install qtbase qtdeclarative qttools
./bin/build
./bin/run examples/Welcome.md
./bin/test
```

A full `brew install qt` also works. Build scripts discover Homebrew Qt on Apple Silicon or Intel; set `QMAKE=/path/to/qmake` to use another Qt 6 installation. The initial build is tested on Apple Silicon with Qt 6.11.2. Other Macs and older macOS versions still need validation.

To create an app with its Qt libraries included:

```sh
./bin/package-mac
open dist/Omawrite.app
```

The package is locally ad-hoc signed, not notarised for public distribution. You can copy it to Applications for personal use. Build outputs are ignored by Git.

## What works in the foundation

- Local Markdown editing with styled headings, emphasis and links.
- Open, Save, Save As, print and word count.
- Find/replace, undo/redo and Markdown formatting shortcuts.
- Recovery snapshots and warnings for external file changes inherited from Omawrite.
- macOS menus, native file pickers, Finder Open With support and system light/dark appearance.

On macOS use **Command-S/O/N/P/F/B/I/K**, **Shift-Command-S** for Save As, **Option-Command-F** for replace, **Command-W** to close, and **Control-Command-F** for full screen. Qt maps its internal `Ctrl` shortcut notation to Command on macOS.

## Project map

- `src/`: QML interface, C++ document backend, Markdown highlighter and platform appearance code.
- `tests/`: Qt tests for editor operations and document behaviour.
- `macos/`: application bundle metadata and Markdown file association.
- `bin/`: build, test, run and packaging commands.
- `examples/`: documents for trying features.
- `docs/`: [project direction](docs/roadmap.md), [interview summary](docs/interview-summary.md) and [development guide](docs/development.md).

`origin` points to [andrewjngray/omawrite_mac](https://github.com/andrewjngray/omawrite_mac); `upstream` points to the original Omawrite repository. Keep upstream updates separate from personal changes and review before merging.

## Attribution

Omawrite is copyright David Heinemeier Hansson, under the [MIT license](LICENSE), retained here. Bundled iA Writer Mono fonts are copyright Information Architects Inc., based on IBM Plex, and distributed under the [SIL Open Font License](fonts/OFL.txt).
