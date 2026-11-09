# Open Markdown from Codex in Fomawrite

macOS builds include a browser-readable dark icon at
`Fomawrite.app/Contents/Resources/FomawriteRunningIcon.png`. It is the same
artwork used while Fomawrite runs. The Finder `.icns` icon stays separate.

After installing the app at `/Applications/Fomawrite.app`, add this table to
`~/.codex/config.toml` (or replace the existing Fomawrite handler table):

```toml
[desktop.custom_file_handlers.fomawrite]
label = "Fomawrite"
command = "/usr/bin/open"
args = ["-a", "/Applications/Fomawrite.app"]
input = "path"
icon = "/Applications/Fomawrite.app/Contents/Resources/FomawriteRunningIcon.png"
```

Fully quit and reopen Codex, then open a local Markdown file and choose
**Open → Fomawrite**. Custom handlers appear after the built-in apps.
This configuration was verified in Codex 26.1103.60933; it is an app-specific
setting and may change in later Codex versions.

Use a PNG file path. The `.icns` path showed a broken image in the dropdown.
The final standalone PNG configuration was visually confirmed by Andrew on
9 November 2026. An earlier embedded data URL was not confirmed working.

The build, packaging and installation scripts do not edit Codex preferences.
An existing standalone icon under `~/.codex/icons/` can remain in use; installing
a newer Fomawrite build does not require changing that working configuration.

## Build verification

```sh
./bin/build
./bin/check-codex-icon
# The same check accepts packaged, Dev or installed bundles:
./bin/check-codex-icon "dist/Fomawrite.app"
```

The check requires an externally readable file with bytes equal to the dark
runtime PNG. CI runs it immediately after building the macOS app. Packaging,
Dev preparation and installation copy that file along with the bundle.
