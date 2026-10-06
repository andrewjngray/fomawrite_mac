import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, EditorSelection } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { LanguageDescription } from "@codemirror/language";
import {
  buildBlocks, buildBlockDecorations, fenceLanguages, ImageStore,
  ImageWidget, TaskWidget, HrWidget, FenceLabelWidget, FenceEndWidget, connectImageBridge,
} from "../src/blocks.ts";
import { buildLiveDecorations, isHiddenDecoration } from "../src/live.ts";

function stateAt(doc, anchor, head = anchor) {
  return EditorState.create({
    doc,
    selection: EditorSelection.single(anchor, head),
    extensions: [markdown({ base: markdownLanguage, codeLanguages: fenceLanguages })],
  });
}

/** Every decoration as {from, to, widget, cls}. */
function collect(state, ranges) {
  const out = [];
  buildBlockDecorations(state, ranges).between(0, state.doc.length, (from, to, d) => {
    out.push({ from, to, widget: d.spec.widget, cls: d.spec.class });
  });
  return out;
}
const widgetsOf = (items, Ctor) => items.filter((i) => i.widget instanceof Ctor);
const linesWith = (items, cls) => items.filter((i) => i.cls === cls && i.from === i.to).map((i) => i.from);

const IMG_DOC = 'before\n\n![alt text](pics/a.png "My title")\n\nafter ![inline](b.png) end\n';
const imgAt = IMG_DOC.indexOf("![alt");
const imgEnd = imgAt + '![alt text](pics/a.png "My title")'.length;

test("image outside the selection is replaced by a widget spanning the whole node", () => {
  const items = collect(stateAt(IMG_DOC, 0));
  const imgs = widgetsOf(items, ImageWidget);
  assert.equal(imgs.length, 2);
  const w = imgs.find((i) => i.from === imgAt);
  assert.ok(w, "block image widget present");
  assert.equal(w.to, imgEnd);
  assert.equal(w.widget.src, "pics/a.png");
  assert.equal(w.widget.alt, "alt text");
  assert.equal(w.widget.title, "My title");
  assert.equal(w.widget.block, true, "alone on its line");
  const inline = imgs.find((i) => i.widget.src === "b.png");
  assert.equal(inline.widget.block, false, "mid-paragraph image is inline");
  assert.ok(inline.widget.estimatedHeight !== undefined);
  assert.ok(w.widget.estimatedHeight > 0);
});

test("image yields no widget when the selection touches it (inclusive)", () => {
  for (const pos of [imgAt, imgAt + 5, imgEnd]) {
    const imgs = widgetsOf(collect(stateAt(IMG_DOC, pos)), ImageWidget);
    assert.ok(!imgs.some((i) => i.from === imgAt), `pos ${pos}: no widget`);
    assert.equal(imgs.length, 1, "the other image is still rendered");
  }
});

test("reference/unclosed/multi-line images stay raw; angle-bracket src is unwrapped", () => {
  const d = "![ref][r]\n\n![a](nope\n\n![multi\nline](x.png)\n\n![c](<my pic.png>)\n";
  const imgs = widgetsOf(collect(stateAt(d, d.length)), ImageWidget);
  assert.deepEqual(imgs.map((i) => i.widget.src), ["my pic.png"]);
});

test("task marker yields a checkbox widget with the right state; cursor on the line reveals", () => {
  const d = "- [ ] open\n- [x] done\n- [X] DONE\n\nplain\n";
  const tasks = widgetsOf(collect(stateAt(d, d.length)), TaskWidget);
  assert.deepEqual(tasks.map((i) => [i.from, i.to, i.widget.checked]), [[2, 5, false], [13, 16, true], [24, 27, true]]);
  const onLine = widgetsOf(collect(stateAt(d, 8)), TaskWidget); // cursor in "open"
  assert.deepEqual(onLine.map((i) => i.from), [13, 24], "only the cursor's line shows raw text");
});

