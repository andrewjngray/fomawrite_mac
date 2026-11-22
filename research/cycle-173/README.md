# Cycle 173 — Proportional PDF Preview wheel scrolling

Build 173 (`0.3.0-dev68`).

Windows devices and virtualized mouse wheels can emit several small events per physical wheel step. Qt Quick's default flick distance is short and can vary with input timing. PDF publishing Preview now applies a larger, proportional distance for angle-only vertical wheel events using the system's scroll-lines preference. High-resolution pixel input, horizontal scrolling and modified gestures retain Qt's standard handling. The PDF Reader and Web Preview keep their existing input paths.

## Verification

- Build 172 in Parallels moved 36 screen pixels for a quarter detent and 144 screen pixels for a full detent. The regression failed on 172 and passed with the new proportional distance. The existing PDF and Web right-click gesture tests passed.
- [Hands-on checklist](../usability/cycle-173.md).
