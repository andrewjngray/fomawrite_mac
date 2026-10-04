# Cycle 108 — verify the shipped footer and responsive view state

Date: 4 October 2026. Target: **0.3.0-dev9 / macOS 0.3.0 (108)**.

Andrew's screenshots show the prior separate Source and Preview footers: a Source dropdown on the left, and an extra Source button appearing at the right in Visual Edit. The installed build107 executables and current Qt cache contain the shared DocumentFooter instead. No app was running at the start of this review. This establishes a presentation mismatch but does not establish which process produced the supplied screenshots.

The previous native test fixtures loaded Main.qml from the source directory. Their successful screenshots did not prove that the installed executable was displaying the same UI. Cycle108 closes that verification gap and fixes additional responsive behavior found by an independent state audit.

## Changes

- A contracted Source view no longer highlights a hidden Visual Edit preference. One click enters Full Visual Edit.
- Turning Visual Edit off in a contracted Full view stays on read-only Preview instead of jumping to Source.
- Explicit Split works as soon as the two panes fit, including the resize hysteresis interval where the saved Split preference already exists.
- Writing focus follows the actual focused editor item. Clicking Source after Visual Edit no longer leaves stale visual focus that can choose the wrong pane on resize.
- Footer, workspace menu and native View-menu checks describe the effective visible view. Re-selecting an active workspace view must retain its checkmark.
- The shared footer groups appearance/template controls beside the persistent right-hand Source / Split / Full and Visual Edit controls. Mode-driven navigation contraction no longer switches Aa to two menus; the visible action group retains its positions at a fixed window size.
- About Fomawrite shows the running application version and executable path so Dev and ordinary copies can be identified.
- Footer regression fixtures load bundled QML resources. A separate, explicit diagnostic mode in the actual executable uses temporary settings and a disposable synthetic document to exercise its own rendered controls and capture evidence.

## Verification

- `./bin/build` and `./bin/package-mac` pass. [Build log](build.log), [package log](package.log).
- Final `./bin/test`: **152 passed, 0 failed, 0 skipped**, 71 seconds. [Full log](test.log).
- Four focused native Cocoa regression workflows pass (6 including setup/cleanup). [Native regression log](native-regression.log). They cover rapid mode cycles, responsive focus/menu state and repeated compact/resized style menus.
- Rapid bundled-QML tests use real rendered controls, repeated mode and menu choices, long UTF-8 drafts, selection, Undo/Redo and delayed viewport refreshes. Source reading position returns within 2 logical pixels at the same geometry. A prior navigation fixture now enters Visual Edit through the actual view command instead of forcing a focus flag inconsistent with the focused editor.
- The explicit installed-bundle check exercises **124 states per app**, across 1440, 1280, 960, 739 and 720px windows, plus the 820px Split-restoration boundary. It checks every visible footer action's coordinates, visibility and selected state, untruncated short labels, exact source/revision/selection preservation, Undo/Redo, saved bytes and return scroll position. It writes 25 native window captures per copy, including dark mode and About.
- The wrapper requires a fresh report from the expected executable, verifies its executable hash, compares seven embedded QML hashes with the checkout, and checks native Cocoa plus application and bundle versions. QML engine warnings fail the diagnostic.
- Independent native screenshot review confirms stable wide and compact footer controls, correct narrow Visual/Edit/Full states, disabled Split when too narrow, readable labels and aligned bottom rules. The synthetic library's settings/data entries are disposable fixture folders.

The rapid native fixture clicks the positions from the last painted frame and checks all action bounds at the next presented frame. It requires further clicks while viewport restoration is still pending. This avoids targeting an intermediate, unpainted SplitView coordinate while retaining the overlapping-transition test. The full suite retains known font-substitution and unrelated delayed-teardown warnings; the three installed-bundle checks have no QML engine warnings.

The first expanded checks exposed actual focus and mode bugs rather than merely confirming existing snapshots. The initial diagnostic failed the Source-focus scenario before the production focus fix; the complete suite also caught and fixed the workspace menu reselect checkmark issue.

External macOS Accessibility inspection was unavailable (AXIsProcessTrusted false; AX window query -25211; System Events timed out). No OS permissions or privacy settings were changed. The installed-bundle diagnostic uses Qt pointer events delivered to its own temporary window; it does not automate another application or prove physical VoiceOver behavior.

## Review builds

Stable `dist/Fomawrite Dev.app`, ordinary/demo `dist/Fomawrite.app`, and `/Applications/Fomawrite.app` target build108. Their final identities and check results are recorded in [artifacts.json](artifacts.json). All are locally ad-hoc signed. [Dev check](dev/bundle-verification.json), [demo check](demo/bundle-verification.json), [Applications check](applications/bundle-verification.json). The previous Applications bundle is preserved as recorded in [install.log](install.log). Public RC1 remains unchanged.

## Captures from the installed Dev executable

- [Source at 1440px](dev/width-1440-pass-0-step-0.png)
- [Split at 1440px](dev/width-1440-pass-0-step-3.png)
- [Full Preview](dev/width-1440-pass-0-step-7.png)
- [Source at 960px](dev/width-960-pass-0-step-0.png)
- [Split at 960px](dev/width-960-pass-0-step-3.png)
- [One-click Visual Edit after contraction](dev/narrow-source-to-visual.png)
- [Turning Visual Edit off retains Full Preview](dev/narrow-visual-to-preview.png)
- [Explicit Split at 820px](dev/explicit-split-at-820.png)
- [Dark Split](dev/dark-split.png)
- [Running app identification](dev/running-app-about.png)

Full Dev captures and reports are retained. Duplicate demo/Applications PNGs remain local; their reports and verification manifests are committed. [Optional review exercise](../usability/cycle-108.md).

Physical input/VoiceOver, multi-display scaling, arbitrary restored user workspaces and the public distribution process are not covered by the bundle diagnostic.
