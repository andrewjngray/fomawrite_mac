import test from "node:test";
import assert from "node:assert/strict";
import { EditorState } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { syntaxTree } from "@codemirror/language";
import { parseInline, parseTableText, buildTableDom } from "../src/tables.ts";
import { inlineMathEndIn, mathMarkdown, mathCache, clearMathCache, renderMath } from "../src/math.ts";

const txt = (v) => ({ k: "text", v });
const math = (tex) => ({ k: "math", tex });

// ---------------------------------------------------------------- segmentation
test("cell inline math: $tex$ becomes a math segment between text segments", () => {
  assert.deepEqual(parseInline("area $x^2$ units"), [txt("area "), math("x^2"), txt(" units")]);
  assert.deepEqual(parseInline("$a$ and $b$"), [math("a"), txt(" and "), math("b")]);
  assert.deepEqual(parseInline("$\\frac{1}{2}$"), [math("\\frac{1}{2}")]);
});

test("cell inline math: false positives stay text (same rules as the paragraph parser)", () => {
  assert.deepEqual(parseInline("costs $5 and $10"), [txt("costs $5 and $10")]);
  assert.deepEqual(parseInline("$5"), [txt("$5")]);
  assert.deepEqual(parseInline("a $ x$ b"), [txt("a $ x$ b")], "space after the opener");
  assert.deepEqual(parseInline("a $x $ b"), [txt("a $x $ b")], "space before the closer");
  assert.deepEqual(parseInline("$x$5"), [txt("$x$5")], "closer followed by a digit");
  assert.deepEqual(parseInline("$$"), [txt("$$")]);
  assert.deepEqual(parseInline("$$x$$"), [txt("$$x$$")], "display math is not an inline cell construct");
  assert.deepEqual(parseInline("lone $ sign"), [txt("lone $ sign")]);
});

test("cell inline math: escaped dollars are text and never delimit", () => {
  assert.deepEqual(parseInline("\\$x\\$"), [txt("$x$")]);
  assert.deepEqual(parseInline("price \\$5 and \\$10"), [txt("price $5 and $10")]);
  assert.deepEqual(parseInline("$a\\$b$"), [math("a\\$b")], "an escaped dollar inside math does not close it");
  assert.deepEqual(parseInline("\\$a$ then $b$"), [txt("$a$ then "), math("b")]);
});

test("cell inline math: its content is protected from emphasis, code, links and strike", () => {
  assert.deepEqual(parseInline("$a_b * c_d$"), [math("a_b * c_d")]);
  assert.deepEqual(parseInline("$a*b*c$"), [math("a*b*c")]);
  assert.deepEqual(parseInline("$a~~b~~c$"), [math("a~~b~~c")]);
  assert.deepEqual(parseInline("$`x`$"), [math("`x`")]);
  // an emphasis closer inside math does not close the emphasis
  assert.deepEqual(parseInline("*a $b*c$ d*"), [{ k: "em", c: [txt("a "), math("b*c"), txt(" d")] }]);
});

test("cell inline math: nests inside bold, italic, strike and link text; code spans win over math", () => {
  assert.deepEqual(parseInline("**$x$**"), [{ k: "strong", c: [math("x")] }]);
  assert.deepEqual(parseInline("_see $y_1$_"), [{ k: "em", c: [txt("see "), math("y_1")] }]);
  assert.deepEqual(parseInline("~~$z$~~"), [{ k: "del", c: [math("z")] }]);
  assert.deepEqual(parseInline("[$x$](https://a.b)"), [{ k: "link", href: "https://a.b", title: "", c: [math("x")] }]);
  assert.deepEqual(parseInline("`$x$`"), [{ k: "code", v: "$x$" }]);
  assert.deepEqual(parseInline("`a` $x$ `b`"), [{ k: "code", v: "a" }, txt(" "), math("x"), txt(" "), { k: "code", v: "b" }]);
});

test("cell inline math: an unclosed opener never throws and stays text", () => {
  for (const s of ["$", "$x", "x$", "$\\", "$a\\", "$$$", "$ ".repeat(200), "$a ".repeat(300)]) {
    assert.doesNotThrow(() => parseInline(s), JSON.stringify(s.slice(0, 20)));
  }
  assert.deepEqual(parseInline("$x"), [txt("$x")]);
});

test("cell inline math: \\| inside a cell is the table-level pipe escape and reaches KaTeX as |", () => {
  const t = parseTableText("| a | b |\n|---|---|\n| $x$ | $a\\|b$ |");
  assert.deepEqual(t.rows[0].map((c) => c.text), ["$x$", "$a\\|b$"]);
  assert.deepEqual(parseInline(t.rows[0][1].text), [math("a|b")]);
});

