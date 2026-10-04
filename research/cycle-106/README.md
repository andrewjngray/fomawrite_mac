# Cycle 106 — consistent, working workspace footers

Date: 4 October 2026. Review target: `0.3.0-dev7`, macOS `0.3.0 (106)`.

Andrew reported that the bottom controls looked unlike the header, did not line up across columns and did not work consistently. The old Source and Preview bars were 38px and 48px high, the navigation columns had no matching footer, and a separate 18px global status strip sat beneath them.

## Changes

- Shared 52px pane footers align their top rule and bottom edge. Scroll areas reserve the actual footer height. The extra global strip is removed.
- Footer controls reuse the header's system font, rounded capsules, theme colors and hover/pressed/selected/focus behavior. Split/Full form one capsule; template and appearance menus have clear chevrons.
- Menus open and close immediately: the inherited Material exit fade could consume a rapid repeat opening click. Radio-style entries retain their checkmark when the already-selected appearance/template is chosen again.
- Source has a direct view menu and a compact Manuscript/Editorial/Book menu with text-size actions. The latter replaces the large settings menu previously opened from the bottom edge. Statistics mode keeps those view/appearance controls available.
- The Library filter shares the footer baseline and rounded control treatment. Document status stays in the column footer, with a visible fallback when its usual labels are hidden.
- Visual Edit focuses its editable text when selected. Its Source return and layout actions retain the existing guarded document routing.
- The [workspace guide](../../docs/workspace-ui-redesign-plan.md) records the shared component and behavior rules.

## Verification

- `./bin/build` and the final package rebuild pass. [Build log](build.log), [final package log](package.log).
- Full `./bin/test`: **147 passed, 0 failed, 0 skipped**. [Log](tests.log).
- Five affected native Cocoa workflows: **7/0/0 including setup/cleanup**, with no warnings in the final run. [Log](native-footer.log). Tests exercise actual menu/button clicks, rapid reopening, selected-item reselection, every built-in template and the custom-template dialog, appearance presets and size limits, statistics, filter typing, mode changes and exact draft/Undo/Redo/disk preservation.
- Geometry checks cover 720/960/1440px windows, the 520px minimum height, a 320px Preview pane, collapsed navigation, Visual Edit and explicit light/dark themes. They verify aligned footer edges, non-overlapping controls, menu bounds and a status failure message above the footer.
- The native fixture waits for window exposure/resizing. Its mouse-only test excludes unrelated native keyboard/IME input; the final run excluded no events. This does not weaken source assertions or the separate keyboard-filter test. Earlier native runs received unsolicited characters; the isolation is test-only, not an app input change. Diagnostic logs remain alongside the final log.

## Actual app captures

These are native Qt window captures with disposable documents, not mockups. Older cycle102 filenames identify reused fixtures; all captures here are current source.

- [Source](screenshots/cycle106-footer-0-1440-light.png)
- [Source and narrow Preview](screenshots/cycle106-footer-1-1440-light.png)
- [Full Preview](screenshots/cycle106-footer-2-1440-light.png)
- [Short window with appearance menu](screenshots/cycle106-appearance-short.png)
- [Narrow dark layout](screenshots/cycle106-footer-1-960-dark.png)

Visual review confirms aligned rules, consistent type and rounded control treatment, readable selected states and contained menus. Template names elide at the Preview minimum width while retaining their full tooltip/accessible name.

## Review builds

Stable `dist/Fomawrite Dev.app`, ordinary/demo `dist/Fomawrite.app` and `/Applications/Fomawrite.app` are synchronized to build 106. Strict/deep signatures pass; demo and Applications executables match. [Exact identities](artifacts.json), [Dev preparation](dev-install.log), [Applications installation and previous-copy backup](install.log).

[Optional review exercise](../usability/cycle-106.md). Next work follows Andrew's review of the actual footer behavior and composition; broader image/table/list ergonomics remain a separate cycle.

No Markdown rewrite or appearance preference migration is introduced. Physical VoiceOver/display-scale acceptance and Andrew's normal writing review remain separate checks. Public RC1 is unchanged.
