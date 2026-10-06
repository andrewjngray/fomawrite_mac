import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, EditorSelection } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { syntaxTree } from "@codemirror/language";
import { buildMath, mathMarkdown, mathCache, clearMathCache, renderMath, MathWidget } from "../src/math.ts";

const extensions = () => [markdown({ base: markdownLanguage, extensions: [mathMarkdown] })];
const mdState = (doc, anchor = 0, head = anchor) =>
  EditorState.create({ doc, selection: EditorSelection.single(anchor, head), extensions: extensions() });

/** Nodes of the given names as {name, from, to, text}, in document order. */
function nodes(doc, ...names) {
  const state = mdState(doc);
  const out = [];
  syntaxTree(state).iterate({
    enter: (n) => {
      if (names.includes(n.name)) out.push({ name: n.name, from: n.from, to: n.to, text: doc.slice(n.from, n.to) });
    },
  });
  return out;
}
const inline = (doc) => nodes(doc, "InlineMath").map((n) => n.text);

test("inline $...$ parses to InlineMath with two MathMark delimiters", () => {
  const doc = "Energy $E=mc^2$ here.";
  const all = nodes(doc, "InlineMath", "MathMark");
  assert.deepEqual(all.map((n) => [n.name, n.text]), [["InlineMath", "$E=mc^2$"], ["MathMark", "$"], ["MathMark", "$"]]);
  assert.equal(all[0].from, 7);
});

test("inline math contents are not parsed as emphasis; several spans per line", () => {
  assert.deepEqual(inline("$a_b * c_d$ and $x_1$"), ["$a_b * c_d$", "$x_1$"]);
  assert.equal(nodes("$a_b * c_d$", "Emphasis").length, 0);
});

test("no space right after the opening $ or before the closing $", () => {
  assert.deepEqual(inline("a $ x$ b"), []);
  assert.deepEqual(inline("a $x $ b"), []);
  assert.deepEqual(inline("a $ x $ b"), []);
});

test("escaped \\$ is text and never closes", () => {
  assert.deepEqual(inline("costs \\$5 and \\$6"), []);
  assert.deepEqual(inline("$a\\$b$"), ["$a\\$b$"]);
  // escaped opener: the `$` after `a` cannot open ("$ then" has a space), `$b$` is math
  assert.deepEqual(inline("\\$a$ then $b$"), ["$b$"]);
});

test("prices stay text (closing $ followed by a digit / preceded by a space)", () => {
  assert.deepEqual(inline("It costs $5 and $10."), []);
  assert.deepEqual(inline("$5 and $10, then $x$ ok"), ["$x$"]);
  assert.deepEqual(inline("between $20,000 and $30,000"), []);
  assert.deepEqual(inline("$a$5"), []);
});

test("unclosed $ and a $ spanning a line break are text", () => {
  assert.deepEqual(inline("just a $ sign"), []);
  assert.deepEqual(inline("open $x + y"), []);
  assert.deepEqual(inline("$x +\ny$"), []);
  assert.deepEqual(inline("$$"), []);
  assert.deepEqual(inline("$$x$$ inline"), []);
});

test("math inside inline code is not math", () => {
  assert.deepEqual(inline("`$x$` and $y$"), ["$y$"]);
});

test("block $$ ... $$ on their own lines parses to MathBlock with MathMark lines", () => {
  const doc = "before\n\n$$\n\\int_0^1 x^2 dx\n$$\n\nafter";
  const all = nodes(doc, "MathBlock", "MathMark");
  assert.deepEqual(all.map((n) => [n.name, n.text]), [["MathBlock", "$$\n\\int_0^1 x^2 dx\n$$"], ["MathMark", "$$"], ["MathMark", "$$"]]);
  assert.equal(nodes(doc, "Paragraph").length, 2);
});

test("single-line $$tex$$ is a block; multi-line body keeps every line", () => {
  assert.deepEqual(nodes("$$x^2$$", "MathBlock").map((n) => n.text), ["$$x^2$$"]);
  assert.deepEqual(nodes("$$\na &= b \\\\\nc &= d\n$$", "MathBlock").map((n) => n.text), ["$$\na &= b \\\\\nc &= d\n$$"]);
});

test("unclosed $$ or a blank line before the closer is not a block", () => {
  assert.equal(nodes("$$\nx = 1\n\nmore text", "MathBlock").length, 0);
  assert.equal(nodes("$$\nx = 1\n\n$$", "MathBlock").length, 0);
  assert.equal(nodes("text\n$$\nno end", "MathBlock").length, 0);
});

test("block math interrupts a paragraph and works inside a blockquote", () => {
  assert.equal(nodes("para\n$$\nx\n$$\nnext", "MathBlock").length, 1);
  assert.equal(nodes("> $$\n> x\n> $$", "MathBlock").length, 1);
});

