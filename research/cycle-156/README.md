# Cycle 156 — A new document takes the keyboard

Build 156 (`0.3.0-dev52`), 5 November 2026. Andrew's request, with screenshots of the explorer's "+" still focused after creating `Untitled4.md`:

> "Once the new document is created, the focus should move down into the first line of the actual document itself so they can immediately start typing."

## What was wrong
"+" opens the New document dialog. When the dialog closed, Qt Quick handed keyboard focus back to the item that had it before, the "+" button, so the new document was open but typing went nowhere.

## What changed
- After "+" (or New File…, File → New in Library…) creates and opens a document, the caret moves to the start of its first line, in Source and in Live. Focus is moved once the New document dialog, and the unsaved-changes prompt if one appeared, has finished closing, so the dialog cannot take it back.
- Cancel still leaves focus on "+". "New in Library in Window…" is unchanged: the new window opens with its own focus.
- The app's built-in self-check (`--check-document-views`, "formatting-follows-writing-focus") clicked the header's small Bold button, which build 155 hides while the Format Bar is shown. It now clicks whichever Bold is on screen. The same fix went to Windows as `20e9c5f`.

## Tests
`tests/cycle156-new-document-focus.inc`: creates a document with "+", types the name and presses Return; checks Source has the keyboard at position 0 and that typed text lands in the document; then repeats in Live and checks the page has the keyboard with the caret at 0. It fails on build 155 (`f.editor->hasActiveFocus()`) and passes now.