test("a table parsed end to end exposes math segments per cell", () => {
  const t = parseTableText("| Expr | Note |\n|:--|--|\n| $x^2$ | costs $5 and $10 |\n| \\$a\\$ | **$\\alpha$** |");
  assert.deepEqual(parseInline(t.rows[0][0].text), [math("x^2")]);
  assert.deepEqual(parseInline(t.rows[0][1].text), [txt("costs $5 and $10")]);
  assert.deepEqual(parseInline(t.rows[1][0].text), [txt("$a$")]);
  assert.deepEqual(parseInline(t.rows[1][1].text), [{ k: "strong", c: [math("\\alpha")] }]);
});

// ---------------------------------------------------------------- one rule set: agree with the Lezer paragraph parser
const lezerSpans = (doc) => {
  const state = EditorState.create({ doc, extensions: [markdown({ base: markdownLanguage, extensions: [mathMarkdown] })] });
  const out = [];
  syntaxTree(state).iterate({ enter: (n) => { if (n.name === "InlineMath") out.push(doc.slice(n.from, n.to)); } });
  return out;
};
const scannerSpans = (doc) => {
  const out = [];
  for (let i = 0; i < doc.length; i++) {
    if (doc[i] === "\\") { i++; continue; }
    if (doc[i] !== "$") continue;
    const e = inlineMathEndIn(doc, i);
    if (e > 0) { out.push(doc.slice(i, e)); i = e - 1; }
  }
  return out;
};

test("inlineMathEndIn accepts and rejects exactly what the Lezer InlineMath parser does", () => {
  for (const doc of [
    "Energy $E=mc^2$ here.", "costs $5 and $10", "a $ x$ b", "a $x $ b", "$x$5", "\\$x\\$ and $y$", "$a\\$b$",
    "$$x$$ inline", "$a$$b$", "$x\ny$", "price: $3.50 or $4", "$a$ $b$ $c$", "$\\frac{a}{b}$",
  ]) assert.deepEqual(scannerSpans(doc), lezerSpans(doc), JSON.stringify(doc));
});

// ---------------------------------------------------------------- DOM (minimal fake document)
class FakeEl {
  constructor(tag) {
    this.tag = tag; this.children = []; this.attrs = {}; this.className = ""; this.innerHTML = ""; this.textContent = ""; this.title = ""; this.rel = "";
    this.classList = { add: (c) => { this.className = (this.className + " " + c).trim(); } };
  }
  setAttribute(k, v) { this.attrs[k] = v; }
  appendChild(c) { this.children.push(c); return c; }
}
const fakeDoc = { createElement: (t) => new FakeEl(t), createTextNode: (v) => ({ text: v }) };
const find = (e, pred, out = []) => { if (e && e.tag !== undefined && pred(e)) out.push(e); for (const c of e.children ?? []) find(c, pred, out); return out; };

test("buildTableDom: math in a cell becomes a KaTeX span (fw-math-inline), plain dollars do not", () => {
  clearMathCache();
  const t = parseTableText("| h |\n|---|\n| $x^2$ |\n| costs $5 and $10 |\n| $\\badmacro{$ |");
  const dom = buildTableDom(t, fakeDoc);
  const spans = find(dom, (e) => e.tag === "span" && e.className.includes("fw-math-render"));
  assert.equal(spans.length, 2, "x^2 and the failing one; the dollar-price cell has none");
  assert.match(spans[0].className, /fw-math-inline/);
  assert.match(spans[0].innerHTML, /class="katex"/);
  assert.doesNotMatch(spans[0].className, /fw-math-failed/);
  assert.match(spans[1].className, /fw-math-failed/, "a TeX error still renders, with the error span");
  assert.match(spans[1].innerHTML, /fw-math-error/);
  const cells = find(dom, (e) => e.tag === "td");
  assert.deepEqual(cells[1].children, [{ text: "costs $5 and $10" }]);
});

test("table cells reuse math.ts's render cache", () => {
  clearMathCache();
  const t = parseTableText("| h |\n|---|\n| $q^3$ |");
  buildTableDom(t, fakeDoc);
  assert.ok(mathCache.has("I:q^3"), "rendered through renderMath (inline key)");
  const before = renderMath("q^3", false);
  buildTableDom(t, fakeDoc);
  assert.equal(renderMath("q^3", false), before, "second render hits the same cache entry");
});
