# Cycle 14 — native workspace menus

17 September 2026. Implementation and automated checks complete; native verification pending safe Dev-app refresh.

## Implemented

- Native View menu: library/organizer, sort/filter bars, sorting and direction, folder pinning, dates/excerpts, text size, visible Markdown syntax, preview show/reload, Editor/Split/Preview layouts, preview typeface, outline, statistics and fullscreen.
- Native Focus menu: paragraph focus and typewriter scrolling, using existing independent settings. No sentence-focus or linguistic placeholders.
- Native Format: Strikethrough and Inline Code, also retained in More.
- Shared workspace command registry for native menu adapters, toolbar/writing controls and library menus. Persisted state stays in the existing Settings/library objects. Organizer action is disabled while the library is hidden or window is narrower than 1,000 px.
- Formatting selection wrapping now uses a C++ QTextCursor edit block. A new regression check exposed the existing remove/insert operation’s two-step undo; wrapping now undoes as one operation. Bold/italic share this fix.
- Reload Preview forces a new Markdown parse without changing source. Preview typefaces are labelled as typefaces, not full output templates.

## Verification

- `logs/build.log`: build passed.
- `logs/test-native-access.log`: 28 passed, 0 failed, including native menu/settings synchronization, exclusive sort state, narrow-width enabled state, size limits, source/undo preservation across presentation changes, forced preview parsing, and formatting selection/undo/redo.
- `logs/test.log`: initial restricted run; the existing file-watcher test failed in the sandbox. It passed with normal macOS access. Existing Qt Material teardown warnings remain in the dialog test.
- No new keyboard shortcuts were registered; existing shortcuts are retained, so this cycle introduces no shortcut reassignment/collision.
- Packaging and deep/strict signature verification passed; results are recorded in `logs/package.log` / `logs/signature.log`.

## Native blocker and limits

App attachment returned `noWindowsAvailable`, then `timeoutReached`. A process check confirmed the existing stable Dev app was still running. `bin/prepare-dev-app` correctly refused replacement (exit 2); see `logs/prepare-dev-app.log`. Andrew was asked to save and quit it normally. No termination, document save/discard or bundle replacement was forced.

Consequently there is no new native screenshot and no claim of native visual/menu, keyboard, restart-persistence, dark-mode, local-image reload or fullscreen-transition verification. Cycle 13 visual closeout also remains pending. Automated coverage verifies state and document behavior, not the on-screen AppKit result.

## Runnable artifact and next check

`dist/Omawrite.app` is the Cycle 14 packaged artifact. `dist/Omawrite Dev.app` remains the older running build until safely refreshed. After it closes: run `./bin/prepare-dev-app`, open a disposable sample, and follow [the checklist](../usability/cycle-14.md). Capture only sample content; organizer recents may contain private filenames.

## Follow-up in Cycle 15

The old clean Dev window was closed normally and the stable bundle refreshed on 17 September. Native View menu inspection succeeded; it exposed a duplicate Full Screen item, corrected in Cycle 15. Complete visual/dark/narrow/keyboard checks remain outstanding and screenshots were unavailable. The running Dev bundle is now Cycle 15, superseding the earlier blocker above.
