# Cycle129 — publishing startup and theme watcher loop

Review build **0.3.0-dev25 / macOS 0.3.0 (129)**. All three local copies are synchronized and verified: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app` and `/Applications/Fomawrite.app`. [Build identities](verified-builds.json).

Andrew's continuing blank previews exposed a separate defect after Cycle128. Theme refresh removed and re-added all `QFileSystemWatcher` paths. On macOS with his populated themes folder, this replayed notifications roughly every 200 ms; every signal restarted the preview's 220 ms debounce, preventing generation. The old normal-startup run recorded **67 signals per backend in 15 seconds and no output**. Shared theme data explains the failure across app copies.

Cycle128's document-identity and screenshot-gating repairs address valid defects, but its tests did not exercise normal startup/restoration with populated themes. Its passing checks did not justify claiming the whole incident resolved.

## Repair

Watcher paths change only when membership changes. Cached content snapshots suppress unchanged notifications; file events invalidate content cache, catching same-size edits with preserved modification times. Atomic replacements re-arm watches. Explicit Reload Themes forces refresh. The publishing pane shows loading status when an empty URL accompanies pending generation.

## Evidence and limits

- Final full offscreen suite: **204 passed, 0 failed, 0 skipped**. Expanded final watcher fixture: **4 passed, 0 failed, 0 skipped**, including two cases plus setup/cleanup. Two Qt Material `SplitView.qml` null-parent TypeErrors occur in `savesAndOpensFromFooterMenu`; native checks report zero QML warnings.
- Added offscreen watcher checks also **pass old code**. They check bounded behavior but do not reproduce the macOS event loop. Do not describe them as red/green tests.
- Actual old native normal startup with a copied populated user themes folder **fails**. Corrected exact cold (**3327 ms**) and restored (**4260 ms**) startup **passes**, with normal quit confirmed and zero QML warnings. Default public synthetic fixture also passes cold (**3320 ms**) and restored (**3135 ms**). Reports: `/private/tmp/fomawrite-startup-theme-loop-red-128` and `/private/tmp/fomawrite-cycle129-final-startup`.
- [bin/check-publishing-startup](../../bin/check-publishing-startup) is a compile-only harness using normal Main startup, persisted settings/workspace, enabled QML cache and normal guarded quit. No forced frames. Default input is synthetic populated themes; optional theme/document environment inputs are copied into disposable state.
- Actual Dev saved workspace renders matching PDFs. An OS screenshot was viewed; private text, theme files and raw captures are not committed.
- Dev, ordinary and installed copies each pass **4 focused preview groups / 8 actual-pane paints**, with zero QML warnings and strict signatures. Reports include `/private/tmp/fomawrite-cycle129-dev` and `/private/tmp/fomawrite-cycle129-ordinary`. Ordinary/installed executable hashes match. The previous installed copy is preserved at `/private/tmp/Fomawrite-before129-1791225132.app`. Dev quit normally and relaunched through `open` without diagnostic injection.

These checks establish the startup defect and its bounded repair. Physical input, full UI acceptance, arbitrary themes, long-running sessions, VoiceOver and multiple displays remain open. No public release/notarization is included. [Developer handoff](../../DEVELOPER_HANDOFF.md), [optional review](../usability/cycle-129.md).
