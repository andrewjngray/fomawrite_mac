# Cycle 155 — Windows Go and Open Recent menu candidate

22 October 2026. Bounded continuation from Focus/chrome checkpoint `ecf777d6c054446cef4635fc56d194b6e8bba4eb`. The required local build and three focused regression cases pass. Complete local regression, native red/green, exact CI and packaged qualification remain pending; current build identities stay in [Windows STATUS](../../docs/windows/STATUS.md).

## Change

Windows File → Open Recent and Go → Smart Folders → Recents expose the existing recent-file model through one inline Qt Quick Controls component. Labels retain document paths, unavailable rows are disabled, and empty/clear states track the shared library. Each open snapshots its URL and calls `win.requestOpen` through the existing menu focus dispatcher.

Go exposes the existing document Back/Forward, Library Back/Forward/Enclosing Folder, Open Link, Quick Search, Command Palette, Locations, Smart Folders and Hashtags routes. Locations, saved queries and hashtags use live model delegates. Existing labels, enabled states and root-validation behavior follow the native menu. All Windows popups inherit bounded height, nested focus ownership and editing-focus restoration.

Document navigation keeps `requestOpen`/`requestHistory`, including the Live document barrier and unsaved-change decision. Library navigation remains independent of document history. No document backend, persistence, search or native-Mac menu behavior changes. The native menu's Command Palette shortcut is not added to Windows by this menu-only slice.

## Existing regression cases tightened

| Case | Actual menu coverage required by the candidate |
| --- | --- |
| `goMenuRoutesSavedQueriesHashtagsAndGuardedRecents` | Actual Location, saved-query, New Smart Folder and hashtag triggers; query creation/removal updates; missing-root refusal; disabled unavailable recents and both File/Go recent instances. Windows absence/registry substitutes are removed. |
| `navigationHistoryAndQuickOpenStayIndependent` | Actual document and Library history, Enclosing Folder, Quick Search, Command Palette and Open Link routes; restored caret, unsaved Cancel, failed-history open preservation, invalid-link disabling and independent folder/document state. |
| `recentMenuTracksLibraryAndGuardsOpen` | Actual recent opening on both platforms; both recent routes reach the unsaved prompt; Cancel retains selected draft and Undo; a disappeared target cannot discard the draft; Clear Recent Files updates both menus. |

The new native regression witnesses are the required `saved`, `back && forward`, and `recent` menu nodes. The optional proof script now includes these three cases alongside the five Focus/chrome cases and derives all counts from its expected-case map. With baseline `0e9ef4a71d0c43e21440d9efe0c1d1110998d6c5`, it requires eight specific missing-node failures plus passing init/cleanup, restores and verifies original menu bytes in `finally`, then requires all ten results to pass. Workflow labels identify Focus/chrome/Go; an empty optional baseline input still skips proof. This native red/green has **not run**. The local Mac pass verifies its native menu path and does not constitute Windows red evidence.

## Verification and limits

- Required `./bin/build`: passed, complete log retained locally as `evidence/build.log`.
- Focused `./bin/test goMenuRoutesSavedQueriesHashtagsAndGuardedRecents navigationHistoryAndQuickOpenStayIndependent recentMenuTracksLibraryAndGuardsOpen`: **5 passed, 0 failed, 0 skipped**, including init/cleanup, **3.250 seconds**, Qt 6.11.2 ARM64/macOS. Complete build/test output: `evidence/focused-green.log`.
- The first focused attempt was **4/1/0**, 18.041 seconds: the real Go handler recorded the second document's visible caret at 15 before Main's queued document-load reset. The former direct Backend route had implicitly used stored position 0. The fixture now waits for the actual reset before the first Go activation and retains the 7/0 restored-caret assertions. No product code or navigation guard changed for this timing correction. Failed output is retained in `evidence/focused-first-failed.log`; it is not Windows production red evidence.
- `WindowsMenuBar.qml` passes QML parsing and bytecode generation; workflow YAML parses; `git diff --check` is clean. These checks do not execute Windows behavior.
- Complete local regression remains pending and must be coordinated with the clipboard qualification's quiet-desktop requirement. No full suite was run as part of these focused checks.
- Windows ARM64/x64 execution, native red/green and ordinary packaged popup/input qualification are pending. Menu-node signal dispatch does not certify physical nested-menu activation or scrolling.
- Add Location's picker is exposed through the existing handler; its actual native picker flow remains packaged follow-through.
- Primary Mac checkout/apps, Focus/chrome test bodies, root STATUS and Windows STATUS are outside this slice. Only the native-proof map/counts and descriptive workflow labels extend beyond the Go/menu tests.

Completed local logs are retained under this cycle's ignored `evidence/` folder. This record is a candidate description, not a completion or package-promotion claim.

## Bounded UI Automation provider recovery

A separate driver correction handles the observed x64 final-Exit `E_FAIL (0x80004005)` as well as the previously handled `RPC_E_DISCONNECTED`. It discards the partial traversal and reacquires the same owned window, with at most two retries inside the unchanged 4.5-second budget. Access-denied, changed owner/window, incomplete results and other errors remain fatal; input is never replayed. Each retry logs its HRESULT.

Native non-UI helper proof failed on the previous helper at the new provider-E_FAIL case and passed **13/13** with this change. Evidence: `dist/windows-testing/results/diagnosis/uia-efail-proof-0e9/`. Old helper SHA-256 `264046b08314c69b71345534d042d82db948c1cced1a2b835562b98ba25b047c`; new helper `32871e17dc095c89513feed25224009554be3d236318919694f9b94f3e140e92`. The subsequent exact `0e9ef4a` x64 ordinary startup/restore gate passed independently; it does not assert that an E_FAIL retry occurred during that successful run. Earlier rejected observations are retained in the [desktop checkpoint](../../docs/windows/M2-DESKTOP-0E9.md).
