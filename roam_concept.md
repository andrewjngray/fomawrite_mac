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
