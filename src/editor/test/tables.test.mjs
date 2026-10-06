import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, EditorSelection } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { ensureSyntaxTree } from "@codemirror/language";
import {
  splitRow, parseAlignments, parseTableText, parseInline, safeHref,
  findTables, buildTableDecorations, TableWidget,
} from "../src/tables.ts";

// ---------------------------------------------------------------- pure parser
test("splitRow: optional outer pipes, trimming, offsets and click positions", () => {
  const cells = splitRow("| a | bb |", 100);
  assert.deepEqual(cells.map((c) => c.text), ["a", "bb"]);
  assert.deepEqual(cells.map((c) => [c.from, c.to, c.at]), [[102, 103, 102], [106, 108, 106]]);
  assert.deepEqual(splitRow("a | b").map((c) => c.text), ["a", "b"]);
  assert.deepEqual(splitRow("  | a | b |  ").map((c) => c.text), ["a", "b"]);
});

test("splitRow: escaped pipes stay in the cell, empty cells survive", () => {
  assert.deepEqual(splitRow("| a \\| b | c |").map((c) => c.text), ["a \\| b", "c"]);
  assert.deepEqual(splitRow("| a \\\\| b |").map((c) => c.text), ["a \\\\", "b"], "an escaped backslash does not escape the pipe");
  const e = splitRow("| 1 |  | 3 |");
  assert.deepEqual(e.map((c) => c.text), ["1", "", "3"]);
  assert.equal(e[1].at, 6, "empty cell: caret just inside it");
  assert.deepEqual(splitRow("| a | |").map((c) => c.text), ["a", ""], "trailing empty cell before the last pipe");
  assert.deepEqual(splitRow("   "), []);
});

test("parseAlignments: left, center, right, none; non-delimiter rows are rejected", () => {
  assert.deepEqual(parseAlignments("|:---|:---:|---:|---|"), ["left", "center", "right", null]);
  assert.deepEqual(parseAlignments("a|b"), null);
  assert.deepEqual(parseAlignments("| - | -- |"), [null, null]);
  assert.equal(parseAlignments("| : | --- |"), null);
});

test("parseTableText: header, alignments, body rows with absolute offsets", () => {
  const src = "| Name | Qty |\n|:--|--:|\n| apple | 3 |\n| pear | 10 |";
  const t = parseTableText(src);
  assert.deepEqual(t.aligns, ["left", "right"]);
  assert.deepEqual(t.header.map((c) => c.text), ["Name", "Qty"]);
  assert.deepEqual(t.rows.map((r) => r.map((c) => c.text)), [["apple", "3"], ["pear", "10"]]);
  for (const c of [...t.header, ...t.rows.flat()]) assert.equal(src.slice(c.from, c.to), c.text);
  assert.equal(src.slice(t.rows[1][1].at).startsWith("10"), true);
  const shifted = parseTableText(src, 50);
  assert.equal(shifted.rows[0][0].at, t.rows[0][0].at + 50);
});

test("parseTableText: ragged rows are padded / truncated to the delimiter's column count", () => {
  const t = parseTableText("| a | b | c |\n|---|---|---|\n| 1 |\n| 1 | 2 | 3 | 4 | 5 |\n\n| x | y | z |");
  assert.deepEqual(t.rows.map((r) => r.map((c) => c.text)), [["1", "", ""], ["1", "2", "3"], ["x", "y", "z"]]);
  assert.equal(t.rows[0].length, 3);
  const src = "| a | b | c |\n|---|---|---|\n| 1 |";
  const short = parseTableText(src);
  assert.equal(short.rows[0][2].at, src.length, "padding cells point at the end of their line");
});

test("parseTableText: escaped pipes inside cells", () => {
  const t = parseTableText("| expr | note |\n|---|---|\n| a \\| b | `x\\|y` |");
  assert.deepEqual(t.rows[0].map((c) => c.text), ["a \\| b", "`x\\|y`"]);
});

