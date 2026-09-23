# Cycle 42 — Mac 0.2.0 RC1 archive

Final build and 70 tests pass. Stable Dev and ordinary packaged bundles share candidate source. Version: application 0.2.0-rc1, bundle 0.2.0/build 42. Apple Silicon, minimum macOS 14, local ad-hoc signature. MIT and bundled font licenses retained.

`bin/archive-mac-release mac-v0.2.0-rc1` verifies committed build inputs, builds/packages with a running-app guard, creates a ZIP, build/source manifest and SHA-256 checksums. Generated binaries remain outside Git; GitHub prerelease assets provide the binary archive. Repeatable procedure does not imply byte-identical signatures/timestamps.

This is a release-candidate checkpoint, not completed iA parity. docs/release-acceptance.md lists all unresolved product and hardware acceptance. Existing personal/example edits were preserved and excluded from staging.
