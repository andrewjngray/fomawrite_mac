# Cycle 159 — Windows Format menu routes

22 October 2026. Windows Format now exposes the 28 remaining portable actions already present in NativeMenuBar: headings, lists/tasks, blockquote/body, structure, highlight, code blocks, Wikilinks, snippets, date, table, table of contents, horizontal rule and Clear Styles. Each uses the existing shared handler through Windows focus dispatch. Existing shortcuts, Source/Live guards and the Source-selection requirement for Clear Styles are preserved. No backend or native Mac menu changes were made. Current build/package identity remains in [Windows STATUS](../../docs/windows/STATUS.md).

`windowsFormatMenuSourceEditsAreAtomic` requires each actual Windows menu node, its native label and menu membership, then pointer-activates it and verifies exact text, unchanged saved bytes, one-step Undo and Redo. `windowsFormatMenuLiveAndGuardsPreserveSelection` checks all 28 guards, inert auxiliary/Preview actions, and real shipped Live-page selections for heading, wrapping and snippet commands. The production platform boundary is asserted before enabling the Windows component on the Mac host; existing Mac semantic cases remain intact.

Long-menu coverage uses actual key navigation and timestamped wheel input, waits for scrolling to settle, verifies the target centre against the window, viewport and clipping ancestors, then sends a direct pointer click. It does not set scroll position or directly trigger the successful formatting action. The initial two bottom-row failures were fixture visibility/input-timestamp defects; those logs remain under `evidence/`. No product change was added to obtain the corrected result.

Verification on Qt 6.11.2, ARM64/macOS, offscreen:

- Tests-first red on the Cycle158 product: **2 passed, 40 failed, 0 skipped**, 5.940 seconds, including init/cleanup. All **29 Cycle159 cases** failed at missing real Windows Format nodes; the other 11 failures belong to Cycle160's independently missing routes. Log: `evidence/combined-missing-menu-red.log`.
- Combined focused green after both bounded ports: **53 passed, 0 failed, 0 skipped**, 47.861 seconds. Includes all Cycle159 cases, all Cycle160 routes and production-platform/component rows, eight existing Source/Live formatting regressions, and `windowsMenuFormattingKeepsImmediateSaveShortcut`. Log: `evidence/combined-menu-green.log`. The first attempted green was **51/2/0**, retained as `evidence/combined-first-green-failed.log`; it is not old-product red evidence.
- Required `./bin/build`: **passed**; log `evidence/build.log`. `git diff --check`: clean.

Native Windows ARM64/x64 menu input, deployed-package checks and CI remain pending. Full regression is reserved for the combined integration gate after the next bounded route batch. No clipboard tests, full suite, VM commands, commit or push ran in this slice; the primary Mac checkout/apps remain untouched. This is implemented host coverage, not a full parity or promotion claim.
