# Cycle 41 — integrated acceptance pass

Synthetic Closeout.md and Long-notes.md were used. Native checks: spelling Replace/Undo and code exclusion; grammar check; palette navigation; two-page paginated preview; speech Start/Stop invocation; fullscreen Quit/relaunch with two tabs; long-note forward jump after fixing Qt's dropped empty anchor behavior. Return positions covered automatically; native return-click result was inconclusive.

Dark-theme screenshots reviewed and sample-only. `narrow.png` records an unsuccessful resize attempt and is excluded from evidence; width behavior is covered by existing automated layout tests, not claimed as native acceptance. Recents remained collapsed. Custom output PDF: all three rendered pages visually reviewed; title, body, image, header/footer and counters correct.

70 tests pass. Bounds test reports 2,000 tags and a scan limit for a 2,001-tag fixture, approximately 50 ms locally. Oversized file skipped. Screen geometry clamp tested with missing/off-screen coordinates. Hardware, cross-app, VoiceOver and language scope remain open in docs/release-acceptance.md.
