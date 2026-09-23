# Working on Fomawrite

This is Andrew Gray's Markdown editor, branded Fomawrite and forked from omacom/omawrite.

- Read README.md and docs/roadmap.md before changing product scope.
- Keep the editor calm and local-file based. Build small, useful increments.
- Preserve plain UTF-8 Markdown, undo behaviour, unsaved-change prompts and recovery.
- Retain upstream MIT attribution and the bundled font licence.
- Use QML for interface behaviour and C++ for document I/O and formatting.
- Keep macOS-specific behaviour separate from the Linux implementation.
- Run ./bin/build and ./bin/test for editor changes; verify affected native workflows in the app.
- Never commit build/, build-tests/ or dist/.
- Explain what changed, how it was verified and what remains untested.
- Keep docs/build-cycles.md current for every feature cycle: planned scope, changes, tests, native verification, known gaps, runnable artifact and a short optional usability exercise for Andrew. Incorporate his feedback into the next cycle.
- Use docs/ia-writer-inventory.md as the reference backlog; distinguish implemented behavior from partial support and future work. Do not claim feature parity from matching menu labels alone.
- Save cycle screenshots and verification logs under research/cycle-NN/ and keep research/usability/ checklists current. Use sample documents and exclude private writing from committed screenshots.
- Reuse `dist/Fomawrite Dev.app` and bundle ID `io.github.andrewjngray.fomawrite.dev` for native QA; prepare it with `./bin/prepare-dev-app` after building. Do not create per-cycle app identities. Close the QA app normally before replacing it, preserving unsaved work. Announce app-access requests before attaching, and let Andrew grant any Always allow permission himself. See docs/development-app-approvals.md.
