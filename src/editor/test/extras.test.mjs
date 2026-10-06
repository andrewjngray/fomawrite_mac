import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, EditorSelection } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { syntaxTree, ensureSyntaxTree } from "@codemirror/language";
import {
  extrasMarkdown, buildExtras, collectHeadings, buildTocTree, footnoteInfo, countFrontMatterFields,
  FootnoteRefWidget, FootnoteLabelWidget, TocWidget, FrontMatterWidget,
} from "../src/extras.ts";

const extensions = () => [markdown({ base: markdownLanguage, extensions: [extrasMarkdown] })];
const mdState = (doc, anchor = 0, head = anchor) =>
  EditorState.create({ doc, selection: EditorSelection.single(anchor, head), extensions: extensions() });

/** Nodes of the given names as {name, from, to, text}, in document order. */
function nodes(doc, ...names) {
  const state = mdState(doc);
  ensureSyntaxTree(state, doc.length, 1000);
  const out = [];
  syntaxTree(state).iterate({
    enter: (n) => {
      if (names.includes(n.name)) out.push({ name: n.name, from: n.from, to: n.to, text: doc.slice(n.from, n.to) });
    },
  });
  return out;
}
const texts = (doc, name) => nodes(doc, name).map((n) => n.text);

// ---------------------------------------------------------------- parsing: footnotes
test("[^id] parses to FootnoteRef; ids with letters, digits and dashes", () => {
  assert.deepEqual(texts("See[^1] and [^note-2] here.", "FootnoteRef"), ["[^1]", "[^note-2]"]);
});

test("[^] (empty id), whitespace in the id and unclosed refs are not footnotes", () => {
  assert.deepEqual(texts("a [^] b [^a b] c [^open", "FootnoteRef"), []);
});

test("a footnote ref is not parsed as a link", () => {
  assert.equal(nodes("x[^1]", "Link").length, 0);
});

test("[^id]: text at line start parses to FootnoteDef with a FootnoteLabel", () => {
  const doc = "Text[^1].\n\n[^1]: The note.";
  const all = nodes(doc, "FootnoteDef", "FootnoteLabel", "FootnoteRef");
  assert.deepEqual(all.map((n) => [n.name, n.text]), [
    ["FootnoteRef", "[^1]"],
    ["FootnoteDef", "[^1]: The note."],
    ["FootnoteLabel", "[^1]:"],
  ]);
});

test("definition text is parsed as inline markdown", () => {
  assert.deepEqual(texts("[^1]: a **bold** note", "StrongEmphasis"), ["**bold**"]);
});

test("multi-line definition: indented continuation lines belong to it, an unindented line does not", () => {
  const doc = "[^a]: first line\n  second line\n    third\nnot part\n";
  assert.deepEqual(texts(doc, "FootnoteDef"), ["[^a]: first line\n  second line\n    third"]);
});

test("a blank line followed by a 4-space indented paragraph continues the definition", () => {
  const doc = "[^a]: first\n\n    para two\n\nafter";
  assert.deepEqual(texts(doc, "FootnoteDef"), ["[^a]: first\n\n    para two"]);
  assert.deepEqual(texts("[^a]: first\n\n  not enough\n", "FootnoteDef"), ["[^a]: first"]);
});

test("consecutive definitions are separate; [^id]: mid-line or indented 4+ is not a definition", () => {
  assert.deepEqual(texts("[^1]: one\n[^2]: two", "FootnoteDef"), ["[^1]: one", "[^2]: two"]);
  assert.equal(nodes("text [^1]: not a def", "FootnoteDef").length, 0);
  assert.equal(nodes("    [^1]: code", "FootnoteDef").length, 0);
});

test("definitions inside blockquotes / lists are left alone", () => {
  assert.equal(nodes("> [^1]: quoted", "FootnoteDef").length, 0);
});

