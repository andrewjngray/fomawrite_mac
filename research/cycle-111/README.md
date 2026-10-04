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


## Prioritized follow-up after the close-out review

4 October 2026: stop at completed Cycle 111. The review rechecked all three local executable hashes against the artifact manifest; all still match. No further product edits or app rebuilds are included in this close-out. The following findings come from code and existing evidence, not a new physical-input session.

1. **Proposed Cycle 112 — formatting focus safety.** The current Source-formatting guard excludes Visual Edit focus but does not exclude Find, Replace, Outline or link fields. Some command routes (strikethrough, inline code and page break) bypass that guard. Centralize the editing context and enabled states across shortcuts, native menus and toolbar actions. Acceptance: retain a Source selection, focus each auxiliary field, and invoke each formatting route; canonical text, revision and Undo history must remain unchanged. Deliberate formatting from Source must still work after a toolbar click, and Visual must never format a stale Source selection.
2. **Proposed Cycle 113 — safe, consistent link editing.** The nonmodal link popup saves offsets without validating the originating document or source at Apply. Add a stale-target check before replacement; preserve entered fields and offer clear retry feedback if the document changes. Use Edit link/Save for existing links, shared field/action styling, and selection-preserving Cancel/Escape focus restoration. Handle destinations with spaces using valid Markdown. Acceptance: new and existing links, Unicode and escaped delimiters, local paths with spaces, invalid input, intervening typing/reload/document changes, unchanged Cancel and exact one-step Undo.
3. **Everyday-writing acceptance before broader UI expansion.** Exercise Source/Split/Full and Visual Edit with physical mouse/keyboard input, uninterrupted resizing to the actual minimum width and height, pane collapse/reopening, full screen, an existing saved workspace and long documents. Check controls during transitions, focus/caret and reading position, not only settled bounds. Check IME, VoiceOver reading order and display scaling separately. The current packaged checks cover fixed widths 720–1440 at height 800 with synthetic Qt input and isolated workspaces; they do not establish those broader conditions. Include the export popup, context menus and remaining dialogs in the next consistency pass, without treating earlier checks as fresh certification.
4. **Richer editing after those foundations.** Prioritize image selection/placement, keyboard table navigation and more list operations from actual writing feedback. Complex/nested Markdown and unsupported multiline edits retain Source fallback. Typewriter scrolling currently operates on Source; either scope the available control clearly or extend and verify it for rendered editing. Broader parser/export and cross-application clipboard parity remain separate backlog items.
5. **Distribution only when requested.** Developer ID signing, notarization and another-machine installation remain outside the local review build. The public RC1 has not been refreshed.

The recommended next implementation is Cycle 112. Cycle numbers above are proposed scopes, not a promise that complete editor or competitor parity is a fixed number of cycles away.
