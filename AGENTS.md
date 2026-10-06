# Working on Fomawrite

This is Andrew Gray's Markdown editor, branded Fomawrite and forked from omacom/omawrite.

- Read MISSION.md first; it is the only statement of what the product is for and what counts as progress right now. Then README.md, STATUS.md, ARCHITECTURE.md and docs/roadmap.md before changing product scope.
- Keep the editor calm and local-file based. Build small, useful increments.
- Preserve plain UTF-8 Markdown, undo behaviour, unsaved-change prompts and recovery.
- Retain upstream MIT attribution and the bundled font licence.
- Use QML for interface behaviour and C++ for document I/O and formatting.
- Keep macOS-specific behaviour separate from the Linux implementation.
- Run ./bin/build and ./bin/test for editor changes; verify affected native workflows in the app.
- Never commit build/, build-tests/ or dist/.
- Explain what changed, how it was verified and what remains untested.
- Use docs/ia-writer-inventory.md as the reference backlog; distinguish implemented behavior from partial support and future work. Do not claim feature parity from matching menu labels alone.
- Save cycle screenshots and verification logs under research/cycle-NN/ and keep research/usability/ checklists current. Use sample documents and exclude private writing from committed screenshots.
- Reuse `dist/Fomawrite Dev.app` and bundle ID `io.github.andrewjngray.fomawrite.dev` for native QA; prepare it with `./bin/prepare-dev-app` after building. Do not create per-cycle app identities. Close the QA app normally before replacing it, preserving unsaved work. Announce app-access requests before attaching, and let Andrew grant any Always allow permission himself. See docs/development-app-approvals.md.

## What a cycle is

A cycle is one bounded change that leaves `master` buildable and green. It consists of exactly these parts:

1. **Code.** The source change itself, kept to the cycle's stated scope.
2. **Tests.** Every behavioural fix ships with a regression test that **fails on the previous code and passes on the new code** (red/green, as Cycles 131 and 132 did: stash the source fix, keep the test, confirm the failure at the defect, restore the fix). A test that also passes the old implementation is not evidence for the fix; say so if that is all that is possible. The regression count in STATUS.md must not go down.
3. **`research/cycle-NN/README.md`.** The cycle's record: build identity, what changed and why (table of finding → behaviour where applicable), verification (counts, red/green result, native checks actually run) and limits. Commit only reviewed summaries and `verified-builds.json`; `.gitignore` keeps everything else in `research/cycle-*/` local.
4. **One line in `CHANGELOG.md`**, newest first, linking to that record.
5. **Update `STATUS.md`**: build identity, last cycle, test count, bundle state, and the verified/open lists. STATUS.md is the only place the current build is recorded; do not restate it in README.md, the roadmap or the ledger.

Do not append to `docs/archive/build-cycles.md`; it is frozen at Cycle 132. Keep `ARCHITECTURE.md` current when a cycle moves responsibilities between files, adds a module or changes a documented model (persistence, publishing, `.inc` conventions). `DEVELOPER_HANDOFF.md` and `INDEPENDENT_REVIEW.md` are dated snapshots: fix broken links in them, do not rewrite their content. CI (`.github/workflows/ci.yml`) is the build/test evidence; a cycle is not complete while it is red.
