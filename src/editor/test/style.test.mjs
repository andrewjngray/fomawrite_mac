import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { EditorSelection, EditorState } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import {
  SpellEngine, choiceSpec, dropGrammarSpec, dropWordSpec, grammarAt, grammarMenuModel, grammarRanges, markedRanges, misspelledRanges,
  parseSpellRanges, setSpellEffect, spellField, withholdCaretWord,
} from "../src/spelling.ts";
import { createMockBridge } from "../src/bridge.ts";
import { computed as cascade, el as celem, parseRules } from "./cascade.mjs";
import { LIVE_OVERLAY_CSS, LIVE_ROOT_CLASS } from "../src/livetheme.ts";
import { mapThemeCss } from "../src/thememap.ts";

// Style, the third mark category (grey dotted, `.fw-style`): a grammar-like finding with its own look.

const DOC = "It is very unique. We recieve the mail.";
const VERY_UNIQUE = { from: 6, to: 17 };
const RECIEVE = { from: 22, to: 29 };

const stateOf = (doc, anchor = 0) =>
  EditorState.create({ doc, selection: EditorSelection.single(anchor), extensions: [markdown({ base: markdownLanguage }), spellField] });
const whole = (state) => ({ from: 0, to: state.doc.length });
const apply = (state, spec) => state.update(spec).state;
const style = (r, message = "Wording", suggestions = ["unique"]) => ({ ...r, category: "Style", message, suggestions });
const grammar = (r, message = "Grammar thing", suggestions = ["x"]) => ({ ...r, category: "Grammar", message, suggestions });
const spelling = (r) => ({ ...r, category: "Spelling" });
const reply = (...entries) => JSON.stringify(entries);
const sent = () => { const out = { check: [] }; return { out, checkSpelling: (token, json) => out.check.push({ token, segments: JSON.parse(json) }) }; };
function checked(engine, bridge, state, json) {
  engine.request(state, whole(state));
  const spec = engine.reply(state, bridge.out.check.at(-1).token, json);
  return spec ? apply(state, spec) : state;
}
/** The CSS class each mark is drawn with, in document order. */
const classes = (state) => {
  const out = [];
  state.field(spellField).between(0, state.doc.length, (from, to, v) => { out.push([from, to, v.spec.class]); });
  return out;
};

test("a Style entry in a reply becomes a style mark with its message and corrections; the shape of the other categories is unchanged", () => {
  const out = parseSpellRanges(reply(
    style({ from: 6, to: 17 }, "Say it plainly", ["unique", "rare", "unique", "", 4]),
    grammar({ from: 0, to: 2 }),
    spelling({ from: 22, to: 29 }),
    { from: 18, to: 20, category: "Style" }, // no message, no corrections: still a mark
    { from: 30, to: 99, category: "Style", message: "past the end" },
    { from: 3, to: 3, category: "Style" },
  ), 40);
  assert.deepEqual(out, [
    { from: 0, to: 2, grammar: { message: "Grammar thing", suggestions: ["x"] } },
    { from: 6, to: 17, grammar: { message: "Say it plainly", suggestions: ["unique", "rare"], style: true } },
    { from: 18, to: 20, grammar: { message: "", suggestions: [], style: true } },
    { from: 22, to: 29 },
  ]);
  assert.ok(!("style" in out[0].grammar), "a grammar finding is not a style one");
});

test("style marks are drawn with class fw-style, grammar with fw-grammar, spelling with fw-misspelled", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  const s = checked(engine, bridge, stateOf(DOC), reply(style(VERY_UNIQUE), grammar({ from: 0, to: 2 }), spelling(RECIEVE)));
  assert.deepEqual(classes(s), [[0, 2, "fw-grammar"], [6, 17, "fw-style"], [22, 29, "fw-misspelled"]]);
  assert.deepEqual(misspelledRanges(s), [RECIEVE]);
  assert.deepEqual(grammarRanges(s).map((r) => [r.from, !!r.style]), [[0, false], [6, true]], "style marks are listed with the grammar ones, flagged");
});