test("parseTableText: malformed input yields null, never throws", () => {
  for (const s of ["", "| a |", "| a |\n| b |", "| a | b |\n|---|--x|", "\n\n", "| a |\n|---|\\", "|||\n|||"]) {
    let r;
    assert.doesNotThrow(() => (r = parseTableText(s)), JSON.stringify(s));
    if (s !== "|||\n|||") assert.equal(r, null, JSON.stringify(s));
  }
});

// ---------------------------------------------------------------- inline
test("parseInline: strong, em, code, strike, links, escapes", () => {
  assert.deepEqual(parseInline("a **b** *c* `d` ~~e~~ f"), [
    { k: "text", v: "a " }, { k: "strong", c: [{ k: "text", v: "b" }] }, { k: "text", v: " " },
    { k: "em", c: [{ k: "text", v: "c" }] }, { k: "text", v: " " }, { k: "code", v: "d" }, { k: "text", v: " " },
    { k: "del", c: [{ k: "text", v: "e" }] }, { k: "text", v: " f" },
  ]);
  assert.deepEqual(parseInline("[x](https://a.b/c \"T\")"), [
    { k: "link", href: "https://a.b/c", title: "T", c: [{ k: "text", v: "x" }] },
  ]);
  assert.deepEqual(parseInline("a \\| b \\*c\\*"), [{ k: "text", v: "a | b *c*" }]);
  assert.deepEqual(parseInline("***x***"), [{ k: "strong", c: [{ k: "em", c: [{ k: "text", v: "x" }] }] }]);
  assert.deepEqual(parseInline("**bold with `code`**"), [{ k: "strong", c: [{ k: "text", v: "bold with " }, { k: "code", v: "code" }] }]);
});

test("parseInline: unmatched markers and snake_case stay literal; html is just text", () => {
  assert.deepEqual(parseInline("2 * 3 and snake_case_name"), [{ k: "text", v: "2 * 3 and snake_case_name" }]);
  assert.deepEqual(parseInline("**open"), [{ k: "text", v: "**open" }]);
  assert.deepEqual(parseInline("<script>alert(1)</script>"), [{ k: "text", v: "<script>alert(1)</script>" }]);
  assert.deepEqual(parseInline("a<br>b"), [{ k: "text", v: "a" }, { k: "br" }, { k: "text", v: "b" }]);
  assert.deepEqual(parseInline("`<b>`"), [{ k: "code", v: "<b>" }]);
});

test("parseInline: unsafe link schemes are dropped to plain text; safeHref", () => {
  assert.deepEqual(parseInline("[x](javascript:alert(1))"), [{ k: "text", v: "x" }]);
  assert.deepEqual(parseInline("[x](data:text/html,hi)"), [{ k: "text", v: "x" }]);
  assert.equal(safeHref("java\nscript:alert(1)"), null);
  assert.equal(safeHref("HTTP://a.b"), "HTTP://a.b");
  assert.equal(safeHref("mailto:a@b.c"), "mailto:a@b.c");
  assert.equal(safeHref("../rel/path.md"), "../rel/path.md");
  assert.deepEqual(parseInline("see https://a.b/c, ok"), [
    { k: "text", v: "see " }, { k: "link", href: "https://a.b/c", title: "", c: [{ k: "text", v: "https://a.b/c" }] }, { k: "text", v: ", ok" },
  ]);
});

test("parseInline: pathological input terminates", () => {
  const s = "*".repeat(500) + "[".repeat(300) + "`".repeat(200) + "_a_".repeat(200);
  assert.doesNotThrow(() => parseInline(s));
  assert.doesNotThrow(() => parseInline("*a ".repeat(300) + "**b".repeat(300)));
});

// ---------------------------------------------------------------- builder
function stateAt(doc, anchor, head = anchor) {
  const state = EditorState.create({
    doc,
    selection: EditorSelection.single(anchor, head),
    extensions: [markdown({ base: markdownLanguage })],
  });
  ensureSyntaxTree(state, doc.length, 1000); // finish the parse so syntaxTree(state) sees every table
  return state;
}

function collect(state, selection) {
  const out = [];
  buildTableDecorations(state, selection).between(0, state.doc.length, (from, to, d) => {
    out.push({ from, to, widget: d.spec.widget, block: d.spec.block });
  });
  return out;
}

