# Working on Omawrite Mac

This is Andrew Gray's personal Markdown editor, forked from omacom/omawrite.

- Read README.md and docs/roadmap.md before changing product scope.
- Keep the editor calm and local-file based. Build small, useful increments.
- Preserve plain UTF-8 Markdown, undo behaviour, unsaved-change prompts and recovery.
- Retain upstream MIT attribution and the bundled font licence.
- Use QML for interface behaviour and C++ for document I/O and formatting.
- Keep macOS-specific behaviour separate from the Linux implementation.
- Run ./bin/build and ./bin/test for editor changes; verify affected native workflows in the app.
- Never commit build/, build-tests/ or dist/.
- Explain what changed, how it was verified and what remains untested.
