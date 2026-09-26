# Cycle 87 — bounded Visual Edit paragraph breaks

Visual Edit's Return key now adds a blank Markdown line in a simple mapped paragraph; Shift-Return adds one line break. The mapping constructs a candidate source and accepts it only if the new visual projection equals the old projection plus the intended break. It rejects protected list and heading markers, CRLF documents, source-only blocks and edits that would expose or alter Markdown syntax. A rejected break leaves the source unchanged and tells the writer to use Source.

The tests cover direct source mapping, a real Return-key event, list rejection and one-step Undo. The original source editor remains canonical. Existing safe list/task/quote body editing continues; creation of new structured blocks and editable image cards is not claimed. This limitation is visible in the app and recorded in the [editor plan](../../docs/editor-surface-redesign.md).
