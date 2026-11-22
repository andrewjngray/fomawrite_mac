# Cycle 173 — Proportional PDF Preview wheel scrolling

Build 173 (`0.3.0-dev68`).

Windows devices and virtualized mouse wheels can emit several small events per physical wheel step. Qt Quick's default flick distance is short and can vary with input timing. PDF publishing Preview now applies a larger, proportional distance for angle-only vertical wheel events using the system's scroll-lines preference. High-resolution pixel input, horizontal scrolling and modified gestures retain Qt's standard handling. The PDF Reader and Web Preview keep their existing input paths.

## Verification

- Build 172 in Parallels moved 36 screen pixels for a quarter detent and 144 screen pixels for a full detent. The regression failed on 172 and passed with the new proportional distance. The existing PDF and Web right-click gesture tests passed.
- [Hands-on checklist](../usability/cycle-173.md).

The Mac suite passed 434 tests without failures or skips. Cocoa passed the new wheel and both link regressions. The signed Dev and installed bundles each passed four native Preview groups with eight settled pane captures. The installed executable matches the distribution package and the ARM64 archive checksums verified.

[Mac CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/44992210371) passed on retry with 422 Qt passes, zero failures and 12 grammar-service skips. The first attempt crashed during an existing Export Hub check, which passed in the local suite and on retry.

[Windows CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/44992240980) passed on ARM64 and x64 with 531 Qt checks, zero failures or skips, and both deployed desktop checks. Both manifests verified and both packages were staged in Parallels. The native ARM64 app moved 60 screen pixels for a quarter wheel detent and 240 for a full detent with the same Preview and display used for the build 172 baseline.
