# Cycle 157 — Browser-readable Fomawrite icon for Codex

9 November 2026. Andrew confirmed the correct dark icon in Codex after its
custom handler used a standalone PNG, and requested that the build carry the
fix and that it be pushed to Git.

## Change

macOS qmake bundles now include `Contents/Resources/FomawriteRunningIcon.png`
as an ordinary file, in addition to the internal Qt resource. Codex can point
to it without extracting Qt resources or attempting to display `.icns`.
The Finder icon and the running app artwork are unchanged. Packaging, Dev
preparation and installation copy the PNG with the bundle.

`docs/codex-open-with.md` records the working handler table, the restart step
and the distinction between the external PNG and the Finder icon. Builds do
not edit Codex preferences. Andrew's already-working standalone icon remains
in use; no installed app is replaced while running.

## Verification

- `bin/check-codex-icon` failed on the previous build because the external PNG
  was absent, then passed after the qmake fix and rebuilding. It compares the
  bundled file byte-for-byte with the dark runtime PNG, rejecting a missing
  file or the light Finder artwork. CI runs this check after its macOS build.
- `./bin/build` passed.
- Full regression-suite and packaging results are recorded in STATUS.md.

## Limits

The screenshot Andrew supplied verifies his working local Codex configuration.
The documented app-bundle path uses the same PNG bytes, but was not separately
selected in his live Codex menu. The installed app and its saved documents are
untouched. No physical editor/UI acceptance rerun is warranted for this
packaging-only change. CI completion is tracked separately in STATUS.md.
