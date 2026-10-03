# Cycle 99 — Safe visual blocks

3 October 2026. Integrated review build: application `0.3.0-dev2`, macOS `0.3.0 (100)`.

## Delivered

- Standalone simple images project their description; selecting one offers an image inspector with a local thumbnail and original destination. Description edits preserve path, optional title and other source bytes. Remote images are not fetched by this inspector.
- Confirmed simple pipe tables project editable cell text, protected separators and a distinguished header. Empty cells accept text. This is a text projection, not an aligned graphical grid.
- Inline code uses a code face without exposing its delimiters. Overlapping heading, emphasis, strong and link formatting composes correctly.
- End-of-item Return continues simple unordered/ordered lists, preserves indentation, increments numbering and resets task checkboxes. Return on an empty item exits the list. Structural actions own their Undo unit.
- Empty documents accept typing; minimal Unicode diffs expand to complete graphemes. Emoji, skin-tone changes and combining accents preserve canonical Undo.
- Visual caret tracking keeps long documents in view. Projection updates defer during input composition. Rejected edits show an actionable Source notice even in narrow panes.
- Fence type/length, ambiguous table rows, thematic breaks and multiline HTML comments remain protected. Ordinary edits must reproject exactly and retain their editable mapping.

## Verification

`./bin/build` passes. The integrated suite passes **134 tests, zero failures/skips** ([tests](tests.log)). The independent mapping/format harness passes **10/0/0** ([mapping checks](mapping-tests.txt)).

Native Cocoa tests pass image inspection and real TextEdit edits, source/Undo preservation, list continuation, local-only thumbnail resolution, Unicode replacements, long-document caret visibility, narrow refusal feedback and heading navigation: **5/0/0** including setup/cleanup ([native checks](native-visual.log)). App-only synthetic [image inspector](screenshots/image-inspector.png) and [long visual document](screenshots/long-visual-document.png) captures were inspected.

The initial integrated run found an empty-grapheme boundary rejection in new documents; corrected before the final suite. Test setup also needed to observe queued file-load completion and address the main PreviewPane rather than Export's hidden instance. Those were fixture corrections. The new navigation checks found and corrected a real stale-preview render race, recorded in Cycle 100.

## Explicit limits

Multiline paste, splitting a list mid-item, arbitrary nested blocks, row/column insertion, graphical table layout, inline image positioning and adding/removing an empty image description still use Source. Existing nonempty image descriptions are editable. CRLF list continuation is mapped exactly; general CRLF paragraph breaks stay protected. Source remains the canonical Markdown rather than a serialization of the visual surface.

Physical IME keyboards, VoiceOver reading/navigation and physical display scaling still need hands-on acceptance. Automated QAccessible checks are recorded in Cycle 98. See the [review exercise](../usability/cycle-99.md).