test("overlap: Spelling over Grammar over Style (lookups; the stylesheet is checked below)", () => {
  let s = stateOf("A very unique recieve here.", 0);
  const text = (r) => s.doc.sliceString(r.from, r.to);
  // style 2..13 ("very unique"), grammar 7..13 ("unique"), spelling 14..21 ("recieve")
  s = apply(s, {
    effects: setSpellEffect.of({
      ...whole(s), all: true,
      ranges: [
        { from: 2, to: 13, grammar: { message: "wordy", suggestions: ["unique"], style: true } },
        { from: 7, to: 13, grammar: { message: "agreement", suggestions: [] } },
        { from: 14, to: 21 },
      ],
    }),
  });
  assert.equal(grammarAt(s, 10).message, "agreement", "inside both: the Grammar one");
  assert.equal(text(grammarAt(s, 4)), "very unique", "where only Style covers: Style");
  assert.equal(grammarAt(s, 13).message, "agreement", "at an edge both reach: Grammar still wins");
  assert.equal(grammarAt(s, 2).style, true);
  // the same range found by both: the Grammar mark is the one a click reaches
  s = apply(s, { effects: setSpellEffect.of({ ...whole(s), all: true, ranges: [{ from: 2, to: 6, grammar: { message: "s", suggestions: [], style: true } }, { from: 2, to: 6, grammar: { message: "g", suggestions: [] } }] }) });
  assert.equal(grammarAt(s, 4).message, "g");
});

test("the menu model of a Style finding: titled Style, the message line, up to five corrections, Ignore Style Suggestion (the same ignore choice)", () => {
  const m = grammarMenuModel({ message: "Say it plainly", suggestions: ["a", "b", "c", "d", "e", "f"], style: true });
  assert.equal(m.label, "Style");
  assert.equal(m.header, "Say it plainly");
  assert.deepEqual(m.corrections.map((c) => [c.text, c.choice.kind]), ["a", "b", "c", "d", "e"].map((w) => [w, "grammarSuggestion"]));
  assert.deepEqual(m.footer.map((f) => [f.cls, f.text, f.choice.kind]), [["fw-spell-ignore-grammar", "Ignore Style Suggestion", "ignoreGrammar"]]);
  assert.equal(grammarMenuModel({ message: " ", suggestions: [], style: true }).header, "Possible style suggestion");
  // grammar's own menu is untouched
  const g = grammarMenuModel({ message: "m", suggestions: [] });
  assert.equal(g.label, "Grammar");
  assert.deepEqual(g.footer.map((f) => f.text), ["Ignore Grammar Issue"]);
});

test("a style correction replaces the text as an input.grammar user change (reported, undoable) and is refused when the text moved", () => {
  const s = stateOf(DOC);
  const spec = choiceSpec(s, VERY_UNIQUE, "very unique", { kind: "grammarSuggestion", word: "unique" });
  assert.equal(s.update(spec).state.doc.toString(), "It is unique. We recieve the mail.");
  assert.equal(choiceSpec(s, VERY_UNIQUE, "very rare", { kind: "grammarSuggestion", word: "rare" }), null);
});

test("Ignore Style Suggestion: the text is remembered per document, its marks go, later replies are filtered; spelling marks stay", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  let s = checked(engine, bridge, stateOf(DOC), reply(style(VERY_UNIQUE), spelling(RECIEVE)));
  s = apply(s, engine.ignoreGrammar(s, "very unique")); // the page's one ignore choice serves Grammar and Style
  assert.deepEqual(grammarRanges(s), []);
  assert.deepEqual(misspelledRanges(s), [RECIEVE]);
  assert.ok(engine.ignoredGrammar.has("very unique"));
  s = checked(engine, bridge, s, reply(style(VERY_UNIQUE), spelling(RECIEVE)));
  assert.deepEqual(grammarRanges(s), [], "filtered from the next reply");
  engine.clearIgnored();
  s = checked(engine, bridge, s, reply(style(VERY_UNIQUE), spelling(RECIEVE)));
  assert.equal(grammarRanges(s).length, 1, "a new document starts with nothing ignored");
  // Learn / Ignore Spelling never touch style marks; dropGrammarSpec never touches spelling ones
  assert.equal(markedRanges(apply(s, dropWordSpec(s, "recieve"))).filter((r) => r.grammar?.style).length, 1);
  assert.equal(misspelledRanges(apply(s, dropGrammarSpec(s, "very unique"))).length, 1);
});

