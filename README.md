# Fomawrite

Fomawrite is a calm, local-file Markdown editor for macOS, built with Qt Quick/QML and C++. It grew from the MIT-licensed [Omawrite project](https://github.com/omacom/omawrite); that history and attribution remain in this repository. The writing files stay plain UTF-8 Markdown.

The current app supports a resizable library and organizer, live preview, native menus and file dialogs, Markdown formatting, local links, search, themes, focus tools, export/print, recovery and unsaved-change protection. The [iA Writer menu audit](docs/ia-menu-audit-2026-09-23.md) distinguishes working behavior from partial support and unverified features. Fomawrite is **not yet a full iA Writer or Typora replacement**.

## Build and run

On macOS, install Apple's Command Line Tools and Homebrew Qt (`brew install qtbase qtdeclarative qttools`), then run:

```sh
./bin/build
./bin/test
./bin/run examples/Welcome.md
```

`QMAKE=/path/to/qmake` selects another Qt 6 installation. To make the two local app bundles:

```sh
./bin/package-mac
./bin/prepare-dev-app
open dist/Fomawrite.app
```

`dist/Fomawrite.app` is the ordinary app. `dist/Fomawrite Dev.app` is the stable QA app; both come from the same source, but can differ until refreshed. The final short-connector writing icon is generated from `macos/FomawriteIcon.svg`; run `./bin/make-app-icon` (requires QtSvg and `iconutil`) to rebuild its committed `.icns` and runtime PNG. They use bundle IDs `io.github.andrewjngray.fomawrite` and `io.github.andrewjngray.fomawrite.dev`. Bundles are locally ad-hoc signed, not notarized for public distribution. Do not replace either while it is running; close it normally and preserve unsaved work first.

The first launch of a renamed Mac bundle migrates prior preferences and copies matching workspace/recovery state from the matching former Omawrite identity, leaving the old files in place. Existing hidden `.omawrite-authors.json` sidecars and clipboard metadata remain compatible so a product rename does not discard authorship annotations. [Migration details](docs/product-rename.md).

## Project and status

Source lives in `src/` (QML interface, C++ document I/O and formatting), `macos/` (bundle metadata), `bin/` (build and packaging), and `tests/` (Qt checks). The [roadmap](docs/roadmap.md), [cycle log](docs/build-cycles.md) and [acceptance ledger](docs/release-acceptance.md) track current work and explicit gaps. Cycle 60 is the Fomawrite identity migration; the prior menu work remains an implemented subset.

The project repository is [andrewjngray/fomawrite_mac](https://github.com/andrewjngray/fomawrite_mac). `upstream` remains the original Omawrite repository. The earlier Mac 0.2.0 RC1 download uses the old name and predates the later menu cycles; do not present it as this Fomawrite build.

## Attribution

The upstream Omawrite code is copyright David Heinemeier Hansson and remains under the [MIT license](LICENSE). Bundled iA Writer Mono fonts are copyright Information Architects Inc., based on IBM Plex, and remain under the [SIL Open Font License](fonts/OFL.txt).
