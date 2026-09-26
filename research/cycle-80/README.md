# Cycle 80 — Source button leaves Visual Edit

In Split view, Source previously set the layout to Split and focused the existing source editor, leaving the visual pane unchanged. That made the click appear inert. It now disables Visual Edit, selects Editor Only through the shared workspace command, and focuses the source editor. No document rewrite occurs.

`./bin/build` passed. `./bin/test` passed 113 tests with zero failures and skips, including a focused QML regression on the document pane's Source button. Both packaged bundles passed strict local signature verification. The refreshed Dev app launched on `/private/tmp/fomawrite-cycle79-ui-sample.md`; the ordinary Applications app remained running and untouched. No private-screen capture was made.

Andrew should check the physical click in the refreshed Dev window. The visual editor's supported-source subset and broader acceptance gaps remain as recorded in Cycles 69 and 79.
