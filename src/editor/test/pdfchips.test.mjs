import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, EditorSelection } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { buildPdfChips, chipsForView, parseChip, PdfChipWidget } from "../src/pdfchips.ts";
import { liveExtension } from "../src/live.ts";
import { Session } from "../src/modes.ts";

const DOC = [
  "<!-- fomawrite:pdf-notes 1 -->",
  "# Notes on Report.pdf",
  "",
  "## [Page 3](Report.pdf#page=3)",
  "",
  "<!-- fomawrite:pdf highlight page=3 colour=green text=12+5 -->",
  "> ALPHA",
  "",
  "<!-- fomawrite:pdf underline page=3 colour=red text=40+5 -->",
  "> OMEGA",
  "",
  "<!-- fomawrite:pdf strike page=3 text=50+5 -->",
  "> GAMMA",
  "",
  "<!-- fomawrite:pdf note page=3 at=100,150 -->",
  "My note.",
  "",
  "<!-- fomawrite:pdf ink page=4 -->",
  "",
  "<!-- some other comment -->",
  "",
].join("\n");

const stateAt = (doc, anchor = doc.length) =>
  EditorState.create({ doc, selection: EditorSelection.single(anchor), extensions: [markdown({ base: markdownLanguage })] });

function chips(state, selection) {
  const out = [];
  buildPdfChips(state, undefined, selection).widgets.between(0, state.doc.length, (from, to, d) => {
    out.push({ from, to, text: state.doc.sliceString(from, to), info: d.spec.widget.info, widget: d.spec.widget });
  });
  return out;
}

test("parseChip: kind and page, colour for text marks, header, unknown kinds, other comments", () => {
  assert.deepEqual(parseChip("<!-- fomawrite:pdf highlight page=3 colour=green text=1+2 -->"), { kind: "highlight", label: "Highlight · p. 3", colour: "#34c759" });
  assert.equal(parseChip("<!-- fomawrite:pdf underline page=2 -->").colour, "#ff3b30");
  assert.equal(parseChip("<!-- fomawrite:pdf strike page=2 -->").label, "Strikethrough · p. 2");
  assert.equal(parseChip("<!-- fomawrite:pdf highlight page=2 colour=mauve -->").colour, "#ffd60a");
  assert.deepEqual(parseChip("<!-- fomawrite:pdf note page=3 at=1,2 -->"), { kind: "note", label: "Note · p. 3", colour: "" });
  assert.deepEqual(parseChip("<!-- fomawrite:pdf-notes 1 -->"), { kind: "header", label: "PDF notes", colour: "" });
  assert.equal(parseChip("<!-- fomawrite:pdf ink page=4 -->").label, "Ink · p. 4");
  assert.equal(parseChip("<!-- fomawrite:pdf ink -->").label, "Ink");
  assert.equal(parseChip("<!-- some other comment -->"), null);
  assert.equal(parseChip("<!-- fomawrite:pdf -->"), null);
  assert.equal(parseChip("<!-- fomawrite:pdf note page=3 --> trailing"), null);
});

test("Live: a chip for the header and for each kind, unknown kinds generic, other comments untouched", () => {
  const found = chips(stateAt(DOC));
  assert.deepEqual(found.map((c) => c.info.label), [
    "PDF notes", "Highlight · p. 3", "Underline · p. 3", "Strikethrough · p. 3", "Note · p. 3", "Ink · p. 4",
  ]);
  assert.equal(found[0].text, "<!-- fomawrite:pdf-notes 1 -->");
  assert.ok(found.every((c) => c.widget instanceof PdfChipWidget));
  assert.ok(!DOC.slice(found.at(-1).to).includes("fomawrite"), "the other comment is not a chip");
});

test("rendering never changes the document", () => {
  const state = stateAt(DOC);
  const before = state.doc.toString();
  buildPdfChips(state);
  buildPdfChips(state, undefined, EditorSelection.single(DOC.indexOf("highlight")));
  assert.equal(state.doc.toString(), before);
  assert.equal(before, DOC);
});

test("the chip gives way to the raw marker while the caret is on its line", () => {
  const at = DOC.indexOf("<!-- fomawrite:pdf note");
  const state = stateAt(DOC, at + 5);
  const found = chips(state);
  assert.ok(!found.some((c) => c.info.kind === "note"), "the note's line is raw");
  assert.equal(found.length, 5);
  // A selection across two markers reveals both.
  const sel = EditorSelection.single(DOC.indexOf("<!-- fomawrite:pdf highlight") + 3, DOC.indexOf("<!-- fomawrite:pdf underline") + 3);
  assert.deepEqual(chips(state, sel).map((c) => c.info.kind), ["header", "strike", "note", "ink"]);
});

test("markers inside a code fence or indented code are text, not chips", () => {
  const doc = "```\n<!-- fomawrite:pdf highlight page=1 -->\n```\n\n    <!-- fomawrite:pdf note page=1 -->\n\n<!-- fomawrite:pdf note page=2 -->\n";
  const found = chips(stateAt(doc, 0), EditorSelection.single(0));
  assert.deepEqual(found.map((c) => c.info.label), ["Note · p. 2"]);
});

test("Source mode draws no chips; Live does", () => {
  const session = new Session();
  const view = {
    state: session.createState(""),
    dispatch(...specs) { this.state = this.state.update(...specs).state; },
    setState(s) { this.state = s; },
    get visibleRanges() { return [{ from: 0, to: this.state.doc.length }]; },
    plugin(ext) { return ext === liveExtension && session.getMode() === "live" ? {} : null; },
  };
  session.attach(view);
  session.setDocument(DOC, 1);
  const count = () => { let n = 0; chipsForView(view).decorations.between(0, view.state.doc.length, () => { n++; }); return n; };
  assert.equal(session.getMode(), "source");
  assert.equal(count(), 0);
  assert.equal(session.getText(), DOC);
  session.setMode("live");
  assert.ok(count() >= 6);
  assert.equal(session.getText(), DOC);
  session.setMode("source");
  assert.equal(count(), 0);
  assert.equal(session.getText(), DOC);
});
