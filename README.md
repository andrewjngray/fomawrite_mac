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

## Writing workspace

- Resizable file library, Markdown editor and live rendered preview; Editor/Split/Preview layouts.
- Organizer with saved locations, favorites and recent files; expandable folders, sorting, filtering and safe new-file/new-folder creation.
- Visible Markdown syntax or styled editing, paragraph focus and centered typewriter scrolling.
- Adjustable writing size and Sans/Serif/Mono preview typography.
- Rendered tables, task lists, local images and relative links to Markdown documents.
- Heading outline navigation and reading statistics.
- Local Markdown editing with styled headings, emphasis and links.
- Open, Save, Save As, print and word count.
- Find/replace, undo/redo and Markdown formatting shortcuts.
- Recovery snapshots and warnings for external file changes inherited from Omawrite.
- macOS menus, native file pickers, Finder Open With support and system light/dark appearance.

On macOS use **Command-S/O/N/P/F/B/I/K**, **Shift-Command-S** for Save As, **Option-Command-F** for replace, **Command-W** to close, and **Control-Command-F** for full screen. Qt maps its internal `Ctrl` shortcut notation to Command on macOS.

Try `./bin/run examples/Workspace-tour.md`. Use the sidebar toggles to show/hide the library and organizer, the outline icon to jump to headings, and **Aa** for writing options, and the word count for statistics. Files remain ordinary UTF-8 Markdown. The library filter currently covers files in expanded folders. Preview uses Qt Markdown; it does not yet offer iA Writer's complete extensions, web/PDF template system or publishing integrations.

## Latest build: closeout implementation pass, Cycles 20b–28

Command palette and sentence focus; bounded saved-content/tag searches and saved queries; preview extensions/TOC/scroll synchronization; HTML/PDF export and basic output styles; shared-process windows/native tabs; opt-in autosave and saved-file versions; selection spelling/analysis and manual authorship annotations.

Build and all 48 tests pass. Both bundles refreshed. Representative native workflows and a basic PDF export were checked. **Full parity/release closeout is still pending**: the [closeout audit](docs/closeout-audit.md) distinguishes working subsets from unfinished templates, services, authorship workflows and QA. Read the [new-feature guide](docs/new-feature-guide.md) before using the optional autosave/annotation features on important writing.

## Build cycles and feedback

See [the cycle log and short usability exercises](docs/build-cycles.md) for each working checkpoint. The [iA Writer menu inventory](docs/ia-writer-inventory.md) records observed features and remaining gaps. Tell Codex what to keep, change or add; the next cycle will incorporate that feedback.

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


### Cycle 29 source checkpoint

Authorship metadata now follows Duplicate, Rename and Move; current verification is 51 passing tests. See [Cycle 29](research/cycle-29/README.md), the [remaining closeout work](docs/closeout-audit.md), and [GitHub snapshot scope](docs/github-snapshot.md). The project remains an early personal editor, with the original parity plan still open.
