# Cycle 100 — Heading links review

Use disposable Markdown samples; save personal writing first.

1. Open `Source.md` and `Target.md` in separate Fomawrite windows. Give Target two `# Same` headings. From Source, follow `[second heading](Target.md#same-1)`. The existing Target window should come forward with its caret at the second heading.
2. Add `# Live heading` in Target without saving. Follow `[live](Target.md#live-heading)` from Source. Confirm Target retains its unsaved content and Undo still removes your last edit.
3. Follow a missing heading such as `Target.md#not-here`. Confirm a readable message appears in Target and the caret stays where it was.
4. Make Source dirty, follow a Target link, and choose Cancel. Confirm Source stays open with its unsaved text and Target does not navigate.
5. Try the same navigation when Target is in a native tab. Confirm it selects the right view without creating a duplicate.

Record any mismatch in focus, scroll position or feedback. The automated checks use the same production routing but do not establish physical multi-display behavior or complete VoiceOver acceptance.
