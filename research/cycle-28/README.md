# Cycle 28 — Release/parity audit — still open

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Integrate new slices, review samples, run regressions, package/sign locally and record remaining acceptance gaps honestly.

## Tests and native verification

Final build and all 48 tests passed. Ordinary and stable Dev bundles refreshed; package deep/strict signature verification passed. Sample-only screenshots reviewed and basic exported PDF visually inspected.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Not full parity or release sign-off. Complete the acceptance gaps in this matrix and earlier Move To success/separate-volume, exact tooltip timing, clipboard cross-app, accessibility/dark/narrow/picker checks. No notarization/public release.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Use the new sample and report one concrete surprise at a time.

Final handoff: Dev closed normally after sample-only QA; no user draft was discarded. Native spelling, writing review and share-picker open/cancel also passed. Sample version and authorship metadata remain with examples/Cycle28-QA.md. See docs/closeout-audit.md for the still-open scope.