test("horizontal rules yield a widget unless touched; setext heading is not a rule", () => {
  const d = "para\n\n---\n\n***\n\n___\n\nTitle\n-----\n";
  const hrs = widgetsOf(collect(stateAt(d, d.length)), HrWidget);
  assert.deepEqual(hrs.map((i) => [i.from, i.to]), [[6, 9], [11, 14], [16, 19]]);
  assert.equal(widgetsOf(collect(stateAt(d, 7)), HrWidget).length, 2, "cursor on --- reveals it");
});

const FENCE = "intro\n\n```js title\nconst a = 1;\n```\n\noutro\n";
const openAt = FENCE.indexOf("```js");
const body = FENCE.indexOf("const");
const closeAt = FENCE.lastIndexOf("```");

test("fence lines get label replacements only when the selection is outside the block", () => {
  const out = collect(stateAt(FENCE, 0));
  const labels = widgetsOf(out, FenceLabelWidget);
  assert.equal(labels.length, 1);
  assert.deepEqual([labels[0].from, labels[0].to], [openAt, openAt + "```js title".length]);
  assert.equal(labels[0].widget.label, "js title");
  const ends = widgetsOf(out, FenceEndWidget);
  assert.deepEqual(ends.map((i) => [i.from, i.to]), [[closeAt, closeAt + 3]]);
  assert.deepEqual(linesWith(out, "fw-code-open"), [openAt]);
  assert.deepEqual(linesWith(out, "fw-code-close"), [closeAt]);

  for (const pos of [body + 3, openAt, closeAt + 3]) {
    const inside = collect(stateAt(FENCE, pos));
    assert.equal(widgetsOf(inside, FenceLabelWidget).length, 0, `pos ${pos}: label gone`);
    assert.equal(widgetsOf(inside, FenceEndWidget).length, 0, `pos ${pos}: close fence raw`);
  }
});

test("fence without info string gets an empty label; unclosed fence has no closing replacement", () => {
  const d = "```\ncode\n```\n\n```py\nunclosed";
  const out = collect(stateAt(d, 0, d.length));
  assert.equal(widgetsOf(out, FenceLabelWidget).length, 0, "selection spans everything: all raw");
  const out2 = collect(stateAt(d, d.indexOf("code"))); // cursor in the first block
  const labels = widgetsOf(out2, FenceLabelWidget);
  assert.deepEqual(labels.map((l) => l.widget.label), ["py"]);
  assert.equal(widgetsOf(out2, FenceEndWidget).length, 0);
  assert.deepEqual(widgetsOf(collect(stateAt(d, d.length)), FenceLabelWidget).map((l) => l.widget.label), [""], "cursor in unclosed block reveals it; the closed fence without info gets an empty label");
});

test("tables: .fw-table-line on every line, delimiter row dimmed", () => {
  const d = "| a | b |\n|---|:-:|\n| 1 | 2 |\n\nafter\n";
  const out = collect(stateAt(d, d.length));
  assert.deepEqual(linesWith(out, "fw-table-line"), [0, 10, 20]);
  assert.deepEqual(linesWith(out, "fw-table-delim-line"), [10]);
  const dim = out.filter((i) => i.cls === "fw-table-delim");
  assert.deepEqual(dim.map((i) => [i.from, i.to]), [[10, 19]]);
});

test("nested blockquote lines carry their depth", () => {
  const d = "> a\n> > b\n> > > c\n\nplain\n";
  const out = collect(stateAt(d, d.length));
  assert.deepEqual(linesWith(out, "fw-quote-d1"), [0, 4, 10]);
  assert.deepEqual(linesWith(out, "fw-quote-d2"), [4, 10]);
  assert.deepEqual(linesWith(out, "fw-quote-d3"), [10]);
});

test("only visible ranges are decorated", () => {
  const items = collect(stateAt(IMG_DOC, 0), [{ from: 0, to: 6 }]);
  assert.equal(items.length, 0);
});

