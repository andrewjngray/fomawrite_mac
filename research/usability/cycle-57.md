# Cycle 57 usability

- [ ] Open `research/cycle-57/sample/Help.md`, then choose Help → Omawrite Help. Confirm the page is readable, scrollable and selectable without becoming editable.
- [ ] Close it with Escape, reopen it and use Close.
- [ ] Open What’s New in Omawrite and check that its feature descriptions match the current app without implying full iA Writer or Typora parity.
- [ ] Confirm Keyboard Shortcuts remains reachable and the sample source, caret, Undo history and saved status do not change.
- [ ] Repeat in dark mode and a narrow window; check keyboard traversal and VoiceOver.
- [ ] Report whether the local Help structure is useful and which offline topic should be added next.
- [ ] With a normal synthetic document window away from the screen center, choose Window → Center. Confirm only that window moves, retains its size and leaves document status unchanged.
- [ ] Repeat on a second display if available, then report whether AppKit centers within that display’s available area.

Recorded native checkpoint: macOS accessibility verified the Help menu, both bundled pages, Close and Escape. Synthetic Templates.md source and status remained unchanged. Later 57b QA showed Window → Center visible/enabled and invoked it on clean Target.md without changing status. A live app-only crop showed source and preview but could not establish on-screen centering; no screenshot was saved. Help visuals, Center OS/multi-display geometry, dark/narrow, VoiceOver and OS Help search checks remain open.
