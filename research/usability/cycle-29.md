# Cycle 29 optional usability check

Use only a disposable sample.

- Mark a phrase as Human or Reference and save.
- Add an unsaved annotation, then Duplicate. Reopen the copy and check both labels.
- Rename and Move the original; confirm the same editor and unsaved state remain.
- Save and reopen; check labels again.
- Try an occupied destination: it should refuse without overwriting existing data.
- Report an unexpected window switch if it recurs; its earlier cause remains unexplained.

Automated: 51 tests pass. Native sample Duplicate, Rename and same-volume Move passed. Full cross-volume/crash testing remains outside this increment.
