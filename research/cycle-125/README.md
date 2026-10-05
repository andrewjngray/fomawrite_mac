# Cycle 125 — publishing themes from the folder

Verified review build **0.3.0-dev21 / macOS 0.3.0 (125)**. Dev, ordinary/demo and Applications are refreshed. [Verified build identities](verified-builds.json).

## Scope and changes

Publishing themes now appear automatically when CSS files are added to the themes folder. Top-level `.css` files support normal names, including capitals, spaces, underscores and uppercase extensions. Resource folders belong beside their CSS files. CSS inside nested archive folders is not automatically cataloged. A folder theme named `claude-like.css` has a distinct entry from the bundled Claude Like theme.

Folder selection and Import Theme resolve local stylesheet imports and embed supported images and fonts for portable Web/PDF output. Common Typora hexadecimal glyph escapes in quoted strings and harmless comments are supported. Missing optional font files use fallback families with a visible advisory. Typora export-only directives are omitted with an advisory; remote fonts and assets are never downloaded. Import remains useful for bringing a theme into this same folder as a standalone CSS copy.

A debounced watcher refreshes the chooser when files are added, changed or removed. Changes to selected CSS or its local resources also refresh the preview, including atomic saves. Failure and advisory messages remain visible. The selected theme is preserved if another theme fails to load.

The publishing menu shows one checked selection and uses a bounded scrolling list for larger theme folders. Basic Settings and Custom Settings are separate from CSS theme selection, with Edit Basic Settings retaining the existing font and page controls. Publishing appearance remains independent of Source/Visual Edit and document text/history.

## Verification status

- A standalone QtCore probe read every existing top-level theme without modifying the source themes: **13 loaded, 0 failed**. It covers local font assets, dark-theme imports, glyph escapes and export-directive notices. This verifies loading, not rendered parity with Typora.
- `./bin/build` passes. Final executable hashes, embedded UI/version and strict signatures match the verified records for all three app copies.
- **195 regressions pass, 0 failed, 0 skipped.** Two new slots cover folder discovery/resources/import portability, unsafe-resource rejection, external changes and persistent failure feedback. Focus and narrow-menu regressions also pass.
- **157 native checks pass per app copy**, with zero QML warnings. New groups exercise actual repeated/rejected menu clicks, sole checkmarks in native/footer menus, rendered CSS colours, automatic edits/reload, basic selection, CSS import, valid/invalid JSON and the settings editor. Picker completion uses accepted callbacks; physical Finder/picker interaction is not automated. Narrow popup bounds are checked by the regression suite.
- Review locations: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app`, `/Applications/Fomawrite.app`. The previous installed build is retained in a temporary backup. Nonfatal Qt timer and Chromium compositor teardown diagnostics remain in local logs; these are separate from the zero QML-warning count.

## Compatibility and acceptance limits

Themes apply to the semantic document elements supported by Fomawrite. Typora editor/UI selectors may have no matching element. Network resources, SVG assets, symlink resources, parent-folder traversal, arbitrary CSS escapes, advanced import layer/supports modifiers and image/image-set syntax remain unsupported and report failures. Optional missing fonts can change typography and pagination.

Bounds remain explicit: 64 resources, eight import levels, 1 MiB per imported stylesheet, 4 MiB per raster image, 32 MiB per font, 64 MiB combined input resources and 128 MiB expanded CSS. Large embedded fonts can increase preview/export cost; general large-document performance is not established by the loading probe.

Physical printing, VoiceOver, physical input/IME, display scaling, broad Markdown/CSS compatibility and the historical native activation gaps remain open. Private writing, theme contents and screenshots are excluded from this record.

[Optional user review](../usability/cycle-125.md).
