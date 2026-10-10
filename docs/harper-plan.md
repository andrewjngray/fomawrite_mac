# Harper in Fomawrite — plan

Decision (Andrew, 10 October 2026): build Harper into the tool as the grammar and style engine, with a way to keep pulling new Harper releases down periodically and a menu item that triggers an update check. This note is the plan to agree before building.

## What Harper is, checked on 10 October 2026

- **Automattic/harper**, Apache-2.0. An offline English grammar checker written in Rust. Releases about every two weeks: v2.11.0 on 16 September and v2.12.0 on 1 October 2026, each with macOS Apple-silicon binaries (`harper-ls`, `harper-cli`) and a desktop app.
- **harper.js** on npm, latest 2.10.0 (10 September 2026): the same engine compiled to WebAssembly, with `WorkerLinter` (runs in a Web Worker, recommended for interactive apps) and `LocalLinter`. Exports the WASM as `binary`, `slimBinary` and inlined variants. The API is marked "not yet stable". The package unpacks to about 75 MB because it carries several binary variants; the app would ship one.
- **API that matters to us:** `new WorkerLinter({ binary, dialect })`, `setup()`, `lint(text)` → `Lint[]` with a span, message, kind and suggestions; `applySuggestion`; `ignoreLint` / `exportIgnoredLints` / `importIgnoredLints` (privacy-respecting hashes); `importWords` / `exportWords` for a custom dictionary; `getLintConfig` / `setLintConfig` to turn rules on and off; dialects American, British, Australian, Canadian.
- **Rules and dictionary live inside the engine.** There is no separate rules feed. "Pulling the rules down" therefore means fetching a newer harper.js release, verifying it, and loading it in place of the bundled one.

## Where it runs

Two ways to host the engine. The recommendation is the first.

1. **harper.js in the editor page's Chromium, as a Web Worker.** The Live page already exists in the app; it gains a worker that lints on request. The Source editor gets its findings the same way, over the bridge: the host sends a block's prose, the page answers with findings. Offline, sandboxed, no Gatekeeper, no extra process, one engine for both surfaces. Cost: the page must be created at startup (hidden until Live is used) so Source has an engine from the first keystroke, and the WASM (several megabytes) adds to startup and memory.
2. **harper-ls as a sidecar process** spoken to over the language server protocol from C++. No page dependency, but a downloaded unsigned binary that macOS may refuse to run after an update, a second process to supervise, and the LSP's document model to bridge.

## How it fits what is already built

The pipeline from Cycles 143 and 144 stays: each surface marks what is prose, an engine returns findings with offsets, categories, messages and suggestions, the surfaces underline them, the Review pane lists them, right-click offers the suggestions, Ignore and Learn. Harper plugs in behind `SpellCheck` as a second engine:

- **Engine choice:** Harper when its bundle is loaded, the macOS checker otherwise (and still for the spelling of languages Harper does not cover, which is everything but English). A setting under Edit → Writing Checker: Harper / macOS.
- **Categories:** Harper's lint kinds map to Spelling (red), Grammar (blue) and a new Style (grey) for readability, word choice, repetition and formatting. The pane filter gains Style.
- **Dialect:** a setting, default Australian or British (to confirm), shown where the pane's language popup is today.
- **Learn and Ignore:** Learn adds to Harper's custom words and to the macOS dictionary; Ignore uses Harper's hashed ignore list, persisted in the app's data folder, so an ignored finding stays ignored across runs.
- **Rules:** a Rules page in the Review pane listing Harper's rules with on/off switches, persisted.

## Keeping Harper current

- **Pinned bundle in the build.** `harper.lock.json` names the version and npm integrity hash; `bin/fetch-harper` downloads that tarball, verifies the hash, and places the needed files in the app's Resources (git-ignored, fetched in CI like `npm ci`).
- **Updates at runtime.** A menu item **Help → Check for Writing Checker Updates…** and a weekly automatic check (a setting to turn it off). The check reads the npm registry's latest version; if newer, it downloads the tarball to the app's data folder, verifies the published sha512, unpacks the files the page needs, and loads the new engine on the next document open, with a notice ("Harper 2.11.0 installed; was 2.10.0"). A failed verification deletes the download and says so. Nothing in the page or the host executes outside Chromium's sandbox.
- **Rollback:** the bundled version is always kept; a bad update can be discarded from the same menu.

## Phases

1. **Spike, half a day.** Load harper.js in the editor page from a local copy, lint three of Andrew's real documents and the test sentences, record findings, timing and WASM size, and compare with what Grammarly shows for the same text. Deliverable: a note with the comparison. Decides whether Harper's quality is enough to be the default.
2. **Cycle 145, engine.** The engine seam, the worker in the page, Source over the bridge, categories and colours, dialect, Learn and Ignore, the pinned bundle and the CI fetch. Installed build.
3. **Cycle 146, updates and rules.** The menu item, the weekly check, verified download and hot swap, rollback, the Rules page.

## Questions for Andrew

1. Dialect: Australian, British, or American English as the default?
2. Keep the macOS checker selectable as an engine, or fallback only?
3. Where should the update item live: Help menu (with About) or Edit → Writing Checker?
4. Three real documents for the spike, ideally ones you have run through Grammarly, with screenshots of what it flagged?
5. Is a Style category (grey underline) welcome in the document, or should style findings stay in the pane only?

## Sources

- Harper repository and releases: https://github.com/Automattic/harper/releases
- harper.js on npm: https://www.npmjs.com/package/harper.js
- harper.js linting API: https://writewithharper.com/docs/harperjs/linting
- harper.js from a CDN (binary option): https://writewithharper.com/docs/harperjs/CDN
