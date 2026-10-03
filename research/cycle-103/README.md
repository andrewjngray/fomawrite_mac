# Cycle 103 — approved concept in the native app

Date: 4 October 2026. Review target: `0.3.0-dev4`, macOS `0.3.0 (103)`.

Andrew’s comparison showed that the earlier implementation still differed materially from the approved concept. This cycle uses that reference for the actual QML workspace and compares screenshots of running native controls with the same disposable sample text.

## What changed

- Pane dividers are one continuous rule from header to body. Their invisible nine-pixel hit areas retain practical drag resizing. The reference widths are 184px Organizer and 232px Library.
- Blue tabbed folder outlines replace the bucket-like glyphs. Compose, pane, search and formatting glyphs share a consistent drawing grid. Round 32px tools and divided capsules match the concept; redundant header actions move into the existing library menus.
- System-font labels use regular weight, with deliberate medium section headings and semibold document titles. Real local documents appear as dated cards with a heading title and readable excerpt. Reads are bounded to a 2KB prefix and checked against the library root.
- The writing view uses warm `#F6F3EE` paper, 17px Menlo on macOS for Manuscript, a maximum 680px writing width, quieter inline syntax, a visible word-count pill and calmer search/footer controls. Editorial and Book remain available. Combined bold/italic formatting is corrected, with code kept literal and source/Undo preserved.
- On the first upgraded launch of each app copy, restored windows adopt Source/Manuscript, cards and both navigation panes. The migration key follows that copy’s workspace identity because Dev/demo/Applications share preferences but have separate checkpoints. The upgraded checkpoint is saved before the migration is marked complete; the flag is cleared from live QML engines so later layout changes persist.
- **Aa → Reference writing layout** reapplies the presentation without altering the open document or its chosen export template. Preview/Visual Edit/Split remain in the workspace control, View menu and Source footer mode menu. Cycle 102’s pane reopen and Preview template controls remain.

## Visual evidence

These are native Qt window captures with synthetic documents, not HTML mockups or generated images:

- [Reference-size Source view](screenshots/cycle103-source-reference.png)
- [Reference-size Source with Find](screenshots/cycle103-source-find.png)
- [Wide Source view](screenshots/cycle103-source-wide.png)
- [Narrow Source view](screenshots/cycle103-source-narrow.png)
- [Dark Source view](screenshots/cycle103-source-dark.png)
- [Palatino Preview retained](screenshots/cycle103-preview-wide.png)

The functional Organizer retains actual Favorites, Tags and Recents rather than inserting fictional reference items into a user’s library. Native window decoration and source-editing behavior remain native. Andrew’s visual sign-off is still pending.

## Verification

- Full regression suite: **141 passed, 0 failed, 0 skipped**. [Log](tests.log).
- Native reference composition, draft preservation and inline formatting checks: **6/0/0**, including setup/cleanup. Real pointer clicks open document cards; an off-center drag resizes a one-pixel divider; header/body rule positions are checked throughout. [Log](native-controls.log).
- Retained Cycle 102 native pane/template checks: **5/0/0**, including setup/cleanup. [Log](native-controls.log).
- Native production window-routing fixture: **113 passing assertions**, including seeded legacy Book/Preview migration, saved Unicode text, Palatino output, subsequent user checkpoints, unsaved edits/Undo, independent tabs/windows, resizing, keyboard focus and native print/sharing routes. [Log](native-workspace.log).
- The final quiet-inline preset was rechecked in the focused native tests and refreshed captures. Standard build/package and stable Dev preparation are logged in [package.log](package.log) and [dev-install.log](dev-install.log).

The full-suite logs contain existing teardown warnings from delayed Preview callbacks and Qt’s Material SplitView; no assertions failed. Physical VoiceOver and multi-display checks remain open.

## Review builds

- `dist/Fomawrite Dev.app` — stable Dev identity.
- `dist/Fomawrite.app` — ordinary/demo package with bundled Qt.
- `/Applications/Fomawrite.app` — installed ordinary package.

Bundle identities, source revision, hashes and signature verification are recorded in [artifacts.json](artifacts.json); installation and the retained previous Applications backup are in [install.log](install.log). Local ad-hoc signing is not a notarized public release.

[Optional review exercise](../usability/cycle-103.md). Next: review this actual build against the reference, then choose the remaining writing friction for the next cycle. Optional distribution Cycle 101 remains separate.
