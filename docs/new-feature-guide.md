# Using Omawrite Mac 0.2.0 RC1

Open the packaged `dist/Omawrite.app` for everyday use or `dist/Omawrite Dev.app` for QA. Both edit real local UTF-8 files. The archive contains an Apple Silicon app requiring macOS 14 or later, with bundled Qt and an ad-hoc signature.

## Find and organize

Open by Path (Shift-Command-O) accepts absolute paths, ~/ paths and file URLs. Opening a folder changes the library without replacing the document. Go → Quick Open searches bounded saved-file contents and exact #tags. Save query adds a Smart folder in the organizer. Tags → Refresh scans the current folder; counts are numbers of documents, not occurrences. Unsaved buffers, hidden/symlink/build folders and oversized files are excluded.

## Write and review

Aa → Theme offers System, Light, Dark and Warm paper. Go → Command Palette (Shift-Command-P) includes document, export, theme and review actions. Command-S saves the active document; Undo/Redo retain normal editing behavior.

Edit → Spelling and Grammar checks the first 50,000 characters using a selected macOS dictionary. Review each suggestion, choose Replace, and Undo if needed. Code and link destinations are masked. Grammar is optional and language-dependent. Focus → Writing Review shows system word classes and review words, refreshing while open; it does not automatically rewrite prose. Speak Selection / Stop Speaking use the system voice.

## Read and export

Preview supports tables, local images, ordinary Markdown, repeated/multiline footnotes and bounded local Markdown/CSV/code/image blocks. Long-note links jump to their rendered destination. The parser is a documented subset, not complete iA syntax compatibility.

File → Paginated Preview offers page navigation and fit-page/fit-width. View → Output Style chooses persisted export typography. Custom JSON supports `fontFamily`, `pointSize` (8–32), `header`, `footer` and `titlePage`; header/footer tokens are `{title}`, `{page}` and `{pages}`. File → Insert Page Break inserts `<!-- pagebreak -->`. HTML embeds bounded local raster assets so the images travel with the file; missing assets reject export without replacing an existing output.

## Save, restore and authorship

Autosave and Keep Previous Version on Save are opt-in. Previous versions capture saved Markdown before replacement, not unsaved buffers or authorship sidecars. Restore Version pauses autosave until explicit Save; one Undo restores previous text and labels. Recovery snapshots are separate from saved-file versions.

Authorship Annotations assigns manual Human/AI/Reference labels. Save writes a hidden `.NAME.md.omawrite-authors.json` sidecar. Keep it beside its Markdown. Omawrite Markdown Copy/Paste carries validated annotation metadata; other apps may strip it. Plain external text is unlabelled. Export Authorship Metadata writes a separate hash-bound JSON file; HTML/PDF do not embed assertions. Labels are not verified provenance.

Saved-file windows restore their tabs/cursor/library root, normal bounds, screen name and window state. Unavailable display placement is clamped to a current screen. Quit gathers all dirty-document decisions before closing any window, so Cancel retains drafts.

See [the acceptance ledger](release-acceptance.md) for unverified hardware, accessibility and remaining parity work. This is a release candidate, not a notarized public release or completed parity sign-off.
