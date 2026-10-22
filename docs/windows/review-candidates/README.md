# Unmerged desktop test-driver proposals

These patches preserve local diagnostic work for independent review. They are **not applied to the maintained drivers**, do not change Fomawrite itself, and are not covered by the green `78b9d9c` CI run. Do not infer app acceptance from them.

- `native-filename-acknowledgment.patch` is relative to the maintained normal driver/helper at `78b9d9c`. It observes actual filename selection and acknowledges each physical Unicode scalar before sending the next. No clipboard, SetValue or input replay. Native PowerShell parsing and combined C# compilation passed in the Parallels ARM64 guest; latest execution outcome is in the session handover.
- `clipboard-search-observer.patch` is relative to the historical retained diagnostic copy `dist/windows-testing/results/diagnosis/final-2f0-r11-clipboard-r10c/clipboard/`, not the maintained source tree. It replaces stale cross-thread accessibility objects with fresh observations and guarded hit-tests. Nine metadata tests passed, but native parsing/compilation and execution remain pending. A new matching PublicControl receipt is required; original user clipboard restoration remains unknown.

Frozen local copies and invocation instructions:

- `dist/windows-testing/results/diagnosis/native-path-acknowledgment-r1/` (index SHA-256 `f9abe202af4c4511f0605908ab10b381cbacdec196830d714d417aa46c78c6e8`).
- `dist/windows-testing/results/diagnosis/clipboard-search-observer-r12/` (index SHA-256 `1802a71e4d17b0530531c8ee355a76ce37a522a1937cf1d53456a4d71dedfac9`).

The `dist/` copies are ignored local evidence and will not appear in a Git clone. The patches are committed for review, not promotion. Preserve prior failed reports and review the actual observer/input contract before applying either proposal.
