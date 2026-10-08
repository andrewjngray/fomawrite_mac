// `==highlight==` in Live: the parser, the decorations, the theme mapping and the readable default pair.
import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { EditorSelection, EditorState } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { buildLiveDecorations, isHiddenDecoration } from "../src/live.ts";
import { extrasMarkdown } from "../src/extras.ts";
import { mapThemeCss } from "../src/thememap.ts";
import { contrastRatio } from "../src/livetheme.ts";

const EDITOR_CSS = readFileSync(new URL("../editor.css", import.meta.url), "utf8");

function decorate(doc, caret) {
  const state = EditorState.create({
    doc,
    selection: EditorSelection.single(caret ?? doc.length),
    extensions: [markdown({ base: markdownLanguage, extensions: [extrasMarkdown] })],
  });
  const classes = [], hidden = [];
  buildLiveDecorations(state).between(0, doc.length, (from, to, d) => {
    if (isHiddenDecoration(d)) hidden.push(doc.slice(from, to));
    else if (d.spec.class && from < to) classes.push([d.spec.class, doc.slice(from, to)]);
  });
  return { classes, hidden };
}
const marked = (r) => r.classes.filter(([c]) => c === "fw-highlight").map(([, t]) => t);

test("==text== is a highlight mark with both == hidden while the caret is elsewhere", () => {
  const doc = "plain ==hi there== end\n\nlast";
  const r = decorate(doc, doc.length);
  assert.deepEqual(marked(r), ["==hi there=="]);
  assert.deepEqual(r.hidden, ["==", "=="]);
});

test("a caret touching the highlight reveals the markers and keeps the mark", () => {
  const doc = "plain ==hi there== end";
  const r = decorate(doc, 10);
  assert.deepEqual(marked(r), ["==hi there=="]);
  assert.ok(r.classes.some(([c]) => c === "fw-revealed"));
  assert.deepEqual(r.hidden, []);
});

test("nested inline formatting inside a highlight still renders", () => {
  const r = decorate("==a **b** c==\n\nx", 15);
  assert.deepEqual(marked(r), ["==a **b** c=="]);
  assert.ok(r.classes.some(([c, t]) => c === "fw-strong" && t === "**b**"));
});

test("operators and longer runs are not highlights", () => {
  for (const doc of ["a == b == c\n\nx", "x ===y=== z\n\nx", "a ==\nb==\n\nx", "\\==nope==\n\nx", "`==code==`\n\nx", "== a ==\n\nx"]) {
    assert.deepEqual(marked(decorate(doc, doc.length)), [], JSON.stringify(doc));
  }
});

test("the theme's mark rule reaches Live with its background and colour together", () => {
  const css = mapThemeCss("mark { background: var(--highlight-color); color: #112233; padding: 2px; border-radius: 3px }");
  assert.match(css, /#write\.fw-mode-live \.fw-highlight\s*\{[^}]*background-color: var\(--highlight-color\)/);
  assert.match(css, /\.fw-highlight\s*\{[^}]*color: #112233/);
  assert.ok(!/padding|border-radius/.test(css), css);
});

test("the editor's own highlight pair is dark text on yellow and reads at 4.5:1 or better", () => {
  const pick = (name) => new RegExp(`--${name}:\\s*(#[0-9a-f]{6})`, "i").exec(EDITOR_CSS)?.[1];
  const bg = pick("fw-highlight-bg"), fg = pick("fw-highlight-fg");
  assert.ok(bg && fg, "both variables are defined on :root");
  assert.ok(contrastRatio(fg, bg) >= 4.5, `${fg} on ${bg}: ${contrastRatio(fg, bg)}`);
  assert.match(EDITOR_CSS, /#write\.fw-mode-live \.fw-highlight\s*\{[^}]*background-color: var\(--fw-highlight-bg\)[^}]*color: var\(--fw-highlight-fg\)/);
  // not re-defined for html.dark: the pair is the same on a dark page
  assert.ok(!/:root\.dark\s*\{[^}]*--fw-highlight/.test(EDITOR_CSS));
});

test("a mark colour that means 'no colour' is dropped so the readable highlight text stands", () => {
  for (const none of ["inherit", "currentcolor", "unset", "initial", "inherit !important"]) {
    const css = mapThemeCss(`mark { background: var(--highlight-color); color: ${none}; border-radius: 1px }`);
    assert.match(css, /\.fw-highlight\s*\{\s*background-color: var\(--highlight-color\);\s*\}/, none + ": " + css);
    assert.ok(!/(?<!background-)color: (?:inherit|currentcolor|unset|initial)/.test(css), none);
  }
  // other elements keep a colour keyword as written
  assert.match(mapThemeCss("em { color: inherit }"), /\.fw-em\s*\{\s*color: inherit/);
});
