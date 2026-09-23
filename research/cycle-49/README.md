# Cycle 49 integration record — native recheck pending

The source now aligns the captured Format menu groups and adds ordered tasks and a conservative Clear Styles command. [Synthetic sample](sample/Format-actions.md) and [usability exercise](../usability/cycle-49.md) are ready.

`./bin/build` and `./bin/test` pass after the live-found Clear Styles correction: **80 tests, zero failures**. [Native menu inspection](native-menu.txt) confirmed the menu order and ordered-task/completion/Undo actions in the locally packaged app. The first Clear Styles live attempt found an early-refusal bug; the fix and regression pass in source, but the final app package and native recheck remain pending because both running app instances stopped responding to UI inspection after a QA window close. `./bin/prepare-dev-app` correctly refuses to replace the running Dev bundle. The ordinary packaged bundle predates the final correction; `build/Omawrite.app` is current.

No screenshot was retained from the running apps because their sidebars displayed non-sample file names. No user document was changed. Once the apps are normally closed, prepare the stable Dev bundle, run the sample actions again, capture a sample-only screenshot, then refresh the ordinary bundle before declaring the cycle complete.
