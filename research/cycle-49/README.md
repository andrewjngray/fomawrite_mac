# Cycle 49 evidence

The source now aligns the captured Format menu groups and adds ordered tasks and a conservative Clear Styles command. [Synthetic sample](sample/Format-actions.md) and [usability exercise](../usability/cycle-49.md) are ready.

`./bin/build` and `./bin/test` pass after the live-found Clear Styles correction: **80 tests, zero failures**. [Native menu inspection](native-menu.txt) confirmed the menu order and ordered-task/completion/Undo actions in the locally packaged app. The first Clear Styles live attempt found an early-refusal bug; after the fix, the refreshed Dev app changed `## **Styled sample**` to `Styled sample`, then one Undo restored it. The sample was saved with its original bytes. [Cropped screenshot](screenshots/clear-styles.jpg) contains only the synthetic document. `./bin/prepare-dev-app` and `./bin/package-mac` refreshed both stable app identities; strict signature verification passed.

No user document was changed. The UI inspection tool timed out when closing disposable QA windows in both app identities; this did not block the later sample recheck, but the cause of those timeouts remains unverified. The earlier [native menu log](native-menu.txt) records the pre-fix refusal and is retained as the bug-finding evidence.
