# Roam concept — Fomawrite as the harness between writing and distilled thought

Status: concept only. Listed in [IDEAS.md](IDEAS.md) as Maybe. Nothing here is built.

## The problem, in Andrew's words (paraphrased)

In an AI-centric world there are too many artifacts. Markdown files, chats, agents and exports multiply. Roam Research is where the small, precious, settled thoughts go: daily notes, the things worth distilling and keeping. The editor is the human-to-machine interface; Roam is the place to stay. The question is how the two relate so that writing in one and keeping in the other is one habit rather than two.

## What Roam is good at, and what Fomawrite is good at

| | Roam | Fomawrite |
| --- | --- | --- |
| Unit | A block, nested in an outline, on a page or a daily note | A document, a plain `.md` file |
| Links | Bidirectional `[[page]]` and `((block))` references, backlinks, graph | Local file links, outline, library folders |
| Strength | Small units, dense cross-linking, daily capture, "where did I think about X" | Long-form writing, typography, publishing, byte-exact files you own |
| Weakness | Long documents are awkward; export is lossy; it is a service | No cross-document graph; no daily capture habit built in |

The split suggests itself: **write and publish in Fomawrite; distil and keep in Roam.** The harness is whatever makes the handoff between them a single gesture.

## How the graph is actually used (read 7 October 2026, graph `AJG_claude`)

- **Size and shape.** About 3,480 pages, of which about 1,920 are daily notes. The daily note is the working surface: Roam's recent-activity view shows the daily-notes view open for most of the session time, and the recently edited pages are almost all daily notes.
- **Daily notes are meeting-shaped.** A typical day (23 September 2026) is a list of meetings, each a block like `[[Soprano]] Pre Board Meeting Potentia Catch up` with the points under it, attendees as `[[Person]]` links, and sometimes an AI-assistant summary block. Many days are empty: capture happens when there is something worth keeping, not every day.
- **The tag vocabulary is small and stable.** The most referenced pages are task states and meeting scaffolding: `DONE` (354 references), `ProjectTodos` (336), `TODO` (329), `meeting` (328), `tags` (296), then `notes`, `Attendees`, `Subject`, `boardmeeting`, `Put todays date here`, `Who is attending here`. Those last two are template placeholders, so there is a meeting template in use (`roam/templates` and `Templates_page` are starred).
- **People and companies are pages.** `Renato`, `Tim Reed`, `Nitro`, `Linkly`, `SuperChoice`, `Soprano`, `Jinjer`, `EHG`, `Micromine`, `Potentia` and named people are the hubs; a page per person or deal, linked from daily notes, with backlinks doing the filing.
- **Tasks live in Roam, GTD-style.** The starred `Vault` page is a GTD index: `ProjectTodos`, `WorkTodos`, `PersonalTodos`, `scheduled`, `waitingfor`, `somedaymaybe`, `DONE`. `Live List` holds the very-real-time tasks with a weekly sprint tally. `GSD` holds a few open intentions, including "create a daily journalling templates page".
- **Quick Capture** is a starred catch-all page for fragments and quotations (an Adam Phillips line sits there now). `poem`, `quote`, `videos - inspiration` and `prayers` show the graph also holds the distilled, personal material the concept is about.
- **Framework.** The help-notes page records the intent: P.A.R.A. without Resources, tag pages used purely as storage with linked references, projects versus areas. The `Areas` page itself is empty, so the framework is partly aspirational.
- **Not used.** No long-form writing in Roam; nothing that looks like a document draft. That supports the split in the next section.

What this means for Fomawrite: the natural unit to send to Roam is a block, not a document; the natural target is today's daily note or a person/deal page; the vocabulary to respect is `[[Page]]` links, `#tags`, `{{[[TODO]]}}`, and the meeting template. A "Send to Roam" that lands a selected paragraph on today's note with an optional `[[page]]` and a back-link to the file would fit the existing habit exactly.

## Possible shapes of the harness, cheapest first

1. **Send to Roam.** Select a paragraph or a block in Fomawrite, choose *Send to Roam*, and it lands as a block on today's daily note (or a chosen page) with a link back to the file and position. Roam's API supports creating blocks and pages; this session already has a Roam connector that can append to the daily note and create blocks, so the mechanics exist. One-way, no sync, no conflict.
2. **Roam side pane.** A pane in Fomawrite showing today's daily note and search over the graph, read-only, so the distilled context is beside the writing. Still one-way: Roam is read, never rewritten from the file.
3. **Pull from Roam.** Insert a Roam block or page into a document as a quote with a `((uid))` link, so published writing can cite the distilled thought. Roam's Markdown-ish syntax needs translating to CommonMark on the way in.
4. **Two-way link, not sync.** A document carries a front-matter key naming its Roam page; Roam's page carries a link to the file. Each side knows the other exists; neither overwrites the other. Full sync is deliberately out: it would break the "file is the truth" rule and Roam's block model does not round-trip to Markdown.

## How it fits the mission

[MISSION.md](MISSION.md) says the file is the truth and the app must stay small enough to trust. Shapes 1 to 4 keep both: nothing in Roam ever rewrites a file, and each shape is a bounded feature with a test. A full synchroniser would violate both and is not proposed.

## What would need to be true before building any of it

- Andrew's actual Roam habits are known: which pages, how daily notes are used, what "precious" looks like in practice. A short look at the graph's conventions with Andrew present would settle this.
- The Roam API path from a desktop Qt app is chosen (Roam's own API with a token, or the existing MCP connector run as a local helper).
- The current measure (is Live the daily editor?) is answered, so that this does not start before the editor itself is settled.

## Open questions for Andrew

- Which of the four shapes would you actually use first?
- Is the daily note the main target, or named pages?
- Should sent blocks carry a back-link to the file, or stay clean?
