# Cycle 112 — independent writing-pane zoom

Date: 4 October 2026. Review build: **0.3.0-dev11 / macOS 0.3.0 (112)**.

Complete and verified in the stable Dev, ordinary demo/package and Applications copies.

## Included work

- Independent Source and Preview minus / percentage / plus controls. Visual Edit uses the Preview pane’s zoom.
- Saved 75–200% zoom, presets, Reset to 100% and optional Link zoom. Linking adopts the initiating pane’s percentage; unlinking preserves the values.
- A readable Preview baseline, with one-time migration of the existing Source size.
- Retained reading anchors through repeated zoom/reset and rewrapping, invalidated by actual scrolling, editing or view/document changes.
- Divider dragging and double-click balance, respecting Source 480px / Preview 320px minimum widths.
- Stable footer and aligned header controls through Source, Split, Full, Visual Edit and narrow/dark layouts.
- Screen magnification preserves Markdown, selection, Undo/Redo and exported typography.

## Verification

- Production build and packaging pass; all three local copies have strict ad-hoc signatures.
- Full regression suite: **163 passed, 0 failed**.
- Focused native Cocoa checks: **8 passed including setup/cleanup, 0 failed**, with no QML warnings.
- Each actual app bundle passes **124 footer states, seven daily-writing workflows and five pane-zoom workflows**, with matching embedded resources/version and no QML warnings.
- Repeated preset/reset and divider-balance checks measured **0px reading-anchor drift** against a 2px bound. Pointer tests cover both double-click balance and subsequent ordinary drag, with width persistence across modes.
- Exact Markdown, dirty state, selection, Undo/Redo, saved-file bytes and exported HTML bytes remain covered.

[Verified build identities](verified-builds.json). [Optional writing exercise](../usability/cycle-112.md).

The detailed reports, logs and synthetic screenshots remain local under `research/cycle-112/`; `LOCAL-VERIFICATION.md` indexes that evidence. They are excluded from this public repository’s new commit. The full-suite log retains existing compiler/font and Material SplitView teardown warnings; the zero-warning claim applies to focused native and final bundle checks.

## Limits and next work

Native checks deliver synthetic Qt pointer/key events to isolated app windows. Physical mouse/trackpad, VoiceOver, IME, multiple displays and arbitrary restored user workspaces remain acceptance work. Broader custom-template/PDF combinations and multi-window preference changes need wider coverage. No public/notarized app release is included.

This user-requested zoom increment supersedes the earlier proposed Cycle112 formatting-focus work. Formatting focus safety and safe link editing remain the next bounded improvements, followed by review-led image/table/list ergonomics.
