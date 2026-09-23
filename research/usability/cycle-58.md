# Cycle 58 usability

- [ ] Open `research/cycle-58/sample/Source.md` and place the caret inside the inline link.
- [ ] Choose Go → Open Link. Confirm Target.md opens at the second `Same` heading and its title has no `#same-1` suffix.
- [ ] Return to Source.md and follow `[[Target#same-1]]`; confirm it reaches the same heading.
- [ ] In Target.md, follow the same-file `#same-1` link and confirm the document stays clean with no new Undo step.
- [ ] Repeat from rendered preview and confirm the target heading is visible after rendering.
- [ ] Make an unsaved synthetic edit in Source.md, request a fragment link to another file and choose Cancel. Confirm source, caret and scroll stay put.
- [ ] Try a missing heading and report whether opening the valid document without a dedicated message is acceptable.
- [ ] Report whether fragment navigation feels predictable and whether cross-window transfer should be the next refinement.

Recorded native checkpoint: Source.md opened by path and Go → Open Link opened Target.md with clean status and no fragment in the title. Exact caret and preview scrolling are automated only; visual scroll and screenshot capture were unavailable.
