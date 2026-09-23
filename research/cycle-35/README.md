# Cycle 35 — authorship transfer

Markdown copy includes bounded custom authorship metadata and ordinary text/Markdown. Paste validates hash, schema and ranges; external/stale text is unlabelled. Text and annotations paste as one Undo step. Native Cut/Copy/Paste routes and keyboard handling use this path for the editor, keeping ordinary fields unchanged. Metadata can be exported explicitly as JSON; annotation rows select their text for inspection.

Build and 63 tests pass: clipped range round-trip, stale metadata rejection, Undo and export. Native workflows are checked in the combined five-cycle run. Labels remain manual assertions; other applications may drop custom metadata. HTML/PDF exports do not claim provenance; use the explicit hash-bound metadata export with matching Markdown.
