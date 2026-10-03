# Cycle 100 — Heading links between open documents

Scope: close the bounded navigation gap in the earlier Cycle 58 acceptance ledger. A local heading link should reach its target even when that file is already open in another window or native tab. A missing heading should give readable feedback.

## Changes

- Preserve the fragment through ordinary open routing, explicit new-tab/new-window requests and native window creation.
- Resolve the heading against the destination window’s live Markdown, including unsaved headings, without reloading either document.
- Show a dismissible, nonmodal message for missing headings or invalid heading addresses; failed navigation retains the current caret.
- Keep the existing unsaved-source prompt before transferring focus. Cancel leaves both documents unchanged.
- Cancel stale queued preview-render completions before they can consume a newer pending heading. This prevents false missing-heading notices during rapid document/preview refresh.

## Verification

The regression definitions in `tests/cycle100-navigation.inc` cover duplicate headings, encoded Unicode, unsaved target headings, missing/invalid targets, source selection, canonical Undo, source Cancel, and explicit view fragment preservation. `tests/cycle100-navigation-native.inc` exercises the production session manager with three native views and disposable files. The integrated native session fixture passed (111 total assertions across Cycles 98 and 100), including the cross-window Unicode heading, dirty-target preservation, missing-heading feedback, source Cancel, and Undo checks. Evidence: [native workspace log](../cycle-98/native-workspace.log). Both focused navigation slots passed after the integrated fixture exposed and fixed a stale-preview callback race (4 passes including setup/cleanup, zero failures/skips). The aggregate regression result will be recorded by the integrated build run.

## Limits

This cycle does not claim broader Markdown parser or cross-application clipboard parity. Heading navigation uses Fomawrite’s existing slug rules. Physical multi-display navigation and all external URL providers remain outside this bounded check.

## Review

Open two local documents. In one, follow a Markdown link to a heading in the other; try a heading that exists only in the other window’s unsaved text. Then follow a missing heading and confirm the message appears without moving the caret. Retry from a dirty source and Cancel.

## Integrated verification

The final build passes all 134 regression tests (zero failures/skips), including both navigation slots. Native Cocoa rerun passes the heading workflow alongside Visual Edit. See [Cycle 99 tests](../cycle-99/tests.log), [native Visual Edit/navigation log](../cycle-99/native-visual.log), and [111-assertion workspace run](../cycle-98/native-workspace.log).

## Local artifacts

Application **0.3.0-dev2**, bundle **0.3.0 (100)**. The demo/package `dist/Fomawrite.app` and `/Applications/Fomawrite.app` are refreshed, strict-signature verified and have identical executable hashes. The previous Applications copy is backed up under `/private/tmp/Fomawrite-before-build100-20261003-195432.app`.

The stable Dev app canceled its normal quit request. Its running bundle remains unchanged; the verified build-100 replacement is staged at `/private/tmp/Fomawrite-Dev-build100-ready.app` until Andrew saves/closes Dev. It has the same stable Dev identifier and has not been launched or registered as another app. Exact installed/staged versions and hashes: [artifacts.json](artifacts.json).

The generated bundles are excluded from Git. Public releases and notarization are unchanged. The source push is recorded in Git history for this cycle.
