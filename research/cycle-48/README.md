# Cycle 48 evidence

Scope: selectable created/modified/hidden dates in the library, with one command state shared by native and in-window menus. See [cycle record](../../docs/build-cycles.md) and [usability exercise](../usability/cycle-48.md).

Build and tests: `./bin/build` passed; `./bin/test` passed with 78 tests and zero failures. `./bin/prepare-dev-app` passed, including strict signature verification. `./bin/package-mac` produced the ordinary locally ad-hoc-signed app. Existing Qt font-alias and SplitView teardown warnings remain.

Native QA: in a separate Dev window, opened only `sample/First.md`. View → View Options → Show Date contained Date Modified, Date Created and None. With `First.md` birth 2025-01-02 and modification 2026-06-15, its row showed 2 Jan for Created, 15 Jun for Modified and no date for None. Sort by Date Modified remained selected. Toolbar library options exposed View Options → Show Date with the same three choices; selecting None there also hid the date. The original window and documents were not edited. [Created](screenshots/created-date.jpg) and [modified](screenshots/modified-date.jpg) crops contain only sample files.

Run `./prepare-sample.sh` after a fresh checkout to recreate the differing file timestamps; Git does not preserve them. Both sample documents are synthetic Markdown.

Remaining live checks: relaunch persistence, context-menu date choice, dark/narrow/VoiceOver, and creation-time-unavailable volumes. Preference migration, persistence and date separation have automated coverage.
