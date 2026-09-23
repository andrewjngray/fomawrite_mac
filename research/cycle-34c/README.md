# Cycle 34c — automatic previous versions

Opt-in File → Keep Previous Version on Save preserves changed, existing disk bytes before replacing them. Failure to create a checkpoint blocks Save with an error. Unchanged saves do not create duplicate versions. Native versions still contain Markdown only; restoration uses the explicit-save/unlabelled policy from 34b.

Build and 62 tests pass, including real NSFileVersion capture of previous content and unchanged-save deduplication. Native checks are recorded with the combined five-cycle QA run. Hardware/display, OS-shutdown and disk-full/device-removal acceptance remain open; this implementation does not close those checks.