test("the host's ignoreGrammar(text, message) slot is the one Style uses (mock bridge)", () => {
  const mock = createMockBridge();
  mock.ignoreGrammar("very unique", "Say it plainly");
  assert.deepEqual(mock.calls.filter((c) => c.name === "ignoreGrammar").map((c) => c.args), [["very unique", "Say it plainly"]]);
});

test("the caret rule covers style: a range the empty caret ends is withheld until the caret leaves, then marked", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  const doc = "It is very unique";
  const s = stateOf(doc, doc.length); // typing at the end of "unique"
  const done = checked(engine, bridge, s, reply(style({ from: 6, to: 17 })));
  assert.deepEqual(grammarRanges(done), [], "withheld while the caret is at its end");
  assert.equal(engine.withheldAt, 17);
  const { keep, withheldAt } = withholdCaretWord([{ from: 6, to: 17, grammar: { message: "", suggestions: [], style: true } }], { empty: true, head: 3 });
  assert.equal(keep.length, 1);
  assert.equal(withheldAt, null);
});

// ---------------------------------------------------------------- CSS
test("fw-style is a grey dotted underline; on overlap Spelling beats Grammar beats Style (real cascade, filtered and exact Live)", () => {
  const css = readFileSync(new URL("../editor.css", import.meta.url), "utf8");
  assert.match(css, /\.fw-style \{ text-decoration: underline dotted #8a8f98; text-decoration-skip-ink: none; \}/);
  const theme = "#write { text-decoration: none } p { text-decoration: none } a { text-decoration: none; color: red } body { text-decoration: none }";
  const mapped = mapThemeCss(theme);
  for (const exact of [false, true]) {
    const rules = [...parseRules(css), ...parseRules(theme, 10000), ...parseRules(mapped, 20000), ...(exact ? [] : parseRules(LIVE_OVERLAY_CSS, 30000))];
    const html = celem("html", { classes: [LIVE_ROOT_CLASS] });
    const write = celem("div", { id: "write", parent: celem("body", { parent: html }), classes: ["fw-mode-live"].concat(exact ? ["fw-live-exact"] : []) });
    const line = celem("div", { parent: write, classes: ["cm-line"] });
    const td = (e) => cascade("text-decoration", e, rules, { inherited: false });
    const span = (parent, ...cls) => celem("span", { parent, classes: cls });
    const why = " (exact=" + exact + ")";
    assert.equal(td(span(line, "fw-style")), "underline dotted #8a8f98", "style alone" + why);
    assert.equal(td(span(span(line, "fw-live-link"), "fw-style")), "underline dotted #8a8f98", "style in a link" + why);
    // one element carrying several classes (CodeMirror merges marks over the same range), in either class order
    for (const order of [["fw-style", "fw-grammar"], ["fw-grammar", "fw-style"]]) assert.equal(td(span(line, ...order)), "underline wavy #1a73e8", "grammar over style on one element" + why);
    for (const order of [["fw-style", "fw-misspelled"], ["fw-misspelled", "fw-style"]]) assert.equal(td(span(line, ...order)), "underline wavy #d93025", "spelling over style on one element" + why);
    for (const order of [["fw-style", "fw-grammar", "fw-misspelled"], ["fw-misspelled", "fw-grammar", "fw-style"], ["fw-grammar", "fw-style", "fw-misspelled"]])
      assert.equal(td(span(line, ...order)), "underline wavy #d93025", "all three on one element: spelling" + why);
    // a style span inside a spelling or grammar span adds no line of its own
    assert.equal(td(span(span(line, "fw-misspelled"), "fw-style")), "none", "style inside spelling" + why);
    assert.equal(td(span(span(line, "fw-grammar"), "fw-style")), "none", "style inside grammar" + why);
    // a spelling or grammar span inside a longer style span keeps its own colour
    assert.equal(td(span(span(line, "fw-style"), "fw-misspelled")), "underline wavy #d93025", "spelling inside style" + why);
    assert.equal(td(span(span(line, "fw-style"), "fw-grammar")), "underline wavy #1a73e8", "grammar inside style" + why);
    // and the existing pairings are as they were
    assert.equal(td(span(line, "fw-grammar", "fw-misspelled")), "underline wavy #d93025");
    assert.equal(td(span(span(line, "fw-misspelled"), "fw-grammar")), "none");
  }
});
