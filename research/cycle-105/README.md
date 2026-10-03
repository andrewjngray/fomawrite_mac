# Cycle 105 — toolbar matched to the Ulysses reference

Date: 4 October 2026. Review target: `0.3.0-dev6`, macOS `0.3.0 (105)`.

Andrew supplied a closer toolbar reference and asked for its cleaner, more succinct icon treatment in the actual build. This cycle refines the existing controls rather than introducing additional commands.

## Changes

- Darker enabled toolbar ink and softer perimeter borders have their own light, paper and dark theme tokens. Document typography, column colors and blue folder styling stay separate.
- Circular tools and capsule groups are 34px high, with 32px segments and 18px icon frames. The formatting and zoom capsules have no internal rules; Back/Forward keeps one short inset divider.
- Locally drawn glyphs simplify compose, diagonal chain link, search, share and sidebar controls. The pilcrow has a filled bowl and two stems. Bold stays a true bold B; italic uses a clear serif I. Geometry stays on the shared 24-unit grid, with a stronger toolbar stroke.
- The folder heading and quick − / + controls from Cycle 104 retain their behavior. Focus rings, accessible names, hover/pressed/disabled states, formatting guards and compact-layout actions remain.
- The existing screenshot helper moves the pointer off toolbar controls before capture so review images show their resting state.

## Native visual comparison

These are running Qt app captures with disposable documents, not generated mockups. The filenames retain the cycle numbers of the reused regression fixtures; all images in this directory were freshly captured from build 105 source.

- [Wide Source toolbar](screenshots/cycle103-source-wide.png)
- [Reference-size Source](screenshots/cycle103-source-reference.png)
- [Dark toolbar](screenshots/cycle103-source-dark.png)
- [Narrow toolbar](screenshots/cycle103-source-narrow.png)
- [Collapsed navigation](screenshots/cycle104-controls-collapsed.png)

The native comparison found no clipping, overlapping settled controls or mismatched icon centering. This is a close visual match to the supplied treatment, not a pixel-identical or asset-identical claim. The existing two independent pane-reopen controls share a sidebar glyph and retain distinct tooltips/actions.

## Verification

- `./bin/build` passed. [Log](build.log).
- Full `./bin/test`: **144 passed, 0 failed, 0 skipped**. [Log](tests.log). No additional tests were added solely for cosmetic properties.
- Six existing native Cocoa tests: **8/0/0** including setup/cleanup. [Log](native-controls.log). They cover native composition/pane geometry, real document-card navigation, divider dragging, quick zoom in Source/Preview/Visual Edit, exact source and Undo/Redo preservation, all supported narrow layouts, folder-picker action routing, collapsed-pane restoration and stale visual-formatting guards.
- An independent read-only comparison reviewed native wide/reference/narrow/dark captures and found no visual blockers. Native test logs include transient overlap diagnostics while Qt settles the layout; geometry and pointer assertions pass after settling. Full logs retain existing Preview/Material teardown warnings.
- Physical VoiceOver/display-scale acceptance and Andrew’s visual sign-off remain open. The broader native tab/window matrix and public distribution were not expanded in this cosmetic cycle.

## Review builds

Stable `dist/Fomawrite Dev.app`, ordinary/demo `dist/Fomawrite.app` and `/Applications/Fomawrite.app` are all build 105. Strict/deep signatures and matching demo/Applications executable hashes are recorded in [artifacts.json](artifacts.json). Build preparation and installation are in [package.log](package.log), [dev-install.log](dev-install.log) and [install.log](install.log). The previous Applications copy is retained as recorded there. These are local review builds; the public RC1 is unchanged.

[Optional review exercise](../usability/cycle-105.md). Next work follows Andrew’s visual/writing feedback; review-led image/table/list ergonomics remains the subsequent product scope.
