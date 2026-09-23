# Cycle 52: statistics and View chrome (native QA pending)

Synthetic fixture: `sample/Statistics.md`. [Counting and chrome decisions](counting-rules.md) distinguish deterministic Omawrite behavior from unverified iA semantics.

Source work adds sentence, speaking, task and manual Human/AI/Reference counts, live statistics refresh, Default/Stats Only with ten independent metric toggles, and persisted Title Bar/Toolbar display modes. The unified top strip stays 44 px to protect macOS traffic lights and window dragging. The compact stats label has a full accessible/hover value when elided.

`./bin/build` and `./bin/test` passed after the tooltip adjustment with 84 tests; the combined later source passes 89. The stable Dev app was normally closed and refreshed on 24 September. [Native menu evidence](native-menu.txt) covers the View structure, fixture counts and Stats Only/Default switch. Visual screenshots and the remaining acceptance matrix are pending because screenshot capture was unavailable. No private writing was edited.