// ---------------------------------------------------------------- decorations
function decos(doc, anchor, head = anchor) {
  const state = mdState(doc, anchor, head);
  const out = [];
  buildMath(state).decorations.between(0, doc.length, (from, to, d) => out.push({ from, to, widget: d.spec.widget, cls: d.spec.class }));
  return out;
}

test("InlineMath outside the selection yields a replace decoration with a MathWidget", () => {
  const doc = "a $x^2$ b";
  const rep = decos(doc, 0).filter((i) => i.widget instanceof MathWidget);
  assert.equal(rep.length, 1);
  assert.deepEqual([rep[0].from, rep[0].to], [2, 7]);
  assert.equal(rep[0].widget.tex, "x^2");
  assert.equal(rep[0].widget.display, false);
});

test("InlineMath with the cursor inside or touching it yields no replace decoration", () => {
  const doc = "a $x^2$ b";
  for (const pos of [2, 4, 7]) {
    const items = decos(doc, pos);
    assert.equal(items.filter((i) => i.widget).length, 0, "pos " + pos);
    assert.ok(items.some((i) => i.cls === "fw-math-src"), "raw span marked at " + pos);
  }
  assert.equal(decos(doc, 8).filter((i) => i.widget).length, 1); // one past the end: not touching
});

test("any selection range touching counts (multi-range)", () => {
  const doc = "$a$ and $b$";
  const state = EditorState.create({
    doc,
    selection: EditorSelection.create([EditorSelection.cursor(0), EditorSelection.cursor(9)]),
    extensions: [EditorState.allowMultipleSelections.of(true), ...extensions()],
  });
  let n = 0;
  buildMath(state).decorations.between(0, doc.length, (_f, _t, d) => { if (d.spec.widget) n++; });
  assert.equal(n, 0);
});

test("MathBlock outside the selection: display widget on line 1, other lines hidden and collapsed", () => {
  const doc = "x\n\n$$\na+b\n$$\n\ny";
  const items = decos(doc, 0);
  const w = items.filter((i) => i.widget instanceof MathWidget);
  assert.equal(w.length, 1);
  assert.equal(w[0].widget.display, true);
  assert.equal(w[0].widget.tex, "a+b");
  assert.deepEqual([w[0].from, w[0].to], [3, 5]);
  const hidden = items.filter((i) => !i.widget && i.to > i.from);
  assert.deepEqual(hidden.map((i) => [i.from, i.to]), [[6, 9], [10, 12]]);
  assert.equal(items.filter((i) => i.cls === "fw-math-collapsed").length, 2);
});

test("MathBlock with the cursor on any of its lines stays raw", () => {
  const doc = "x\n\n$$\na+b\n$$\n\ny";
  for (const pos of [3, 7, 11]) {
    const items = decos(doc, pos);
    assert.equal(items.filter((i) => i.widget).length, 0, "pos " + pos);
    assert.equal(items.filter((i) => i.cls === "fw-math-collapsed").length, 0);
  }
});

test("only visible ranges are decorated", () => {
  const doc = "$a$\n\n" + "filler\n".repeat(50) + "$b$";
  const state = mdState(doc, 20);
  let n = 0;
  buildMath(state, [{ from: 0, to: 10 }]).decorations.between(0, doc.length, (_f, _t, d) => { if (d.spec.widget) n++; });
  assert.equal(n, 1);
});

// ---------------------------------------------------------------- render + cache (KaTeX itself runs in node;
// only the widget's DOM (toDOM) needs a browser and is verified there)
test("renderMath renders valid TeX and caches per TeX string and mode", () => {
  clearMathCache();
  const a = renderMath("E=mc^2", false);
  assert.match(a.html, /class="katex"/);
  assert.equal(a.error, undefined);
  assert.equal(renderMath("E=mc^2", false), a, "same object from the cache");
  const d = renderMath("E=mc^2", true);
  assert.notEqual(d, a);
  assert.match(d.html, /katex-display/);
  assert.equal(mathCache.size, 2);
  clearMathCache();
  assert.equal(mathCache.size, 0);
});

test("renderMath turns a TeX error into a .fw-math-error span with the message as title", () => {
  clearMathCache();
  const r = renderMath("\\frac{1", false);
  assert.ok(r.error);
  assert.match(r.html, /^<span class="fw-math-error" title="[^"]+">\\frac\{1<\/span>$/);
  assert.equal(renderMath("\\frac{1", false), r);
  const x = renderMath('<img src=x onerror="1">\\bad', false);
  assert.ok(!x.html.includes("<img"), "TeX shown in the error span is escaped");
});
