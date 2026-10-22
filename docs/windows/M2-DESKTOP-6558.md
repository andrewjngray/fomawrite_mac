# Windows stability checkpoint — 6558d35

22 October 2026. Candidate `6558d3505b8cff0284f3dc3cc0e1c6b92a1336fc`, Qt 6.11.3, ordinary Parallels Windows 11 ARM64 desktop at 200% scaling. CI failed on two assertions described below; this record does not promote the testing shelf.

## Ordinary ARM64 input

The unchanged six-stage normal-launch gate passed and its independent verifier passed. Evidence: `dist/windows-testing/results/final-6558d35-normal-704ff3598af3454d873292eb296c4761/normal/`; report SHA-256 `e909bb89e5e2f8cb35e0a5bad7d54e3b253a5ad438e4ca504870548bb47b5671`.

All five maintained Live cases passed: settled, immediate, 50 ms, 100 ms and 150 ms. The independent verifier reports full acceptance for this driver. Actual rapid dispatch gaps were 0.0000417, 52.774, 104.216 and 151.268 ms. Each case checks exact disk bytes, clean title, undo/redo and reopening, and binds 39 evidence hashes. Evidence: `dist/windows-testing/results/final-6558d35-live-4efd889c98464e3dacea6e67a7981b70/live/`; series SHA-256 `f4a721638f5825251002a868719cef9d07ee7793987fbaea31dda72c093bde00`. This is the packaged green repetition of the rapid-save defect preserved on d573. [Driver scope](LIVE-EDIT-ACCEPTANCE.md).

## Publishing and typography

The ordinary publishing driver passed all nine stages: A/B document changes, Web/PDF, Modern/Classic style changes, reload, actual native HTML/PDF exports, unchanged Markdown and normal exit zero. Root independently verified all 63 evidence-file hashes and reviewed all eight publishing/format captures. Document identity, selected controls and continuous-Web versus paginated-PDF layout were correct. Modern now visibly uses sans-serif; Classic uses serif. [Driver scope and limits](PUBLISHING-ACCEPTANCE.md).

Evidence: `dist/windows-testing/results/publishing-6558d35-35ead7e9afea4abfba8e78d147ccbfbd/evidence/`. Report SHA-256 `54a43e27689bd59057ebc93ba7e3eabad1ddafcca5583c1d0a278854fbecbb55`. The separate `root-visual-review.json` records visual acceptance without overwriting the driver's pending-review flags. Parallels returned wrapper status 255 after the completed evidence copy; the application itself exited zero. The log retains an unknown vendor warning and one compositor-null-texture warning. This bounded workflow is not a flicker-endurance test.

## Package identity and remaining work

ARM64 executable SHA-256: `62207e928df29e648793a0099732f91f1d674389ddfa0ad8f8b8d50f1019937c`; manifest SHA-256: `c5a04e2b51c35482524be2198e26bd067a780b15c83520068f6609ce2256754b`. All 1,608 package files and PE ARM64 identity were verified before Windows-local staging. Original Qt archive checksum pins were retained through the official mirror fallback.

## CI and x64 limits

[Windows CI 39760440078](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39760440078) failed overall. ARM64 passed **323 Qt tests, zero failures, one skip**. x64 passed **321, failed two, skipped one**: the old-page cursor delivery witness in `livePersistenceMismatchAndTimeoutFailClosed`, and the final clean-state assertion in `footerControlsAndMenusRemainReachableWhenResized`. Both architectures nevertheless completed three native appearance checks and 124 deployed scenarios without failures. Those later steps do not turn the failed Qt job green. The remaining menu skip covers portable behavior and is being replaced with Windows menu coverage.

[Shared CI 39760440072](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39760440072) also failed the same footer clean-state assertion: **338 Mac Qt passes, one failure, 13 skips**. Editor tests and committed-bundle verification passed. These failures must be diagnosed before promotion; the earlier passing local run cannot supersede them.

The first x64 ordinary-input trial passed the six workflow stages and clean first exit, then failed accessibility lookup of Exit on the second launch. Its failure dump contained a visible, enabled Exit item. The retained message did not identify the inner exception, so its cause remains unexplained. Evidence: `dist/windows-testing/results/final-6558d35-x64-normal-745932c8393749fdb87c45c41065bc7c/normal/`.

One repeat with exception-chain logging added passed the unchanged workflow and strict independent verifier. No input, search deadline or assertion was changed. Evidence: `dist/windows-testing/results/final-6558d35-x64-normal-25f8496e0f1a44efbd1ab5d4cf9fb855/normal/`; report SHA-256 `b8329ecede99e575b280cebbc3089f861aeec388f15b3eef52eaa5b9db3981c7`. x64 ran under ARM64 emulation; physical x64 hardware is untested. The initial failure remains a limitation, not a discarded attempt.

The first updater desktop trial disabled automatic checking through the real Help menu and verified the native setting before its scheduled check. It stopped before manual update because the Review pane was missing from the accessibility tree. Independent screenshot review showed the actual pane and built-in Harper 2.10.0 label. This is an observation failure, not proof of a broken updater or a passing update. The corrected observer uses owned Review-region OCR. Subsequent retained attempts exposed transient menu-readiness, a redundant reopen interaction, label elision and a restored publishing pane affecting crop geometry; those observation preconditions were corrected without product changes or relaxed engine assertions. The final result below passed. Evidence: `dist/windows-testing/results/updater-6558d35-bf8ac8a672fa49418ffb4c0d97a128e6/`.

The separate View-menu slice is not in this binary. Broader desktop affordance gaps remain tracked in [PARITY-LEDGER.md](PARITY-LEDGER.md). Exact current shelf identity stays in [STATUS.md](STATUS.md). Raw disposable-profile evidence is local and ignored by Git.

## Ordinary updater completion

The final ordinary ARM64 workflow passed: built-in Harper **2.10.0 → installed 2.13.0 → built-in 2.10.0**, followed by a normal relaunch retaining the bundled engine and automatic checking off. Both launches exited normally with status zero. The actual Help toggle disabled automatic checks 1,733 ms after launch, before the scheduled automatic check. Installation and rollback activation markers, required files/WASM and real rendered engine labels agreed.

Evidence: `dist/windows-testing/results/updater-6558d35-07800d8e16af4e65963d7c064c1eface/`; report SHA-256 `7d3d20dd9393b9480439f6aa2a8aea7068b58ddc009f4a8a45d7459d90da5c5c`. Root independently rehashed all **36 evidence files** and visually inspected all four engine captures. The temporary driver SHA-256 is `7920ec3985734da65b27aa4d5bcabcd8ab55d311ed4c45867f6890af347865d0`. It used ordinary argument-free startup and OS input, with isolated profiles; no Backend test methods or fabricated server responses.

A real divider drag widened Review when the origin label was elided; the actual Single command removed a restored publishing pane before the exclusive Review crop. These are recorded observation adaptations, not product fixes. Earlier failed reports remain retained. This workflow qualifies ordinary update/install/rollback/restart for this exact package; it does not supersede the failed full CI or claim network-failure desktop coverage. Native updater regressions cover failed download/extraction separately.