// ---------------------------------------------------------------- parsing: [toc]
test("[toc] alone on a line (any case) parses to TocBlock", () => {
  assert.deepEqual(texts("[toc]", "TocBlock"), ["[toc]"]);
  assert.deepEqual(texts("a\n\n[TOC]\n\nb", "TocBlock"), ["[TOC]"]);
  assert.deepEqual(texts("[Toc]  \n", "TocBlock"), ["[Toc]"]);
});

test("[toc] with other text on the line, or directly under a paragraph line, is not a TocBlock", () => {
  assert.equal(nodes("see [toc] here", "TocBlock").length, 0);
  assert.equal(nodes("[toc] more", "TocBlock").length, 0);
  assert.equal(nodes("[toc]x", "TocBlock").length, 0);
  assert.equal(nodes("    [toc]", "TocBlock").length, 0);
});

// ---------------------------------------------------------------- parsing: front matter
test("a leading --- block closed by --- parses to FrontMatter with two marks", () => {
  const doc = "---\ntitle: Hi\ntags: [a, b]\n---\n\n# Heading\n";
  const all = nodes(doc, "FrontMatter", "FrontMatterMark");
  assert.deepEqual(all.map((n) => [n.name, n.from, n.to]), [["FrontMatter", 0, 30], ["FrontMatterMark", 0, 3], ["FrontMatterMark", 27, 30]]);
  assert.equal(nodes(doc, "ATXHeading1").length, 1);
  assert.equal(nodes(doc, "HorizontalRule").length, 0);
});

test("`...` closes front matter too; blank lines inside are fine", () => {
  assert.deepEqual(texts("---\na: 1\n\nb: 2\n...\nbody", "FrontMatter"), ["---\na: 1\n\nb: 2\n..."]);
});

test("a document that does not start with --- has no front matter", () => {
  assert.equal(nodes("# Title\n\n---\nkey: v\n---\n", "FrontMatter").length, 0);
  assert.equal(nodes("\n---\nkey: v\n---\n", "FrontMatter").length, 0);
  assert.equal(nodes(" ---\nkey: v\n---\n", "FrontMatter").length, 0);
});

test("a leading --- without a closing fence is a horizontal rule, not front matter", () => {
  const doc = "---\nsome text\n";
  assert.equal(nodes(doc, "FrontMatter").length, 0);
  assert.equal(nodes(doc, "HorizontalRule").length, 1);
});

test("the closing fence does not turn the next paragraph into a setext heading, and later --- are rules", () => {
  const doc = "---\na: 1\n---\ntext\n\n---\n";
  assert.equal(nodes(doc, "FrontMatter").length, 1);
  assert.equal(nodes(doc, "SetextHeading1").length + nodes(doc, "SetextHeading2").length, 0);
  assert.equal(nodes(doc, "HorizontalRule").length, 1);
});

// ---------------------------------------------------------------- helpers
test("countFrontMatterFields counts top-level key: lines only", () => {
  const yaml = "title: Hello\n# a comment: not a key\ntags:\n  - a\n  - b: c\nauthor: Me\n\"quoted key\": 1\n- item: x\nurl: http://x.y";
  assert.equal(countFrontMatterFields(yaml), 5);
  assert.equal(countFrontMatterFields(""), 0);
});

const SAMPLE = [
  "# Title", "", "text", "", "## Section A", "", "### Detail **one**", "", "#### Too deep", "",
  "## Section B ##", "", "```", "# not a heading", "```", "", "> ## Quoted heading", "", "# Second [link](u)", "",
].join("\n");

test("collectHeadings: ATX levels 1-3 in order, marks stripped, code ignored", () => {
  const state = mdState(SAMPLE);
  const hs = collectHeadings(state);
  assert.deepEqual(hs.map((h) => [h.level, h.text]), [
    [1, "Title"], [2, "Section A"], [3, "Detail one"], [2, "Section B"], [2, "Quoted heading"], [1, "Second link"],
  ]);
  assert.equal(SAMPLE.slice(hs[0].from, hs[0].textFrom + 5), "# Title");
  assert.equal(SAMPLE.slice(hs[1].textFrom, hs[1].textFrom + 9), "Section A");
  assert.equal(collectHeadings(state), hs, "cached per syntax tree");
});

