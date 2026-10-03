# Cycle 102 — pane restoration and template access

Date: 4 October 2026. Review target: `0.3.0-dev3`, macOS `0.3.0 (102)`.

Andrew reported that Library/Organizer collapse controls disappeared with their headers, and requested clearer View commands and direct template selection from the Preview footer.

## Changes

- Each hidden navigation pane gets a circular reopen button at the beginning of the surviving document header. These controls remain available when the toolbar is faded or hidden; compact layouts open the existing temporary navigation drawer when docking cannot fit.
- View begins with **Toggle Library** and **Toggle Organizer**. Their checkmarks reflect the visible layout. Template sits lower in a separate section, between writing appearance and preview commands.
- The Preview footer displays a tonal template dropdown with the current name and a chevron. It offers the same built-in/custom choices as View → Template. Both update the same backend setting; Visual Edit retains its existing status and Source controls.
- Header width calculations account for reopen controls. The footer shortens/elides template names in narrow panes while retaining mode buttons.

## Verification

- `./bin/build` and final `./bin/package-mac`: passed; the [package log](package.log) includes the final rebuild.
- `./bin/test`: **137 passed, 0 failed, 0 skipped**. [Full log](tests.log).
- Native macOS pointer/menu tests: **5 passed, 0 failed, 0 skipped**, including setup/cleanup. Covers both pane-restoration orders, hidden toolbar, compact Drawer, View order/separators, synchronized Palatino/Helvetica choices, 320/360px footers, popup placement above the trigger, and draft/Undo/Redo preservation. [Native controls log](native-controls.log).
- `./bin/test-window-routing`: **111 passing assertions**, including independent native tabs/windows, focus, compact navigation, full screen and exact draft/Undo preservation. Run separately from other GUI tests. [Native workspace log](native-workspace.log).
- Synthetic app-only screenshots were inspected for the collapsed headers and full/narrow template menu. [Both panes collapsed](screenshots/cycle102-both-collapsed.png), [template menu](screenshots/cycle102-template-selector.png), [narrow template menu](screenshots/cycle102-narrow-template-selector.png).

The checks also exposed a deferred show/hide race: canceled pane reveal requests now stop before opening temporary navigation. Test fixtures wait for layout polish and popup exit transitions, and target the main Preview rather than Export’s hidden preview instance. Source mapping and save/unsaved guards remain unchanged.

## Installed review copies

All three copies are synchronized to `0.3.0-dev3` / `0.3.0 (102)` on 4 October 2026 and pass strict deep local signature verification:

- `dist/Fomawrite Dev.app` — stable Dev identity, prepared with `bin/prepare-dev-app`.
- `dist/Fomawrite.app` — ordinary/demo package with bundled Qt.
- `/Applications/Fomawrite.app` — installed from the ordinary package; matching executable hash.

All were closed before replacement. The prior Applications copy is retained in the backup path in [install.log](install.log). Exact hashes, bundle identities and product source revision are recorded in [artifacts.json](artifacts.json). These local ad-hoc signatures do not imply notarization or a new public release.

## Review

See [the optional exercise](../usability/cycle-102.md). Physical VoiceOver/display checks and broader writing acceptance remain open. Optional distribution Cycle 101 stays separate; the next product increment is Cycle 103, chosen from image/table/list friction during normal writing.
