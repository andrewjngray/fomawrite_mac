# Cycle 52: statistics and View chrome (native QA pending)

Synthetic fixture: `sample/Statistics.md`. [Counting and chrome decisions](counting-rules.md) distinguish deterministic Omawrite behavior from unverified iA semantics.

Source work adds sentence, speaking, task and manual Human/AI/Reference counts, live statistics refresh, Default/Stats Only with ten independent metric toggles, and persisted Title Bar/Toolbar display modes. The unified top strip stays 44 px to protect macOS traffic lights and window dragging. The compact stats label has a full accessible/hover value when elided.

`./bin/build` and `./bin/test` passed again after the tooltip adjustment with 84 tests. Dev native QA, bundle refresh, visual screenshots and sign-off are pending because the Mac locked before the QA app could be closed normally. The running Dev app has not been replaced or force-closed. No private writing has been edited for this cycle.
