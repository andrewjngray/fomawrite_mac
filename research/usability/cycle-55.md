# Cycle 55 usability

- [ ] On `research/cycle-55/sample/Focus.md`, use Focus → Enable Focus Mode → Sentence, then Paragraph. Confirm the desired dimming and cursor movement.
- [ ] Enable Typewriter as well and verify whether independent scrolling is useful.
- [ ] Repeat in narrow and dark windows, full screen and with keyboard menu navigation.
- [ ] Confirm writing, selection and Undo remain normal while Focus is on.
- [ ] Open `research/cycle-55/sample/Custom-review.md`, enter `really, paragraph` in Writing Review, then enable Focus → Enable Style Check → Custom. Confirm prose matches are visible while inline/fenced/indented code and the URL destination are not highlighted.
- [ ] Edit a Custom term and the sample text; confirm the overlay follows after a short pause, then disable Custom and confirm it clears immediately.
- [ ] Confirm Custom never changes the source, caret, saved status or Undo history, including in dark mode and with Find highlighting active.
- [ ] Open `research/cycle-55/sample/Fillers.md` and enable Fillers. Confirm only whole-word, case-insensitive `very`, `really`, `quite` and `just` prose matches appear; `veryish`, code, URL destinations and raw tags should remain untouched.
- [ ] Enable Custom with `really, paragraph`, then toggle Fillers independently. Confirm the shared `really` match is not doubled and remains while Custom is on.
- [ ] Report whether Writing Review is the right place to edit Custom terms and whether the review color is calm enough.

Recorded native checkpoints: the stable Dev app exposed both Fillers and Custom. Toggling Fillers on synthetic `research/cycle-47/sample/Templates.md` left source and saved status unchanged; earlier Custom setup accepted `paragraph` through Writing Review with the same nonmutation result. A later live view of `sample/Fillers.md` visibly showed yellow on prose `really`, `very`, `QUITE`, `just` and link-label `quite`, with no yellow on `veryish`, URL destination, inline/fenced/indented code or raw tag attributes. Status remained `Opened Fillers.md`. The live image was not persisted as a screenshot file; checked-state, Custom overlap appearance, dark/narrow/fullscreen and VoiceOver remain open.
