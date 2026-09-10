# Cycle 5 — UI alignment and Markdown preview

Reference: running iA Writer on this Mac, inspected with computer use using our `examples/Preview-check.md` fixture. The reference screenshot hides the library to exclude private filenames and writing excerpts. No source text was changed in iA Writer.

## Findings

- iA uses compact toolbar controls, thin pane separators, restrained sidebar headings and a bottom filter/formatting bar.
- `# title` is an H1. `#title` is not a heading: iA turns it into a hashtag link; Omawrite currently leaves it plain text. Hashtag navigation remains a separate feature.
- Fenced headings and emphasis stay literal. Omawrite's source highlighter previously disagreed with the preview here; this is fixed.
- The first compact-control screenshot exposed inherited Material padding clipping labels. The final build sets each padding explicitly.
- Preview heading sizing now uses the actual preview text size, rather than QTextDocument's separate default font.

## Evidence

- `screenshots/ia-preview-reference.png`: iA's rendered sample and heading behavior.
- `screenshots/omawrite-four-panes-initial.png`: intermediate review exposing clipped labels; not the final UI.
- Final screenshots are listed in the cycle log after visual verification.

Usability notes: [Andrew's checklist](../usability/cycle-05.md).

## Final verification

- `screenshots/omawrite-four-panes-final.png`: compact organizer/library/editor/preview, with paragraph focus off for normal reading.
- `screenshots/omawrite-split-final.png`: library hidden; writing and preview together.
- `screenshots/omawrite-preview-final.png`: full reading layout.
- `logs/tests.log`: 20 passed, 0 failed.
- `logs/package.log`: completed standalone Mac bundle. Deep/strict signature verification also passed.

The temporary QA copy used a distinct bundle identifier so existing unsaved Omawrite windows were not touched. Its executable and packaged resources came from `dist/Omawrite.app`.

Native live check: replaced the sample heading with “Live preview works”; the preview updated. Restored the original heading with two undo operations (selection replacement arrived as separate deletion/insertion through computer use), then saved the original fixture. Full Preview hid the source editor, and Split/library visibility were restored. Runtime log was empty.

Still unverified manually: narrow-screen usability, dark-mode contrast for every control, keyboard-only traversal and all image/link workflows. Pane proportions and font sizes remain adjustable and are deliberately part of Andrew's feedback exercise.