test("buildTocTree nests by level, including skipped levels", () => {
  const hs = [1, 2, 3, 2, 1, 3].map((level, i) => ({ level, text: "h" + i, from: i, textFrom: i }));
  const t = buildTocTree(hs);
  const shape = (ns) => ns.map((n) => [n.heading.text, shape(n.children)]);
  assert.deepEqual(shape(t), [["h0", [["h1", [["h2", []]]], ["h3", []]]], ["h4", [["h5", []]]]]);
  assert.deepEqual(t[0].children[0].children[0].index, 2);
  assert.deepEqual(buildTocTree([]), []);
});

test("footnoteInfo numbers refs by first reference, then unreferenced definitions", () => {
  const doc = "a[^b] c[^a] d[^b]\n\n[^a]: A\n[^z]: unused\n";
  const info = footnoteInfo(mdState(doc));
  assert.deepEqual([...info.numbers], [["b", 1], ["a", 2], ["z", 3]]);
  assert.deepEqual([...info.defined].sort(), ["a", "z"]);
});

// ---------------------------------------------------------------- decorations
function decos(doc, anchor, head = anchor) {
  const state = mdState(doc, anchor, head);
  const out = [];
  buildExtras(state).decorations.between(0, doc.length, (from, to, d) =>
    out.push({ from, to, widget: d.spec.widget, cls: d.spec.class, line: d.spec.class && from === to }));
  return out;
}
const widgets = (items, Ctor) => items.filter((i) => i.widget instanceof Ctor);

test("footnote refs: widgets only while the selection is outside, numbered by first reference", () => {
  const doc = "x[^b] y[^a] z[^b]\n\n[^a]: A\n[^b]: B";
  const items = decos(doc, 0);
  const refs = widgets(items, FootnoteRefWidget);
  assert.deepEqual(refs.map((r) => [r.from, r.to, r.widget.n, r.widget.id, r.widget.missing]), [
    [1, 5, 1, "b", false], [7, 11, 2, "a", false], [13, 17, 1, "b", false],
  ]);
  // touching (inclusive) a ref reveals just that one
  for (const pos of [1, 3, 5]) {
    const at = decos(doc, pos);
    assert.equal(widgets(at, FootnoteRefWidget).length, 2, "pos " + pos);
    assert.ok(at.some((i) => i.cls === "fw-footnote-src" && i.from === 1), "raw ref marked at " + pos);
  }
  assert.equal(widgets(decos(doc, 6), FootnoteRefWidget).length, 3); // between refs: not touching
});

test("a reference without a definition is flagged missing", () => {
  const refs = widgets(decos("a[^q]", 5), FootnoteRefWidget);
  assert.equal(refs.length, 0); // caret touches the ref at its end
  const w = widgets(decos("a[^q] b", 7), FootnoteRefWidget);
  assert.equal(w.length, 1);
  assert.equal(w[0].widget.missing, true);
});

test("footnote definitions: line class on every line, label widget only while the first line is untouched", () => {
  const doc = "t[^1]\n\n[^1]: first\n  second\n\nafter";
  const items = decos(doc, 0);
  const lineStarts = items.filter((i) => i.cls === "fw-footnote-def").map((i) => i.from);
  assert.deepEqual(lineStarts, [7, 19]);
  const labels = widgets(items, FootnoteLabelWidget);
  assert.equal(labels.length, 1);
  assert.deepEqual([labels[0].from, labels[0].to, labels[0].widget.n], [7, 13, 1]); // "[^1]: " incl. the space
  // caret anywhere on the first line: raw label, line classes stay
  for (const pos of [7, 10, 18 - 1]) {
    const at = decos(doc, pos);
    assert.equal(widgets(at, FootnoteLabelWidget).length, 0, "pos " + pos);
    assert.equal(at.filter((i) => i.cls === "fw-footnote-def").length, 2);
  }
  // caret on a continuation line keeps the label rendered
  assert.equal(widgets(decos(doc, 22), FootnoteLabelWidget).length, 1);
});

