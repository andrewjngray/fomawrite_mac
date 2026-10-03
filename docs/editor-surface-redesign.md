# Editor surface redesign — Ulysses reference

Proposed after Cycle 84. Andrew's 27 September Ulysses screenshot is a visual and interaction reference, not an asset or style file to copy. The aim is an original Fomawrite writing surface with the same calm hierarchy, while retaining plain UTF-8 Markdown and safe editing.

Implementation checkpoint: the approved workspace pass integrated Cycles 90–96 into build 97, followed by bounded editing/navigation work in Cycles 98–100. The current target is `0.3.0-dev3` (macOS `0.3.0`, build `102`), adding the pane restoration and template access changes requested in Andrew’s 4 October review. Existing theme/writing choices remain; Studio and Editorial are defaults for new installations and an explicit combination for existing users.

Cycle 98 adds keyboard pane navigation, compact navigation focus/return and inactive toolbar treatment. Its production native fixture passes 111 assertions across workspace and navigation checks. Cycle 99 extends the bounded projection to simple table cells, image alt text with local thumbnail inspection, inline code and supported list-end Return/task continuation/empty-item exit. Grapheme-safe edits, empty-document input and visual caret tracking strengthen normal editing. Cycle 100 fixes heading links into live documents already open in another window and missing-heading feedback.

Source remains canonical UTF-8 Markdown and appearance stays independent of export style. Arbitrary multiline paste, splitting within a list item, complex/nested structures, spatial image placement and a full editable image/table grid are not implemented. General WYSIWYG still requires further source-mapping work. Current tests and installed bundle verification are recorded in the [Cycle 102 handoff](../research/cycle-102/README.md); physical VoiceOver/display checks and Andrew's writing acceptance remain separate.

The [3 October workspace UI redesign plan](workspace-ui-redesign-plan.md) now records the approved complete-workspace implementation and supersedes the original chrome sequence. The “Fomawrite today” table below preserves the pre-Cycle-85 assessment; use the workspace plan's implementation checkpoint for current behavior. The earlier Cycle 88 acceptance and broader feature gaps remain open.

## What the reference gets right

The screenshot uses three related, distinct column tones: a quiet gray navigation rail, a lighter sheet list, and a slightly warm off-white writing canvas. The text has generous line spacing, moderate measure and a broad top margin. Markdown markers are present but recede; bold and emphasis are evident within the editable-looking text. The toolbar uses small, grouped controls and leaves the document as the visual focus. A word-count chip is available without occupying a permanent footer band. The image is treated as a content object rather than a long source path.

These observations describe the screenshot, not a claim about how Ulysses implements editing internally. The proposed colors and typography should be tuned against Fomawrite in light, dark and scaled display modes, rather than sampled as an exact copy.

## Fomawrite today

| Area | Current behavior | Result |
| --- | --- | --- |
| Source editor | Editable plain Markdown; fixed iA Writer Mono S at 16 px by default; about 65-character measure; 10 px top inset | Reliable source and undo, but the page feels more like code than prose and starts too high. |
| Visual Edit | Editable projection of supported inline spans; at least 18 px body; source-only blocks remain protected | Closer visually, but line breaks and many Markdown structures cannot yet be edited there. It cannot serve as the sole everyday editor. |
| Preview | Styled and read-only, sharing the output template | Good for checking output, but it is not the writing surface. |
| Color | Light and Warm Paper themes; organizer and file list both use `palette.panel` | The three columns lack the subtle depth of the reference. Warm Paper affects the whole app and is more yellow than the reference. |
| Controls | Formatting actions and size controls exist; source/preview layout is selectable | The capabilities exist, but typography, workspace layout and theme are not presented as a coherent writing appearance. |

## Recommended design

Make **Writing appearance** a persisted, reversible choice independent of document contents and export styles:

| Choice | Main text | Canvas and rhythm | Intended use |
| --- | --- | --- | --- |
| Manuscript | Existing mono face | Current compact source layout | Precise Markdown editing; preserves today's default for existing users. |
| Editorial | Readable proportional sans with visible bold/italic | Soft off-white canvas, 19–20 px body, about 1.5 line spacing, 680–740 px measure, 60–90 px top inset | The closest Fomawrite-owned interpretation of the reference. |
| Book | Readable serif | Same generous measure and spacing, slightly warmer page | Long-form reading and drafting. |

Use an optional **Studio** light theme for the surrounding workspace: neutral-gray organizer, near-white file list, warm off-white editor, understated borders and a darker gray selected item. This is separate from the typography choice; an Editorial page should also work with Dark or Warm Paper. Keep a minimum 4.5:1 contrast ratio for normal text and clear caret, selection, focus and hover states. Do not change the stored Markdown, output style or selected export template when either appearance control changes.

For the first implementation, keep the Markdown source `TextEdit` as the canonical editor. Use the existing highlighter to make syntax markers quieter while preserving visible caret movement and selection. Let bold and italic remain legible in place. Source-only constructs such as fenced code, complex tables and image paths remain explicitly source text. Editor-only layout can make the document dominant, but selecting an appearance must not silently change a user's chosen split/full layout.

## Development sequence and gates

1. **Cycle 85 — original Studio palette and writing presets.** Add the three persisted writing appearances, independent of Light/Dark/Studio theme and output styles. Apply font family, size, line spacing, measure and top inset to the source editor; give organizer, file list and writing surface distinct but coordinated tones in Studio. Keep Manuscript as the existing-user default. Verify Markdown bytes, cursor, selection, undo, save/reopen and dark-mode contrast are unchanged by switching appearance. Capture synthetic wide/narrow images for Andrew's visual review before installing the ordinary app.
2. **Cycle 86 — editor chrome and navigation polish.** Simplify the writing toolbar, align the document and word-count presentation, and tune selected rows, dividers and hover states. Keep existing keyboard commands and VoiceOver names. Test narrow windows, long filenames and expanded library/organizer states.
3. **Cycle 87 — richer in-place formatting.** Extend the editable projection one construct at a time (paragraph boundaries and lists first, then links and images only with safe mapping). Require round-trip fixtures for Unicode, nested syntax, paste, Undo/Redo and unsupported spans. No whole-document HTML-to-Markdown rewrite. If source and projection cannot map an edit exactly, route it to Source with a clear explanation.
4. **Cycle 88 — native usability and acceptance.** Use long real-world but non-private sample documents, check cursor/scroll stability while switching appearances and modes, verify light/dark/narrow/display scaling, and compare a saved/reopened document to its original bytes. Andrew judges whether Editorial is visually close enough to become the default for new installations.

The attractive content blocks in the reference—especially inline image cards, link pills and annotations—need the later source-mapping work. They should not be faked in the first styling cycle or represented as generally editable until round-trip behavior passes.

## Follow-through after Cycle 100

Cycle 102 addresses navigation/template feedback. The next product cycle is 103: use the combined build for normal writing, identify the most important image/table/list friction, and implement one safe structural interaction at a time. Candidates are better image placement and keyboard access, clearer table structure/navigation, and edits within a list rather than only at its end. Preserve exact source and canonical Undo/Redo; keep unsupported operations visible and reversible.

Complete remaining human acceptance alongside that work: VoiceOver reading order, actual display scaling/multiple displays, long-document writing and arbitrary-workspace relaunch. Cycle 101 stays an optional distribution track only after an explicit decision. No completed styling or bounded editing cycle establishes general visual editing or competitor parity.
