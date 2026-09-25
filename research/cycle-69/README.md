# Cycle 69 — integrated acceptance

The integrated source passes final `./bin/build` and `./bin/test` with 111 tests, zero failures and zero skips. The stable Dev app was refreshed and checked with a synthetic sample. This is a partial acceptance checkpoint because the native matrix below is not complete. Do not capture private documents, locations or account details.

## Native Dev sample pass (initial)

The stable `dist/Fomawrite Dev.app` was prepared after confirming it was closed; the ordinary `/Applications/Fomawrite.app` was left running and untouched. A synthetic `/private/tmp/fomawrite-cycle69-sample.md` was opened. Accessibility inspection confirmed top-right Bold/Italic/Link/Paragraph/Export controls, a Visual Edit toggle and Source route, and the export hub's PDF/HTML, paper/orientation, style gallery, preview and destination controls. Modern and Classic selections changed the hub's style label and live preview; Modern was restored. The HTML choice exposed Choose CSS and Clear controls.

Visual Edit displayed a safe projection while quote/table source stayed visible. Replacing selected `bold` with `strong` changed only `**bold**` to `**strong**` in the source. Native Undo exposed per-character grouping, so the Dev app was closed normally after restoring/saving the disposable sample; code and a focused regression were then added for one-step contiguous typing Undo. The refreshed Dev-app retest confirmed one Command-Z restored the complete selected `bold` after typing `strong`. No screenshot was saved because the sidebar contained private location shortcuts; only synthetic content and this sanitized text record are committed.

## Final sample pass and remaining work

In the refreshed Dev bundle, editing the body of a simple quote changed only its source text and one Undo restored the original. A top-right Bold action with Visual Edit focused refused to format a stale source selection and displayed a Source route. Modern and Classic output styles changed the live preview; Modern was restored. A disposable `Cycle69 layout check` style was duplicated, immediately selected as Custom, and deleted. The final 245 px gallery displayed full-width Apply and Delete buttons without clipping. The sample document remained saved and clean. The Dev app stays open on that synthetic document for Andrew to review; `/Applications/Fomawrite.app` was neither replaced nor closed.

Automated tests cover conservative mapping, atomic Undo, stale selection protection, bounded user-style persistence and CSS rejection. The native pass did not compare saved PDF/HTML output with the preview, verify user CSS in the final bundle, restart style persistence, test dirty/recovery/multiwindow/external-file workflows, or cover dark/narrow/VoiceOver. No screenshot was committed because the live sidebar included private shortcuts. These are explicit follow-up acceptance checks, not a claim of full WYSIWYG or iA Writer parity.

Final logs: [build](logs/build.log), [test](logs/test.log), [package](logs/package.log). Both generated bundles passed `codesign --verify --deep --strict`; the installed ordinary app was not changed.
