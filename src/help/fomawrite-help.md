# Fomawrite Help

Fomawrite is a calm, local Markdown editor. Your documents remain ordinary UTF-8 Markdown files.

## Start writing

- Use **File → New** or **Open…** to begin.
- Save normally with **Command-S**. Unsaved-change prompts protect drafts when you close or replace a document.
- Use the Library for local folders and files. Tree and List navigation, filtering, sorting, saved locations, favorites, recent files and bounded quick search are available.
- Choose **Source** or **Visual Edit** in the Editing group, then **Single** or **Split** in the separate Layout group. Single shows the chosen editor; Split adds read-only output Preview on the right. Changing one group preserves the other choice. **View → Preview Only** opens a reading view; choose Source or Visual Edit to resume editing.

## Text size and pane widths

Each visible writing pane has its own **− / percentage / +** control in its header. Source and Preview adjust independently from 75% to 200%. Click the percentage for presets or **Reset to 100%**. These are screen settings; Markdown and exported font sizes are unchanged.

**Link zoom** matches both sides to the pane whose menu you used and makes subsequent adjustments affect both. Turn it off to adjust them independently again. Source and Visual Edit share the left editor’s zoom preference. Read-only output Preview has its own zoom. **View → Text Size** changes the last focused writing pane, or the only visible pane.

Drag the divider between Source and Preview to resize the columns. Double-click it to balance their widths; narrow windows retain the minimum usable width for each side. Zoom preferences and column widths are remembered.

## Editing

Find and Replace, Undo and Redo, headings, lists, tasks, emphasis, links, code, tables and other common Markdown commands operate on the source. Clear Styles deliberately refuses complex or ambiguous selections rather than risking damage.

Click Source before using Markdown formatting. Its toolbar, keyboard shortcuts and Format menu act on that Source selection; they are unavailable while Find, filters, link fields, Preview or Visual Edit owns focus. Typewriter scrolling applies to Source only.

Manual word completions use words from the current document. Writing Review and the opt-in Custom and Fillers style checks ignore code and URL syntax. Fillers currently checks the whole words “very”, “really”, “quite” and “just”. These checks never rewrite your text.

Authorship annotations are manual assertions stored beside saved Markdown in a hidden sidecar. The local Authors profile does not label text automatically or prove provenance.

## Find and navigate

**Command-F** opens Find in the document’s Markdown source. Return goes forward, Shift-Return goes back, and Escape returns to writing. Search starts at or after the current selection and wraps. Choose **Replace** to reveal the replacement field; replacing the current match advances to the next match, and **Replace all** is one Undo operation. Search is literal and case-insensitive.

Open **View → Document Outline** or choose Document outline from the workspace menu. Type words from a heading to filter the outline, use Up/Down to select and Return to jump. Escape dismisses it without changing your source selection. The current source section has a small marker; heading levels show the hierarchy.

## Visual list editing

In Visual Edit, Return can split a simple bullet, numbered or task item at a safe caret position. The new item keeps its marker style; a new task starts unchecked. Return on an empty item leaves the list. Splits inside emphasis, links or code and ambiguous nested/continued items stay unchanged; use Source for those structures. Undo and Redo preserve the original Markdown.

## Links

Select ordinary text in Source, or place the caret inside a supported inline link, then choose the link toolbar button or **Format → Add Link**. Enter the text, destination and optional title. **Insert** creates a link; **Save** updates an existing one. A local destination may contain spaces. The change is one Undo operation.

**Cancel** or Escape returns to the original Source selection without changing it. If the document changes while the dialog is open, application is refused and your entries stay visible: cancel and reopen the link against the current text. Complex links, code, images and multiline selections remain Source-editing work.

## Visual table navigation

Within a supported table cell in Visual Edit, **Tab** moves to the next cell and **Shift-Tab** to the previous one, including empty cells. Navigation alone never changes the Markdown or creates an Undo step. At a table boundary or unsupported row the caret stays in place. Use **F6** to leave the writing surface and Source to add/remove rows or columns. Typing in a supported cell uses the existing safe mapping and normal Undo/Redo.

## Safety and privacy

- Editing, search, review and library work stay local.
- Recovery snapshots protect unsaved work. Previous-save versions are optional.
- External file changes are detected before reload or overwrite.
- Sharing, export and printing happen only when you choose their commands.
- Unsupported or ambiguous operations are refused without changing the document.

Open **Help → Keyboard Shortcuts** for the compact shortcut list.

## Pane controls and file lists

The lower appearance menu sits beneath the chosen editor. The Preview template menu follows the left edge of output Preview when you drag the divider. The two separate Editing and Layout groups keep their positions at the right across mode changes. Narrow style menus shorten in place and keep the full style name in their tooltip. Hidden-pane settings remain available from the View menu.

Preview headers show the current filename with a document icon. **— Edited** means the document has unsaved changes; saving clears it. Long names shorten in the middle, and their tooltip shows the full name.

With Show Text Excerpts enabled, the library shows the actual filename above up to two lines of muted excerpt text. Date Modified or Date Created appears beside the filename when enabled. Tree navigation indents real child folders and files; List navigation shows the current folder’s siblings.
