# Cycle 128 — correct document previews

Review build **0.3.0-dev24 / macOS 0.3.0 (128)**.

Andrew reported a blank PDF in the ordinary app and the previous Ulysses document under a Bain document header in Dev. A read-only inspection found that Dev had generated the new Bain PDF while retaining the old PDF open. Cycle127's passing checks did not establish that the generated PDF was the PDF actually displayed.

## Repair

Removed screenshot-gated replacement and the retained screenshot overlay. Document lifetime, source digest and caller revision now identify requests/results; changing documents cancels pending work and clears visible output immediately. Late callbacks cannot restore a previous document. Cached PDFs restore page delegates even when their already-loaded URL emits no status transition. Empty Web output hides Chromium immediately. A 30-second loading deadline reports failure with Reload Preview recovery instead of leaving generation/loading silently pending. Missing-image placeholders remain; canonical Markdown, saving and Undo are unchanged.

## Verification

- `./bin/build` succeeds. `./bin/test`: **202 passed, 0 failed, 0 skipped**.
- Five real-component regressions inspect actual Qt PDF sources and visible page images: cold PDF, replacement without compositor frames, cross-document stale callbacks, rapid/mode/hidden changes, and timeout recovery. Same-URL PDF/Web/PDF bounce and timeout/cache reuse are included.
- Before the repair, component checks failed on cross-document invalidation and replacement without compositor frames. This is red/green evidence, not only added passing assertions.
- Backend coverage includes same-directory file switches, caller token rebinding, explicit cancellation and harmless format-only revision changes.
- The final candidate passes **163 native checks**, including **8 actual-pane identity/paint observations**. Saved synthetic A/B/C documents use distinctive colored markers. Readiness polling never calls `grabWindow` or requests frames; a passive `frameSwapped` observer confirms natural presentation before one screenshot. The PDF pane's marker pixels are compared with its actual source PDF. Web checks inspect the actual DOM and marker pixels.
- Native coverage includes PDF A→B, an in-flight A/B/C switch, hidden/show, theme/Web/PDF changes, and a fresh PDF component. This is synthetic Cocoa automation, not physical mouse/keyboard acceptance or a full persisted multi-window application relaunch.
- Bundle verification and deployment status are recorded in `verified-builds.json`. Raw synthetic captures/reports remain local under this directory; private writing/themes are not committed.

## Remaining acceptance

The fresh-component startup check does not substitute for Andrew's persisted workspace relaunch. Physical input, arbitrary themes/documents, multi-display/VoiceOver and long-running use remain unverified. Replacing a busy app waits for normal closure and preserves unsaved-work prompts. Chromium can emit compositor diagnostics during the broad harness; the visible-content assertions and QML warning gate are recorded separately.

Optional review: switch between two distinct saved documents in PDF mode, toggle Web/PDF quickly, then quit and reopen the ordinary app. Check that heading and preview always agree. [Checklist](../usability/cycle-128.md).