test("partial markdown never throws", () => {
  const docs = [
    "![alt](", "![alt", "![](x)", "![a](x \"unclosed", "```", "```js", "~~~\n", "- [ ]", "- [x", "---", "|", "| a |\n|--",
    "> > > > > deep", "```\n```\n```", "![a](b)![c](d)", "- [ ] ![t](x.png)\n", "", "\n",
  ];
  for (const d of docs) for (const pos of [0, Math.floor(d.length / 2), d.length])
    assert.doesNotThrow(() => buildBlocks(stateAt(d, pos)), JSON.stringify(d));
});

test("fenced code body is not decorated as markdown by live.ts or blocks.ts (nested markdown fence)", async () => {
  const md = LanguageDescription.matchLanguageName(fenceLanguages, "markdown");
  assert.ok(md, "markdown language description present");
  await md.load();
  const d = "```markdown\n**not bold** ![x](y.png) - [ ] no\n```\n";
  const state = stateAt(d, d.length);
  let hidden = 0;
  buildLiveDecorations(state).between(0, d.length, (f, t, dec) => { if (isHiddenDecoration(dec)) hidden++; });
  assert.equal(hidden, 0, "no inline markers hidden inside a code fence");
  assert.equal(widgetsOf(collect(state), ImageWidget).length, 0);
  assert.equal(widgetsOf(collect(state), TaskWidget).length, 0);
});

test("language-data is wired for fences (js, python, css resolve lazily)", () => {
  for (const n of ["js", "python", "css", "rust"]) assert.ok(LanguageDescription.matchLanguageName(fenceLanguages, n), n);
});

// ---------------------------------------------------------------- image store
test("requestImage: one host request per src; replies cached and fan out", () => {
  const sent = [];
  const store = new ImageStore();
  store.sink = (token, src) => sent.push([token, src]);
  const a1 = store.ensure("a.png");
  const a2 = store.ensure("a.png");
  const b = store.ensure("b.png");
  assert.equal(a1, a2);
  assert.deepEqual(sent.map((s) => s[1]), ["a.png", "b.png"]);
  assert.equal(a1.status, "pending");

  let notified = 0;
  a1.listeners.add(() => notified++);
  store.reply(sent[0][0], "data:image/png;base64,AAAA", "");
  assert.equal(a1.status, "ready");
  assert.equal(a1.dataUrl, "data:image/png;base64,AAAA");
  assert.equal(notified, 1);
  store.ensure("a.png");
  assert.equal(sent.length, 2, "no re-request after a successful reply");

  store.reply(sent[1][0], "", "not found");
  assert.equal(b.status, "error");
  assert.equal(b.error, "not found");
  store.ensure("b.png");
  assert.equal(sent.length, 2, "errors are cached too (retried only after a delay)");
  store.reply(sent[1][0], "data:image/png;base64,AAAA", ""); // stale token: ignored
  assert.equal(b.status, "error");
});

test("image replies that are not data:image URLs count as errors; clear() forgets everything", () => {
  const sent = [];
  const store = new ImageStore();
  store.sink = (t, s) => sent.push([t, s]);
  const e = store.ensure("x.png");
  store.reply(sent[0][0], "https://evil.example/x.png", "");
  assert.equal(e.status, "error");
  store.clear();
  assert.equal(store.cache.size, 0);
  store.reply(sent[0][0], "data:image/png;base64,AAAA", ""); // reply after clear: ignored
  store.ensure("x.png");
  assert.equal(sent.length, 2);
});

test("connectImageBridge wires requestImage and imageReply", () => {
  const sent = [];
  let handler;
  const store = new ImageStore();
  connectImageBridge({ requestImage: (t, s) => sent.push([t, s]), imageReply: { connect: (fn) => (handler = fn) } }, store);
  const e = store.ensure("p.png");
  handler(sent[0][0], "data:image/png;base64,AAAA", "");
  assert.equal(e.status, "ready");
});

test("hosts without requestImage get an error placeholder instead of hanging", async () => {
  const store = new ImageStore();
  connectImageBridge({}, store);
  const e = store.ensure("p.png");
  await Promise.resolve();
  assert.equal(e.status, "error");
});