const DOC = "intro\n\n| A | B |\n|:-:|--:|\n| 1 | 2 |\n| 3 | 4 |\n\nouter\n\nnot | a table\n";
const T_FROM = DOC.indexOf("| A");
const T_TO = DOC.indexOf("\n\nouter");

test("findTables: one top-level table with exact line bounds and parsed cells", () => {
  const tables = findTables(stateAt(DOC, 0));
  assert.equal(tables.length, 1);
  assert.equal(tables[0].from, T_FROM);
  assert.equal(tables[0].to, T_TO);
  assert.deepEqual(tables[0].table.aligns, ["center", "right"]);
  assert.equal(tables[0].table.rows.length, 2);
});

test("selection outside the table: one block replace spanning the whole table (all lines)", () => {
  for (const pos of [0, 5, T_FROM - 1, T_TO + 1, DOC.length]) {
    const items = collect(stateAt(DOC, pos));
    assert.equal(items.length, 1, `pos ${pos}`);
    const [it] = items;
    assert.equal(it.from, T_FROM);
    assert.equal(it.to, T_TO);
    assert.ok(DOC.slice(it.from, it.to).includes("\n"), "replace spans line breaks");
    assert.equal(it.block, true);
    assert.ok(it.widget instanceof TableWidget);
    assert.ok(it.widget.estimatedHeight > 0);
    assert.equal(it.widget.table.header[1].text, "B");
  }
});

test("selection touching the table (inclusive, any range) yields no table replacement", () => {
  for (const pos of [T_FROM, T_FROM + 3, DOC.indexOf("| 3") + 2, T_TO]) {
    assert.equal(collect(stateAt(DOC, pos)).length, 0, `caret at ${pos}`);
  }
  assert.equal(collect(stateAt(DOC, 0, T_FROM + 1)).length, 0, "selection reaching into the table");
  assert.equal(collect(stateAt(DOC, 0, DOC.length)).length, 0, "selection covering the table");
  const multi = EditorSelection.create([EditorSelection.cursor(0), EditorSelection.cursor(T_FROM + 4)]);
  assert.equal(collect(stateAt(DOC, 0), multi).length, 0, "a secondary caret inside the table");
});

test("the widget is identified by the table source (unchanged table keeps its DOM)", () => {
  const a = collect(stateAt(DOC, 0))[0].widget;
  const b = collect(stateAt("x\n\n" + DOC.slice(T_FROM, T_TO) + "\n", 0))[0].widget;
  assert.ok(a.eq(b));
  const c = collect(stateAt(DOC.replace("| 3 |", "| 33 |"), 0))[0].widget;
  assert.ok(!a.eq(c));
});

test("several tables, unparsable and nested ones", () => {
  const d = "| a |\n|---|\n| 1 |\n\ntext\n\n| b |\n|---|\n| 2 |\n\n> | q |\n> |---|\n> | 3 |\n\n- | l |\n  |---|\n  | 4 |\n";
  const second = d.indexOf("| b");
  const items = collect(stateAt(d, second + 1));
  assert.deepEqual(items.map((i) => i.from), [0], "caret in the second table: only the first is rendered; quote/list tables stay raw");
  assert.equal(collect(stateAt(d, d.length)).length, 2);
});

test("indented table: the replacement starts at the line start", () => {
  const d = "para\n\n  | a | b |\n  |---|---|\n  | 1 | |\n\nend";
  const items = collect(stateAt(d, 0));
  assert.equal(items.length, 1);
  assert.equal(items[0].from, d.indexOf("  | a"));
  assert.equal(items[0].to, d.indexOf("|\n\nend") + 1);
});

test("a table that is not one (no delimiter row, plain paragraph) stays raw; the builder never throws", () => {
  const d = "| a | b |\n| 1 | 2 |\n\nx | y\n\n| h |\n|:x:|\n";
  assert.equal(collect(stateAt(d, d.length)).length, 0);
  assert.doesNotThrow(() => collect(stateAt("", 0)));
});
