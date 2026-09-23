# Cycle 34b — version restoration safety

Build passed; 61 tests passed, zero failures. Real native file-version creation/restoration is tested on a temporary sample. Tests cover cleared historical authorship, one-step Undo/Redo preserving previous annotations, autosave refusing to write a restored version, recovery metadata retaining the pause, explicit save/reopen and failed Save As preserving the original/draft/Undo.

Native Dev: opened sample/Versions.md; created a version of its saved contents; added disposable draft text; restored via File → Restore Version in Editor; observed original text and the explicit autosave-pause notice; one Undo returned the draft. Quit/Discard removed only that synthetic edit. Screenshot contains synthetic content and prior-cycle sample filenames only.

Policy: NSFileVersion snapshots cover Markdown, not sidecars. Restored text starts unlabelled, and autosave remains paused until successful manual Save, including after Undo. Automatic per-save history and native Versions-browser UI remain future work. Disk-full/device-removal, display/window-state and exhaustive OS shutdown cases remain unverified. Version-restored draft restart is covered by serialized flag checks, not a separate native crash run.
