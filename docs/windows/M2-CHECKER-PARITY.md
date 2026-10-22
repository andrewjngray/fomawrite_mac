# Windows M2 — writing checker regression parity

22 October 2026. Windows branch only; primary Mac checkout and installed apps are untouched.

## Change and findings

Twenty-eight existing Mac regression cases now also run against the shipped Harper JavaScript/WASM engine: 13 Review cases, six Source/Live spelling cases and nine grammar cases. Native macOS rows and their assertions remain. Windows uses real bundled-engine rows rather than skipping the whole feature. Windows menu assertions target the actual Windows menu objects.

The new coverage exposed a product bug: Source already masks Markdown syntax before requesting spelling, but Harper parsed that result as Markdown a second time. Four-space nested-list prose was mistaken for code and its spelling findings disappeared. Host requests now use plaintext; the public raw-Markdown helper retains Markdown parsing.

The host adapter normalizes square brackets only in temporary lint input, preserving UTF-16 offsets. It rejects whitespace-only findings and corrections crossing original bracket punctuation. Source masks link destinations/reference IDs and footnote IDs while preserving visible-label and footnote boundaries. Tests cover inline, empty, full-reference, collapsed and shortcut labels, escaped brackets, emoji offsets, snake_case, code exclusions and correction safety. Document bytes are never normalized by this adapter.

Harper uses a whole-request cache. Its long-text test verifies actual bridge request reuse and correct late-paragraph offsets; native sentence-chunk timing checks remain native-specific. Different dictionaries use proven engine-specific misspellings where needed, without fabricating readiness or findings.

## Local evidence

- Spelling: 14 passed, 0 failed/skipped (six cases × native/Harper plus setup/cleanup), 25.4 seconds.
- Review: 28 passed, 0 failed/skipped (13 cases × native/Harper plus setup/cleanup), 49.8 seconds.
- Grammar: 20 passed, 0 failed/skipped (nine cases × native/Harper plus setup/cleanup), 23.1 seconds.
- Editor: 324 passed, 0 failed/skipped; typecheck and committed asset build passed.
- Red/green: the real bundled engine returned no findings for nested-list prose before the fix, versus expected UTF-16 starts 18 and 37. A second regression rejected normalization-created formatting and bracket-crossing grammar corrections. Final real-WASM tests pass.
- Independent review checked Source masking, public Markdown behavior, real engine usage, offset preservation and correction safety; no remaining material blocker was reported.

Local logs are `/tmp/fomawrite-source-spelling-final.log`, `/tmp/fomawrite-review-plaintext-final.log`, `/tmp/fomawrite-grammar-portability.log`, `/tmp/fomawrite-editor-plaintext-tests.log`, `/tmp/fomawrite-harper-plaintext-red.log` and `/tmp/fomawrite-harper-bracket-red.log`. These temporary logs are local evidence, not durable distribution artifacts.

The first full integration run built successfully and returned **339 passed, 1 failed, 1 skipped**. The failure was an existing tag-watcher test accepting any non-Scanning status, including a pending debounced refresh. Its isolated recheck passed; the affected three watcher cases passed after tightening initial readiness (5 including setup/cleanup). The final wait also requires the expected final tag, so a completed older scan cannot satisfy it. No FileLibrary production code changed. The original failure remains in `/tmp/fomawrite-parity-suite.log`.

The final full-suite rerun, Windows CI and packaged VM validation are pending. Existing shelf builds remain identified in [Windows status](STATUS.md); they do not contain this change until a new exact package is qualified.

## Remaining gaps

The ten Harper updater cases still represent an unsupported Windows capability, not a genuine Mac-only exclusion. The native Mac menu case is platform-specific. Wider ordinary desktop acceptance and installer qualification also remain open. Passing these new checker cases is not a claim of full Windows parity.
