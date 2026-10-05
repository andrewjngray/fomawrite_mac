# Fomawrite

Fomawrite is a calm, local-file Markdown editor for macOS, built with Qt Quick/QML and C++. It grew from the MIT-licensed [Omawrite project](https://github.com/omacom/omawrite); that history and attribution remain in this repository. The writing files stay plain UTF-8 Markdown.

The current app supports a resizable library and organizer, live preview, native menus and file dialogs, Markdown formatting, local links, search, themes, focus tools, export/print, recovery and unsaved-change protection. The [iA Writer menu audit](docs/ia-menu-audit-2026-09-23.md) distinguishes working behavior from partial support and unverified features. Fomawrite is **not yet a full iA Writer or Typora replacement**.

## Build and run

On macOS, install Apple's Command Line Tools and Homebrew Qt (`brew install qtbase qtdeclarative qttools qtwebengine`), then run:

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

`dist/Fomawrite.app` is the ordinary app. `dist/Fomawrite Dev.app` is the stable QA app; both come from the same source, but can differ until refreshed. The approved light icon is generated from `macos/FomawriteIcon.svg`; `macos/FomawriteRunningIcon.svg` supplies the first dark concept while the macOS app runs. Run `./bin/make-app-icon` (requires QtSvg and `iconutil`) to rebuild the committed light `.icns` and both runtime PNGs. They use bundle IDs `io.github.andrewjngray.fomawrite` and `io.github.andrewjngray.fomawrite.dev`. Bundles are locally ad-hoc signed, not notarized for public distribution. Do not replace either while it is running; close it normally and preserve unsaved work first.

To verify the UI inside a specific local bundle, using disposable documents and settings:

```sh
./bin/check-document-views research/footer-review --app "dist/Fomawrite Dev.app"
```

This runs native Qt pointer-event checks, captures the app's window and compares embedded QML/version metadata with the checkout. It does not use macOS accessibility automation or touch the normal workspace. The report identifies the tested executable. Use **Fomawrite → About Fomawrite** to identify a normally running copy.

The first launch of a renamed Mac bundle migrates prior preferences and copies matching workspace/recovery state from the matching former Omawrite identity, leaving the old files in place. Existing hidden `.omawrite-authors.json` sidecars and clipboard metadata remain compatible so a product rename does not discard authorship annotations. [Migration details](docs/product-rename.md).

## Project and status

Source lives in `src/` (QML interface, C++ document I/O and formatting), `macos/` (bundle metadata), `bin/` (build and packaging), and `tests/` (Qt checks). The [roadmap](docs/roadmap.md), [cycle log](docs/build-cycles.md) and [acceptance ledger](docs/release-acceptance.md) track current work and explicit gaps. Cycle 60 is the Fomawrite identity migration; the prior menu work remains an implemented subset.

The project repository is [andrewjngray/fomawrite_mac](https://github.com/andrewjngray/fomawrite_mac). `upstream` remains the original Omawrite repository. The earlier Mac 0.2.0 RC1 download uses the old name and predates the later menu cycles; do not present it as this Fomawrite build.

## Attribution

The upstream Omawrite code is copyright David Heinemeier Hansson and remains under the [MIT license](LICENSE). Bundled iA Writer Mono fonts are copyright Information Architects Inc., based on IBM Plex, and remain under the [SIL Open Font License](fonts/OFL.txt).

## Current workspace review build

Cycle **123** targets **0.3.0-dev19**, macOS **0.3.0 (123)**. Build verification and refreshed local copies are **PENDING**. The left pane owns **Source / Visual Edit** and writing appearance. The right pane is read-only publishing output with **Web / PDF** and its output style. **Single / Split** remains a separate group at the far right. Web renders the exported HTML; PDF displays the actual exported pages, including paper settings and page furniture.

Document bars hide while typing or scrolling and return at the corresponding edge. Each revealed bar stays visible after the pointer leaves, until the next edit or scroll, without changing document geometry. Studio pairs a soft grey editing surface with the warm cream publishing surround. Keyboard access and open menus keep controls reachable. Choose **View → Title Bar → Fade In/Out** or **Always Show** for both document bars. The **Auto-Hide Document Bars** checkbox reflects the same preference. Verification status and local review paths are in the [Cycle123 handoff](research/cycle-123/README.md).

Source offers Manuscript, Editorial, Book and **Code** appearances. Code uses Menlo, syntax colours for supported file/fence languages, line numbers, indentation guides and horizontal scrolling, with Tab/Shift-Tab and indented Return. Manuscript hangs heading markers beside the aligned body column; fenced code has a continuous background, including empty lines. Appearance changes preserve plain Markdown. Code remains a lightweight editor appearance without language-server, autocomplete, linting or debugging features.

Visual Edit preserves canonical Markdown and keeps explicit Source fallback for complex/nested structures and unsupported multiline operations. Physical input, VoiceOver, display testing and the earlier native foreground-activation limitation remain open. [The workspace plan](docs/workspace-ui-redesign-plan.md) records the design direction and remaining acceptance work.
