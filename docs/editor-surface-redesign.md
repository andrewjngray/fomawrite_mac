# Editor surface redesign — Ulysses reference

Proposed after Cycle 84. Andrew's 27 September Ulysses screenshot is a visual and interaction reference, not an asset or style file to copy. The aim is an original Fomawrite writing surface with the same calm hierarchy, while retaining plain UTF-8 Markdown and safe editing.

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
