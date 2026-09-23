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

## Latest fully verified build: Cycle 51; Cycles 52–58a have partial native QA

File and folder right-click menus include open/tab/window, info, favorites, duplicate/rename/Trash, Finder/sharing, output/clipboard, creation, sorting and view options. View → Template shares named presets across preview and export; Show Date offers Date Modified, Date Created and None. Format adds ordered tasks plus conservative Clear Styles. File/Edit follow the captured menu groups, and View offers Tree or current-folder List navigation and continuous/paginated preview routes. Cycle 52 source adds statistics and View chrome modes; Cycle 53 source adds Go access to saved locations/queries/tags and bounded tag refresh; Cycle 54a source adds explicit local word completions. Cycle 55 groups Focus controls and adds opt-in live Custom and Fillers style checks. Cycle 56a adds a local Authors setup profile without assigning authorship labels. Cycle 57 adds bundled offline Help plus native Window → Center; Cycle 58a adds local Markdown/text heading-fragment navigation for inline links, wikilinks, Go → Open Link and preview links. **99 tests pass**. `dist/Omawrite.app` is packaged from Cycle 53 source; `dist/Omawrite Dev.app` is refreshed with the combined Cycle 57b/58a source, with partial native menu/statistics/tag/completion/Focus/Authors/Help/fragment/Center checks. Center geometry remains automated and multi-display behavior is unverified. Fragment support is navigation, not complete Markdown parser or output parity. Online Support and remaining Window/AppKit work remain open. See the [build records](docs/build-cycles.md) and [menu closeout plan](docs/menu-closeout-plan.md). The GitHub RC1 download remains the earlier Cycle 42 binary.

## Archived release: Mac 0.2.0 RC1, Cycle 42

Native spelling suggestions/grammar review and speech; code-aware writing analysis; expanded command palette; paginated preview with fit controls; improved dark links; fullscreen/display restoration and long-footnote navigation fixes. Earlier checkpoints include themes, safe history/recovery, authorship clipboard, portable output and organizer queries/tags.

**Build and 70 tests pass**, with representative native QA and inspected PDF output. Both stable bundles are refreshed. The versioned GitHub prerelease includes the Apple Silicon app ZIP, source/build manifest and SHA-256 checksums; binaries remain excluded from Git history. This is a personal-use release candidate, not full iA parity or a notarized public release. See the [current acceptance ledger](docs/release-acceptance.md), [user guide](docs/new-feature-guide.md) and [build records](docs/build-cycles.md).

## Build cycles and feedback

See [the cycle log and short usability exercises](docs/build-cycles.md) for each working checkpoint. The [current screenshot-backed iA menu audit](docs/ia-menu-audit-2026-09-23.md) maps observed menus to Omawrite's implemented, partial and missing behavior. The [sequential closeout plan](docs/menu-closeout-plan.md) starts at Cycle 48; the [reference inventory](docs/ia-writer-inventory.md) retains earlier inspection history. Tell Codex what to keep, change or add; the next cycle will incorporate that feedback.

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

### Cycle 32 checkpoint

Saved documents now restore their windows, native tab order, cursor and library root after Quit/relaunch. Repeated launches of the same installed app forward requests to its existing process. Build and 56 tests pass; native sample restart/duplicate-open checks passed. See [Cycle 32 evidence](research/cycle-32/README.md) and [the ten remaining cycles](docs/remaining-cycles.md), with whole-workspace themes next.

### Cycles 33 / 34a checkpoint

Aa → Theme now offers Follow system, Light, Dark and Warm paper across the workspace and preview. Recovery preserves the pre-crash disk baseline for safe autosave checks. Quit waits for every document's approval before closing windows, so a later Cancel retains earlier drafts. Build and 59 tests pass; native themes/restart and two-draft Quit/Cancel verified. [Nine planned cycles remain](docs/remaining-cycles.md), including the unfinished safety/version checks in Cycle 34.

### Cycle 34b checkpoint

Restoring a saved version now pauses autosave until explicit Save and clears unverified historical authorship; one Undo restores the previous text and labels. Build and 61 tests pass, including native file-version and failed-save checks. Native Restore/Undo verified. Cycle 34 remains open for its remaining safety/history and display checks.


### Cycles 34c–38 checkpoint

Added opt-in previous-save history, authorship-aware Markdown clipboard and metadata export, richer footnotes/local content blocks, persisted paginated output with portable HTML images, and organizer saved queries/browsable tags. Build and 66 tests pass; representative native sample checks and rendered PDF review completed. Four untouched feature/release cycles remain (39–42), plus documented acceptance gaps in 34–38. See [the remaining plan](docs/remaining-cycles.md).
