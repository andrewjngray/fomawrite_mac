# Cycle 61 — local checkout path rename

The physical Git checkout now lives in `fomawrite_mac`. Product source and app identities did not change. The move preserved existing uncommitted sample files and Git history. Fomawrite's saved local paths were rebased to the new checkout; private preference backups were kept only in the local temporary directory and are not committed.

[Verification](verification.txt) records the build, tests, native app checks and the Codex project-registration follow-up. No live screenshot was saved because the sidebar displayed private folder shortcuts.
