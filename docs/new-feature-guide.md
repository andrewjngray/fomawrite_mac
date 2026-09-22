# Using the closeout build

Start with `examples/Cycle28-QA.md` and disposable copies of your own documents.

- **Go → Command Palette** (Command-Shift-P) searches workspace actions. Focus offers paragraph or sentence focus.
- **Go → Quick Open** searches filenames. Enable saved-file content search for phrases or exact `#tags`; Save query retains its folder and search mode. Results exclude hidden/symlink/build folders and show limits.
- **Format** inserts TOC, highlight, wikilink, simple footnote and content-block syntax. Preview supports documented subsets; source remains Markdown. View optionally synchronizes source/preview scroll by proportion.
- **File → Export HTML/PDF** leaves the source untouched. HTML references local assets via the document folder. View → Output Style chooses fonts; custom JSON is `{"fontFamily":"Georgia","pointSize":12}`. These are basic output styles, not full templates.
- **New Window** now stays in one process. Quit asks about dirty windows in sequence. Window → Merge All Windows creates native tabs; detach retains the document. Window/tab groups are not restored automatically after normal restart.
- **Autosave** is off by default. It applies only to saved files, checks for external changes, and runs once a minute. Recovery snapshots still exist independently. Keep explicit Save and backups for important writing.
- **Create Version of Saved File** checkpoints disk contents using macOS versions. Restore Version replaces the editor text with one-step undo; Save commits it. Unsaved content is not included in the checkpoint, and annotation metadata is not versioned with it.
- **Edit → Check Selection Spelling** reviews selected prose using the system language. Focus → Analyze Selection shows word classes and review words; results are suggestions and do not rewrite text.
- **Authorship Annotations** assigns explicit Human/AI/Reference labels to a selection. They are manual assertions, not automatic source detection. Edits may inherit nearby labels. Save creates a hidden `.NAME.md.omawrite-authors.json` sidecar. Keep it alongside the Markdown; external text edits invalidate it. Duplicate includes current annotations; Rename and Move carry saved metadata and retain unsaved annotations in the editor. Clipboard/export do not yet transport metadata automatically.

The [closeout audit](closeout-audit.md) lists unfinished scope and verification. This build is locally ad-hoc signed, not a public/notarized release.

- **File → Open by Path… (Shift–Command–O):** paste a full local file/folder path, `~/` path or `file:///` URL. Files open with unsaved-change protection; folders open in the library and keep your document. Markdown/text extensions only; relative paths and shell expansions are not supported.
