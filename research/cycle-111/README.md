# Cycle 111 — daily writing handoff

Date: 4 October 2026. Target: **0.3.0-dev10 / macOS 0.3.0 (111)**.

**Complete:** Cycles109–111 are integrated in all three local review copies. The feature sprint stopped at the hour; final native verification and packaging continued afterward to resolve a diagnostic rendering race before release.

## Included work

- [Cycle109](../cycle-109/README.md): Return splits supported simple list items in Visual Edit, with exact-source validation and preserved Undo/Redo. Redundant typography updates no longer erase remaining Redo history.
- [Cycle110](../cycle-110/README.md): searchable, hierarchical outline with keyboard navigation, current Source section, safe cancellation and exact heading jumps from narrow rendered views.
- Cycle111: a consistent Find/Replace bar with rounded fields, grouped Previous/Next controls, match position/count and clear empty-result feedback. Find starts near the Source caret; replacement advances beyond the inserted text, including when the replacement contains the query. Enter/Shift-Enter navigate results and Escape returns to writing. Narrow/dark layouts retain usable controls.
- Find cancels stale view-transition restoration, closes when Source is hidden and updates results without stealing the caret while editing Source.

The existing stationary footer and Source/Split/Full/Visual Edit behavior remain part of the regression gate. No Cycle112 implementation or public release is included.

## Verification record

- Production build and local packaging pass. [Final build](build-final.log), [package](package.log), [installation](install.log).
- Full regression suite: **159 passed, 0 failed**. [Log](test.log).
- Nine focused native writing/footer workflows: **11 passed including setup/cleanup**, no QML warnings. [Log](native-regression.log).
- Final expanded Find regressions: **4 passed** offscreen and native, including interruption of pending view restoration and typing/Undo while Find remains open. [Offscreen](find-final.log), [native](find-native-final.log).
- Each final Dev, demo and Applications executable passes **124 footer states and seven daily-writing workflows**, with zero QML warnings, matching embedded resources, exact version metadata and strict ad-hoc signatures. [Dev](dev/bundle-verification.json), [demo](demo/bundle-verification.json), [Applications](applications/bundle-verification.json). [Artifact identity record](artifacts.json).
- Daily workflows cover Find/Replace navigation and Undo, narrow/dark controls, searchable/current-section outline, exact narrow Visual→Source jumps, four visual list splits and refusal inside protected formatting. Screenshot review confirms stable controls and inline field prompts.

The native diagnostic uses an isolated, nonactivating window. Two earlier demo attempts measured deferred SplitView geometry before Qt drew a frame; the next captured frame showed the correctly laid-out footer. A serialized retry reproduced this, so the diagnostic now explicitly renders two fixed frames before reading click targets/bounds, then applies every original geometry and source-safety assertion. It never waits for an expected geometry or relaxes a failing bound. Earlier reports are retained in [initial report](demo-initial/report.json) and [serialized retry](demo-serial-before-frame-barrier.json). Final reports above certify the rebuilt checker and actual bundles.

Native keyboard tests deliver explicit Qt events to the test window because QtTest’s Cocoa key helper marks events as spontaneous and the existing isolation filter correctly rejects them. This preserves protection from unrelated OS typing while exercising the real QML keyboard path. Physical input/VoiceOver remains a separate acceptance check.

## Captures for review

- [Find at a wide width](dev/daily-find-wide.png)
- [Replace controls](dev/daily-replace-wide.png)
- [Find at a narrow width](dev/daily-find-narrow.png)
- [Dark narrow Find](dev/daily-find-narrow-dark.png)
- [Filtered outline](dev/daily-outline-filtered.png)
- [Outline opened from narrow Visual Edit](dev/daily-outline-narrow-visual.png)
- [Heading jump into visible Source](dev/daily-outline-jump-source.png)
- [List before Return](dev/daily-list-before-return.png)
- [List after Return](dev/daily-list-after-return.png)

These captures use disposable synthetic writing. The diagnostic loads the real executable's embedded QML and delivers synthetic Qt events to its own isolated window. It does not prove physical macOS input, VoiceOver, multiple-display behavior or arbitrary restored user workspaces. Preview/template/parser parity and general WYSIWYG remain outside this increment.

## Review artifacts and next step

Use `dist/Fomawrite Dev.app`, `dist/Fomawrite.app` or `/Applications/Fomawrite.app`; confirm **Fomawrite → About Fomawrite** shows 0.3.0-dev10 and the intended path. App bundles remain locally ad-hoc signed. The earlier public RC1 download is unchanged.

Try the short [list](../usability/cycle-109.md), [outline](../usability/cycle-110.md) and [Find](../usability/cycle-111.md) exercises. Review feedback should determine the next bounded increment. Link-popover polish was identified as a possible next scope but was not implemented in this sprint.
