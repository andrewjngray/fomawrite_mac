import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, EditorSelection } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { buildLiveDecorations, buildLive, isHiddenDecoration } from "../src/live.ts";

const DOC = [
  "# Heading one", // 0..13
  "",
  "Plain **bold** and *ital* and `code` and [label](http://u.rl) end.",
  "",
  "> quoted text",
  "",
  "- list item",
  "",
].join("\n");

function stateAt(doc, anchor, head = anchor) {
  return EditorState.create({
    doc,
    selection: EditorSelection.single(anchor, head),
    extensions: [markdown({ base: markdownLanguage })],
  });
}

function collect(state, selection) {
  const set = buildLiveDecorations(state, undefined, selection);
  const hidden = [], classes = [], lines = [];
  set.between(0, state.doc.length, (from, to, d) => {
    if (isHiddenDecoration(d)) hidden.push([from, to]);
    else if (d.spec.class && from === to) lines.push([from, d.spec.class]);
    else if (d.spec.class) classes.push([from, to, d.spec.class]);
  });
  const text = (r) => state.doc.sliceString(r[0], r[1]);
  return { hidden, classes, lines, hiddenText: hidden.map(text) };
}

const idx = (s) => DOC.indexOf(s);

test("selection outside every node: markers hidden", () => {
  // Cursor in the trailing empty line: touches no node.
  const state = stateAt(DOC, DOC.length);
  const { hiddenText, lines } = collect(state);
  assert.ok(hiddenText.includes("# "), "heading mark and following space hidden: " + JSON.stringify(hiddenText));
  assert.equal(hiddenText.filter((t) => t === "**").length, 2, "both ** hidden");
  assert.equal(hiddenText.filter((t) => t === "*").length, 2, "both * hidden");
  assert.equal(hiddenText.filter((t) => t === "`").length, 2, "both ` hidden");
  assert.ok(hiddenText.includes("[") , "[ hidden");
  assert.ok(hiddenText.includes("](http://u.rl)"), "](url) hidden: " + JSON.stringify(hiddenText));
  assert.ok(hiddenText.includes("> "), "quote mark hidden");
  // List mark is not hidden.
  assert.ok(!hiddenText.some((t) => t.startsWith("- ")));
  // Line decorations
  const lineClasses = lines.map((l) => l[1]);
  assert.ok(lineClasses.includes("fw-h1"));
  assert.ok(lineClasses.includes("fw-quote-line"));
  assert.ok(lineClasses.includes("fw-list-line"));
});

test("hidden ranges have exact offsets", () => {
  const { hidden } = collect(stateAt(DOC, DOC.length));
  const b = idx("**bold**");
  assert.ok(hidden.some(([f, t]) => f === 0 && t === 2));
  assert.ok(hidden.some(([f, t]) => f === b && t === b + 2));
  assert.ok(hidden.some(([f, t]) => f === b + 6 && t === b + 8));
  const lk = idx("[label]");
  assert.ok(hidden.some(([f, t]) => f === lk && t === lk + 1));
  const close = idx("](http");
  assert.ok(hidden.some(([f, t]) => f === close && t === idx("(http://u.rl)") + "(http://u.rl)".length));
});

test("inline style marks and link label class are present", () => {
  const { classes } = collect(stateAt(DOC, DOC.length));
  const has = (cls, s) => classes.some(([f, t, c]) => c === cls && DOC.slice(f, t) === s);
  assert.ok(has("fw-strong", "**bold**"));
  assert.ok(has("fw-em", "*ital*"));
  assert.ok(has("fw-code", "`code`"));
  assert.ok(has("fw-live-link", "label"));
  assert.ok(!classes.some(([, , c]) => c === "fw-revealed"));
});

test("selection inside bold reveals its markers; others stay hidden", () => {
  const b = idx("**bold**");
  const state = stateAt(DOC, b + 4); // inside "bold"
  const { hidden, classes } = collect(state);
  assert.ok(!hidden.some(([f, t]) => f >= b && t <= b + 8), "bold markers not hidden");
  assert.ok(classes.some(([f, t, c]) => c === "fw-revealed" && f === b && t === b + 8));
  // Neighbours still hidden
  assert.ok(hidden.some(([f, t]) => DOC.slice(f, t) === "*"));
  assert.ok(hidden.some(([f, t]) => DOC.slice(f, t) === "# "));
});

test("reveal is inclusive at node boundaries", () => {
  const b = idx("**bold**");
  for (const pos of [b, b + 8]) {
    const { hidden, classes } = collect(stateAt(DOC, pos));
    assert.ok(!hidden.some(([f]) => f === b), `pos ${pos}: opening marker visible`);
    assert.ok(classes.some(([, , c]) => c === "fw-revealed"));
  }
});

test("any selection range counts (multi-range)", () => {
  const b = idx("**bold**"), i = idx("*ital*");
  const state = EditorState.create({
    doc: DOC,
    selection: EditorSelection.create([EditorSelection.cursor(b + 3), EditorSelection.cursor(i + 2)]),
    extensions: [markdown({ base: markdownLanguage }), EditorState.allowMultipleSelections.of(true)],
  });
  const { hidden } = collect(state);
  assert.ok(!hidden.some(([f]) => f === b || f === i));
  assert.ok(hidden.some(([f, t]) => DOC.slice(f, t) === "`"));
});

test("selection in link reveals the whole link", () => {
  const lk = idx("[label]");
  const { hidden } = collect(stateAt(DOC, lk + 3));
  assert.ok(!hidden.some(([f]) => f >= lk && f < idx(" end.")));
});

test("heading reveal when cursor on heading line", () => {
  const { hidden, classes } = collect(stateAt(DOC, 5));
  assert.ok(!hidden.some(([f]) => f === 0));
  assert.ok(classes.some(([, , c]) => c === "fw-revealed"));
});

test("only visible ranges are decorated", () => {
  const state = stateAt(DOC, DOC.length);
  const set = buildLiveDecorations(state, [{ from: 0, to: 13 }]);
  let any = false;
  set.between(14, DOC.length, () => { any = true; });
  assert.equal(any, false);
  const r = buildLive(state, [{ from: 0, to: 13 }]);
  assert.ok(r.hidden.size > 0);
});

test("nested emphasis and partial/invalid markdown never throw", () => {
  const docs = [
    "***bold italic*** and **bold *nested* bold** and *a **b** c*",
    "**unclosed bold and `unclosed code and [unclosed link](",
    "# \n#\n####### seven\n> \n>\n- \n1.\n```\nunclosed fence",
    "[a][ref] [short] ![img](u) <http://auto.link> | a | b |\n|---|---|\n| 1 | 2 |",
    "~~strike~~ ~single~ - [ ] task\n- [x] done",
    "",
    "\n\n\n",
    "emoji 👨‍👩‍👧‍👦 **bold 😀** 你好 *世界*",
  ];
  for (const d of docs) {
    for (const pos of [0, Math.floor(d.length / 2), d.length]) {
      const state = stateAt(d, pos);
      assert.doesNotThrow(() => buildLiveDecorations(state), JSON.stringify(d));
    }
  }
});

test("nested emphasis hides every marker when cursor is elsewhere", () => {
  const d = "x\n\n***bold italic***\n";
  const { hiddenText } = collect(stateAt(d, 0));
  assert.equal(hiddenText.join(""), "***" + "***");
});

test("hidden ranges never span line breaks", () => {
  const d = "[multi\nline label](http://a.b) and **bold\nacross** lines\n\nend";
  const { hidden } = collect(stateAt(d, d.length));
  for (const [f, t] of hidden) assert.ok(!d.slice(f, t).includes("\n"));
});
