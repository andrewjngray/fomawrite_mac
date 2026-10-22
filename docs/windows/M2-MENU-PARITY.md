# Windows M2 — menu routes and CI synchronization

22 October 2026. This bounded change restores existing desktop actions on Windows and tests their actual menu routes. It does not change the primary Mac checkout/apps or declare full menu parity. Final native CI and packaged-app acceptance remain required; current identities are in [STATUS.md](STATUS.md).

## View and print-preview routes

Windows now exposes the existing sort/filter bars, four sort fields, sort direction, folders-first ordering, three date choices, excerpts, Tree/List navigation and Paginated Preview. Long menus scroll within short windows. Commands retain their shared handlers and the menu restores the prior editing focus, including nested menus.

The former Mac-only `nativeWorkspaceMenusShareState` skip covered portable state, so its assertions now use the real platform menu lookup. Fifteen pointer-driven cases check actual activation, reopened checked state, external state changes, and preservation of a selected unsaved Unicode Source draft and its history. A 900×540 window exercises overflow.

Before the menu change, the new route test failed on missing commands. An intermediate run exposed an actual item extending below the window; bounded menu height fixes that geometry. The focused View run passed 26 tests with no failures; the required build and full local run then passed **366 tests, zero failures and one Windows-only skip**, 526.734 seconds. Logs: `/tmp/fomawrite-menu-view-red.log`, `/tmp/fomawrite-menu-view-first-green.log`, `/tmp/fomawrite-menu-view-green.log`, `/private/tmp/fomawrite-menu-view-full.log`. The intermediate aborted attempt is preserved.

## Clipboard and search routes

A separate bounded follow-up restores three Copy formats, two Paste As formats, Delete and Find Next/Previous/Selection using the existing focus-aware handlers. Windows-specific direct field manipulation in the tests is replaced with the real menu route. Unicode selection, one-step Undo, auxiliary-field isolation, disabled/no-match state and Preview-only editing rejection remain required. The new route test failed before the menu additions (5 passes, one missing-Copy-Markdown failure; `/private/tmp/fomawrite-edit-menu-red.log`). The final focused batch passed **30 tests, zero failures/skips**, 103.258 seconds (`/private/tmp/fomawrite-edit-menu-final-green.log`), including actual Qt pointer activation and real Live-page clipboard operations. The unchanged new body then moved into `tests/windows-menu.inc`; its full integrated rebuild/suite is running.

An existing shared Mac/Windows limitation remains: Source Paste As uses TextEdit.canPaste and is disabled for an HTML-only clipboard. The route test uses real dual HTML/plain data with deliberately different content to prove HTML conversion, while the separate HTML-only Backend conversion assertions remain. This is not claimed as HTML-only menu support.

The empty Library hint now says “this computer”; the ordinary Windows capture exposed the inherited “your Mac” wording. This is a text-only correction.

## Two failed CI observations

The exact previous candidate's x64 CI failed the old-page cursor witness and the footer final-clean assertion; shared Mac Qt 6.11.3 failed the same footer assertion. [Exact previous results](M2-DESKTOP-6558.md).

The footer fixture previously waited only for geometry after enabling the lazy Live pane. It could ask to return to Source before the actual browser document was ready, triggering the fail-closed recovery path. It now requires a genuine bridge handshake and matching full-text reply before the view-only sequence. Final disk-byte and clean-state assertions remain, with an additional zero-save-failure assertion.

The stale-renderer fixture previously used a debounced cursor event from the hidden browser as delivery proof. Hidden-page timers can be throttled and lose to automatic renderer retry. The corrected fixture primes the existing DOM input observer and performs the same real page edit, then witnesses the production dispatch metric sent immediately after the delta on the same channel. It asserts that no delta reached the Source document and that Source remains unchanged and clean. It fabricates no readiness or text reply.

Both old fixtures passed locally on Qt 6.11.2; the retained native/shared CI failures are the failed-old-fixture evidence. The corrected focused local run passed **4 tests, zero failures**, 12.319 seconds (`/private/tmp/fomawrite-ci-readiness-focused-green.log`). These are test synchronization corrections, not new production fixes or a claimed local production red/green. Native/shared Qt 6.11.3 reruns remain essential.

The ordinary input driver also now retains full exception chains, HRESULTs and stack traces on failure. Input, deadlines and acceptance checks are unchanged. This preserves evidence for the earlier unexplained x64 second-launch Exit lookup failure.

## Scope remaining

The prepared ordinary desktop supplement must run against the exact new package and menu-source hash. It covers physical nested choices, retained selection/edit/Undo/Save/reopen, and opening/cancelling the native Paginated Preview dialog. Preparation or unit checks do not count as a Windows desktop pass. Other absent portable routes and genuine Mac-only capability exclusions remain explicit in the [parity ledger](PARITY-LEDGER.md).
