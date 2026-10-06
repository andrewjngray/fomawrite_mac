# Cycle 135 — Visual Edit retired, builds aligned

**Build:** `0.3.0-dev31` / macOS `0.3.0 (135)`. **Date:** 7 October 2026. **Decision:** Andrew, after reading the Cycle 134 record: retire the Visual Edit pane now that Live covers it, install the current build, and align every bundle.

## What changed and why

| Removed | Why |
| --- | --- |
| `src/VisualEditPane.qml`, `src/sourcevisualmapping.{h,cpp}`, `src/visualtexthighlighter.{h,cpp}` | The bounded Source↔Visual projection was the interim editing surface; the Live editor (Cycle 134) edits the Markdown text itself with syntax rendered in place, so the mapper's "not a Markdown serializer" limits no longer need a second surface. |
| `Backend::styleVisualEditor / visualProjection / visualPositionForSource / navigateVisualTable / applyVisualBreak / applyVisualEdit` and their members | Only the pane used them. `previewMarkdown` and `stylePreview` stay (publishing and the rendered-document tests use them). |
| `workspaceLayout.visualEditEnabled`, command `visualEditing`, footer `visualEditToggle`, the View-menu and workspace-menu items, the header's Visual Edit label | The footer capsule is now **Source / Live**; the workspace menu gained a **Live** item in the old slot. |
| `PublishingPreview.allowVisualEdit / visualEditEnabled / visualEditorObjectName` | Vestigial properties from before the pane split. |

Kept: `setEditingMode(visual)` in `Main.qml` as a thin alias of the new `setSourceEditing()` so older callers keep working.

**Migration.** `WorkspaceLayout.restoreState` still reads a saved `visualEditEnabled` (version 1 and 2 states) and restores it as `liveEditEnabled`, so a workspace left in Visual Edit opens in Live rather than silently falling back to Source.

**Native acceptance code** (`src/documentviewcheck.cpp` and the `*acceptancecheck.inc` bodies, compiled into the app): mode-cycle steps now click `liveEditToggle` where they clicked `visualEditToggle`; the checks that typed into the visual editor (list Return splits, table Tab navigation, Visual Split typing) were removed rather than rewritten against the Chromium page. These checks have **not** been run natively this cycle.

**Install script.** `bin/install-mac` copies `dist/Fomawrite.app` to `/Applications` after verifying the signature and refusing while either bundle is running (the previous `bin/install` is the Arch `makepkg` path).

## Verification

- Full suite: **207 passed, 0 failed, 0 skipped** (offscreen, Qt 6.11.2). The count went **down from 240** because 33 tests existed only to exercise the mapper and the visual editor (`mapper-fuzz.inc`, `sourcevisualmapping-cycle99.inc`, `cycle109-visual-lists.inc`, `cycle116-tables.inc`, `cycle99-integration.inc`, the inline `visualSourceMapping*`/`visualProjection*`/`visualEditor*` tests, `perfMapperScaling`, `visualSplitEdits…`, `outlineSearchJumpsFromNarrowVisual…`). Tests that used Visual Edit only as a second editing mode (footer capsule cycles, layout independence, workspace menu, narrow-width routing, zoom sharing, document chrome, completions) now drive **Live** instead, so those behaviours stay covered. Three tests that rendered Markdown through the pane's `QTextDocument` (`tableOfContentsUsesUniqueAnchors`, `previewsRelativeImagesAndCentersTypewriter`, `headingAndFencePreviewMatchesSource`) now render through a `RenderedMarkdownDocument` helper that calls the same `previewMarkdown` / `stylePreview` path.
- Editor page: 176 node tests, unchanged.
- Bundles: `./bin/package-mac`, `./bin/prepare-dev-app`, `./bin/install-mac` run with the app closed; `dist/Fomawrite.app`, `dist/Fomawrite Dev.app` and `/Applications/Fomawrite.app` all report `CFBundleVersion` 135 ([verified-builds.json](verified-builds.json)). The stale `dist/Omawrite.app` and `dist/Omawrite Dev.app` (pre-fork names) were deleted.
- CI: see the Actions run for the push.

## Limits

- `bin/check-document-views` (rerun 7 Oct against `dist/Fomawrite.app`): passed everything except three theme-folder fixture checks (`folder-drop-selection-and-rejected-choice-checkmarks`, `folder-css-edits-reload-and-basic-settings-actions`, `theme-import-and-basic-settings-dialog-actions`, all reporting that the disposable themes folder could not be created). The Live steps passed. The harness had been failing at its resource check since Cycle 133 (`PreviewPane.qml`), fixed this cycle.
- `bin/check-publishing-startup` (rerun 7 Oct): cold and restored startup both **passed** (reports kept locally in this folder).
- The Live editor has still not been driven by physical input; this build is the first installed one that has it.
- Open from the review: remaining `Backend` extractions, static library/test split, acceptance code in the product.