test("[toc]: TocWidget with the heading list while outside; raw when touched", () => {
  const doc = "# A\n\n[toc]\n\n## B\n\n### C\n\n#### D";
  const items = decos(doc, 0);
  const w = widgets(items, TocWidget);
  assert.equal(w.length, 1);
  assert.deepEqual([w[0].from, w[0].to], [5, 10]);
  assert.deepEqual(w[0].widget.headings.map((h) => [h.level, h.text]), [[1, "A"], [2, "B"], [3, "C"]]);
  for (const pos of [5, 7, 10]) assert.equal(widgets(decos(doc, pos), TocWidget).length, 0, "pos " + pos);
  assert.ok(decos(doc, 7).some((i) => i.cls === "fw-toc-src"));
  assert.equal(widgets(decos(doc, 11), TocWidget).length, 1);
});

test("TocWidget identity ignores positions (edits above do not rebuild it) but not text", () => {
  const h = (level, text, from) => ({ level, text, from, textFrom: from });
  assert.ok(new TocWidget([h(1, "A", 0)]).eq(new TocWidget([h(1, "A", 40)])));
  assert.ok(!new TocWidget([h(1, "A", 0)]).eq(new TocWidget([h(1, "B", 0)])));
  assert.ok(!new TocWidget([h(1, "A", 0)]).eq(new TocWidget([h(2, "A", 0)])));
});

test("front matter outside the selection: one-line widget with the field count, other lines hidden and collapsed", () => {
  const doc = "---\ntitle: T\ntags: [a]\n  nested: x\n---\n\n# Body";
  const items = decos(doc, doc.length);
  const w = widgets(items, FrontMatterWidget);
  assert.equal(w.length, 1);
  assert.deepEqual([w[0].from, w[0].to, w[0].widget.fields], [0, 3, 2]);
  assert.equal(items.filter((i) => i.cls === "fw-extras-collapsed").length, 4);
  assert.deepEqual(items.filter((i) => !i.widget && i.to > i.from).map((i) => [i.from, i.to]), [[4, 12], [13, 22], [23, 34], [35, 38]]);
  assert.equal(items.filter((i) => i.cls === "fw-front-matter-line").length, 0);
});

test("front matter with the caret inside or on a fence: raw lines with fw-front-matter-line, no widget", () => {
  const doc = "---\ntitle: T\n---\n\nbody";
  for (const pos of [0, 3, 6, 16]) {
    const items = decos(doc, pos);
    assert.equal(items.filter((i) => i.widget).length, 0, "pos " + pos);
    assert.deepEqual(items.filter((i) => i.cls === "fw-front-matter-line").map((i) => i.from), [0, 4, 13], "pos " + pos);
  }
  assert.equal(widgets(decos(doc, 18), FrontMatterWidget).length, 1);
});

test("front matter widget text: singular/plural come from the field count", () => {
  assert.equal(new FrontMatterWidget(1).fields, 1);
  assert.ok(new FrontMatterWidget(3).eq(new FrontMatterWidget(3)));
  assert.ok(!new FrontMatterWidget(3).eq(new FrontMatterWidget(2)));
});

test("a document that is not front matter produces no front matter decorations", () => {
  const items = decos("intro\n\n---\ntitle: T\n---\n", 0);
  assert.equal(widgets(items, FrontMatterWidget).length, 0);
});

test("the builder honours the ranges argument (only visible ranges are decorated)", () => {
  const doc = "a[^1] b\n\n\n\nc[^1]\n\n[^1]: n";
  const state = mdState(doc, doc.length);
  const out = [];
  buildExtras(state, [{ from: 0, to: 8 }]).decorations.between(0, doc.length, (f, t, d) => d.spec.widget && out.push(f));
  assert.deepEqual(out, [1]);
});
