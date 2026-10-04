# Fomawrite Help

Fomawrite is a calm, local Markdown editor. Your documents remain ordinary UTF-8 Markdown files.

## Start writing

- Use **File → New** or **Open…** to begin.
- Save normally with **Command-S**. Unsaved-change prompts protect drafts when you close or replace a document.
- Use the Library for local folders and files. Tree and List navigation, filtering, sorting, saved locations, favorites, recent files and bounded quick search are available.
- Choose **Source**, **Split** or **Full** from the persistent lower-right controls. **Visual Edit** turns supported rendered editing on or off. Writing appearance and output template are separate choices.

## Editing

Find and Replace, Undo and Redo, headings, lists, tasks, emphasis, links, code, tables and other common Markdown commands operate on the source. Clear Styles deliberately refuses complex or ambiguous selections rather than risking damage.

Manual word completions use words from the current document. Writing Review and the opt-in Custom and Fillers style checks ignore code and URL syntax. Fillers currently checks the whole words “very”, “really”, “quite” and “just”. These checks never rewrite your text.

Authorship annotations are manual assertions stored beside saved Markdown in a hidden sidecar. The local Authors profile does not label text automatically or prove provenance.

## Find and navigate

**Command-F** opens Find in the document’s Markdown source. Return goes forward, Shift-Return goes back, and Escape returns to writing. Search starts at or after the current selection and wraps. Choose **Replace** to reveal the replacement field; replacing the current match advances to the next match, and **Replace all** is one Undo operation. Search is literal and case-insensitive.

Open **View → Document Outline** or choose Document outline from the workspace menu. Type words from a heading to filter the outline, use Up/Down to select and Return to jump. Escape dismisses it without changing your source selection. The current source section has a small marker; heading levels show the hierarchy.

## Visual list editing

In Visual Edit, Return can split a simple bullet, numbered or task item at a safe caret position. The new item keeps its marker style; a new task starts unchecked. Return on an empty item leaves the list. Splits inside emphasis, links or code and ambiguous nested/continued items stay unchanged; use Source for those structures. Undo and Redo preserve the original Markdown.

## Safety and privacy

- Editing, search, review and library work stay local.
- Recovery snapshots protect unsaved work. Previous-save versions are optional.
- External file changes are detected before reload or overwrite.
- Sharing, export and printing happen only when you choose their commands.
- Unsupported or ambiguous operations are refused without changing the document.

Open **Help → Keyboard Shortcuts** for the compact shortcut list.
