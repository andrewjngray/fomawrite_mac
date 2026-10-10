import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { EditorSelection, EditorState, Transaction } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import {
  SpellEngine, choiceSpec, dropWordSpec, grammarAt, grammarMenuModel, grammarRanges, markedRanges, misspelledAt, misspelledRanges,
  parseSpellRanges, setSpellEffect, spellField, spellingMenuModel, withholdCaretWord,
} from "../src/spelling.ts";
import { createMockBridge } from "../src/bridge.ts";
import { computed as cascade, el as celem, parseRules } from "./cascade.mjs";
import { LIVE_OVERLAY_CSS, LIVE_ROOT_CLASS } from "../src/livetheme.ts";
import { mapThemeCss } from "../src/thememap.ts";

const DOC = "This is is a test. We recieve the mail.";
const IS_IS = { from: 5, to: 10 }; // "is is"
const RECIEVE = { from: 22, to: 29 };

const stateOf = (doc, anchor = 0, head = anchor) =>
  EditorState.create({
    doc,
    selection: EditorSelection.single(anchor, head),
    extensions: [markdown({ base: markdownLanguage }), spellField],
  });
const whole = (state) => ({ from: 0, to: state.doc.length });
const apply = (state, spec) => state.update(spec).state;
const text = (state, r) => state.doc.sliceString(r.from, r.to);
const sent = () => {
  const out = { check: [] };
  return { out, checkSpelling: (token, json) => out.check.push({ token, segments: JSON.parse(json) }) };
};
const grammar = (r, message = "Repeated word", suggestions = ["is"]) => ({ ...r, category: "Grammar", message, suggestions });
const spelling = (r) => ({ ...r, category: "Spelling" });
const reply = (...entries) => JSON.stringify(entries);

/** Ask, answer with `json`, apply; returns the new state. */
function checked(engine, bridge, state, json) {
  engine.request(state, whole(state));
  const token = bridge.out.check.at(-1).token;
  const spec = engine.reply(state, token, json);
  return spec ? apply(state, spec) : state;
}

test("a reply can carry spelling and grammar together; each becomes its own kind of mark", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  const s = checked(engine, bridge, stateOf(DOC, 0), reply(grammar(IS_IS, "Repeated word", ["is"]), spelling(RECIEVE)));
  assert.deepEqual(misspelledRanges(s).map((r) => text(s, r)), ["recieve"]);
  assert.deepEqual(grammarRanges(s), [{ ...IS_IS, message: "Repeated word", suggestions: ["is"] }]);
  assert.equal(text(s, IS_IS), "is is");
  assert.deepEqual(markedRanges(s).map((r) => [text(s, r), !!r.grammar]), [["is is", true], ["recieve", false]]);
});

test("parseSpellRanges: grammar entries keep message and suggestions, an entry without a category is spelling, unknown categories are not drawn", () => {
  const json = reply(
    { from: 0, to: 4 }, // old shape: spelling
    spelling({ from: 5, to: 8 }),
    { from: 10, to: 14, category: "Grammar", message: "Agreement", suggestions: ["a", "b", "a", "", 3, "c", "d", "e", "f"] },
    { from: 20, to: 25, category: "Grammar" }, // no message, no corrections: still a mark
    { from: 26, to: 30, category: "Style", message: "x" },
    { from: 31, to: 99, category: "Grammar", message: "past the end" },
    { from: 3, to: 3, category: "Grammar" },
    null,
  );
  const out = parseSpellRanges(json, 40);
  assert.deepEqual(out, [
    { from: 0, to: 4 },
    { from: 5, to: 8 },
    { from: 10, to: 14, grammar: { message: "Agreement", suggestions: ["a", "b", "c", "d", "e"] } },
    { from: 20, to: 25, grammar: { message: "", suggestions: [] } },
  ]);
  assert.deepEqual(parseSpellRanges('[{"from":1,"to":2,"category":"Grammar","suggestions":"oops","message":7}]', 5), [{ from: 1, to: 2, grammar: { message: "", suggestions: [] } }]);
});

test("grammar marks keep their message through edits elsewhere; an edit inside the range drops the mark until the next check", () => {
  let s = stateOf(DOC, DOC.length);
  s = apply(s, { effects: setSpellEffect.of({ ...whole(s), all: false, ranges: [{ ...IS_IS, grammar: { message: "Repeated word", suggestions: ["is"] } }, RECIEVE] }) });
  // Text typed before shifts both and the grammar mark still holds its message.
  s = apply(s, { changes: { from: 0, insert: ">> " }, userEvent: "input" });
  const [g] = grammarRanges(s);
  assert.equal(text(s, g), "is is");
  assert.deepEqual([g.message, g.suggestions], ["Repeated word", ["is"]]);
  // A space typed next to it does not unmark it.
  s = apply(s, { changes: { from: g.to, insert: " " }, userEvent: "input" });
  assert.equal(grammarRanges(s).length, 1);
  // A letter inside the range changes what the checker saw: the mark goes.
  s = apply(s, { changes: { from: g.from + 1, insert: "x" }, userEvent: "input" });
  assert.deepEqual(grammarRanges(s), []);
  assert.equal(misspelledRanges(s).length, 1, "the spelling mark elsewhere stays");
});

test("a later reply replaces both kinds in its window; an empty reply clears the grammar mark and leaves nothing behind", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  let s = checked(engine, bridge, stateOf(DOC, 0), reply(grammar(IS_IS), spelling(RECIEVE)));
  assert.equal(markedRanges(s).length, 2);
  s = checked(engine, bridge, s, reply(spelling(RECIEVE)));
  assert.deepEqual(grammarRanges(s), []);
  assert.equal(misspelledRanges(s).length, 1);
  s = checked(engine, bridge, s, "[]");
  assert.deepEqual(markedRanges(s), []);
});

test("a late grammar reply is mapped through the edits made since the request (message kept; touched ranges dropped)", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  let s = stateOf(DOC, DOC.length);
  engine.request(s, whole(s));
  const { token } = bridge.out.check[0];
  const tr = s.update({ changes: { from: 0, insert: "NEW " }, userEvent: "input" });
  engine.noteChanges(tr.changes);
  s = tr.state;
  s = apply(s, engine.reply(s, token, reply(grammar(IS_IS, "Late one"), spelling(RECIEVE))));
  const [g] = grammarRanges(s);
  assert.equal(text(s, g), "is is");
  assert.equal(g.message, "Late one");
  // touched by an edit in flight: dropped
  engine.request(s, whole(s));
  const t2 = bridge.out.check.at(-1).token;
  const tr2 = s.update({ changes: { from: 9, to: 10, insert: "S" }, userEvent: "input" }); // inside "is is" (shifted by 4)
  engine.noteChanges(tr2.changes);
  const spec = engine.reply(tr2.state, t2, reply(grammar({ from: g.from, to: g.to })));
  assert.deepEqual(grammarRanges(apply(tr2.state, spec)), []);
});

test("Ignore Grammar Issue: the range's text is remembered, the mark goes, later replies are filtered; spelling marks stay", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  let s = checked(engine, bridge, stateOf(DOC, 0), reply(grammar(IS_IS), spelling(RECIEVE)));
  s = apply(s, engine.ignoreGrammar(s, "is is"));
  assert.deepEqual(grammarRanges(s), []);
  assert.deepEqual(misspelledRanges(s).map((r) => text(s, r)), ["recieve"]);
  assert.ok(engine.ignoredGrammar.has("is is"));
  // The next check finds the same thing again: it is filtered, the spelling one still arrives.
  s = checked(engine, bridge, s, reply(grammar(IS_IS), spelling(RECIEVE)));
  assert.deepEqual(grammarRanges(s), []);
  assert.equal(misspelledRanges(s).length, 1);
  // A different grammar finding with other text is not ignored.
  s = checked(engine, bridge, s, reply(grammar({ from: 30, to: 38 }, "Other", ["x"])));
  assert.deepEqual(grammarRanges(s).map((g) => text(s, g)), ["the mail"]);
  // The same text elsewhere in the document is ignored too (the set is by text).
  const twice = stateOf("is is here and is is there", 0);
  const again = checked(engine, bridge, twice, reply(grammar({ from: 0, to: 5 }), grammar({ from: 15, to: 20 })));
  assert.deepEqual(grammarRanges(again), []);
  // A new document forgets the ignores.
  engine.clearIgnored();
  assert.equal(engine.ignoredGrammar.size, 0);
  s = checked(engine, bridge, stateOf(DOC, 0), reply(grammar(IS_IS)));
  assert.equal(grammarRanges(s).length, 1);
});

test("ignoring a grammar text keeps working across setSpellCheck(false)/true (only a new document clears it)", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  let s = checked(engine, bridge, stateOf(DOC, 0), reply(grammar(IS_IS)));
  s = apply(s, engine.ignoreGrammar(s, "is is"));
  s = apply(s, engine.setEnabled(false));
  assert.equal(engine.setEnabled(true), null);
  s = checked(engine, bridge, s, reply(grammar(IS_IS)));
  assert.deepEqual(grammarRanges(s), []);
});

test("Learn / Ignore Spelling drop spelling marks of that word but never grammar marks", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  let s = checked(engine, bridge, stateOf(DOC, 0), reply(grammar(IS_IS), spelling(RECIEVE)));
  s = apply(s, dropWordSpec(s, "recieve"));
  assert.deepEqual(misspelledRanges(s), []);
  assert.equal(grammarRanges(s).length, 1);
  assert.equal(grammarRanges(s)[0].message, "Repeated word", "the grammar mark survives with its message");
});

test("the caret rule covers grammar: a range the empty caret ends is withheld until the caret leaves, then marked", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  const doc = "This is is";
  let s = stateOf(doc, doc.length); // typing right behind "is is"
  s = checked(engine, bridge, s, reply(grammar({ from: 5, to: 10 })));
  assert.deepEqual(markedRanges(s), []);
  assert.equal(engine.withheldAt, doc.length);
  s = apply(s, { selection: EditorSelection.cursor(2) });
  s = checked(engine, bridge, s, reply(grammar({ from: 5, to: 10 })));
  assert.equal(grammarRanges(s).length, 1);
  assert.equal(engine.withheldAt, null);
  // withholdCaretWord is generic over the finding: ranges keep their payload.
  const g = { from: 5, to: 10, grammar: { message: "m", suggestions: [] } };
  assert.deepEqual(withholdCaretWord([g], { empty: true, head: 12 }).keep, [g]);
  assert.deepEqual(withholdCaretWord([g], { empty: true, head: 10 }), { keep: [], withheldAt: 10 });
  assert.deepEqual(withholdCaretWord([g], { empty: false, head: 10 }).keep, [g]);
});

test("both kinds can cover the same characters; lookups find each kind by position", () => {
  let s = stateOf("A very bad recieve here.", 0);
  const both = [{ from: 2, to: 18, grammar: { message: "Wording", suggestions: ["fine"] } }, { from: 11, to: 18 }];
  s = apply(s, { effects: setSpellEffect.of({ ...whole(s), all: true, ranges: both }) });
  assert.equal(markedRanges(s).length, 2);
  assert.deepEqual(misspelledAt(s, 14), { from: 11, to: 18 });
  assert.equal(grammarAt(s, 14).message, "Wording");
  assert.equal(grammarAt(s, 30), null);
  assert.equal(misspelledAt(s, 5), null, "the red word does not extend over the grammar range");
  // Two adjacent grammar ranges: a position strictly inside one beats an edge shared with the other.
  const adj = [{ from: 0, to: 5, grammar: { message: "one", suggestions: [] } }, { from: 5, to: 9, grammar: { message: "two", suggestions: [] } }];
  s = apply(s, { effects: setSpellEffect.of({ ...whole(s), all: true, ranges: adj }) });
  assert.equal(grammarAt(s, 3).message, "one");
  assert.equal(grammarAt(s, 7).message, "two");
});

test("a grammar correction is a user-event change input.grammar (reported, undoable) and is refused when the text moved", () => {
  const s = stateOf(DOC, 0);
  const spec = choiceSpec(s, IS_IS, "is is", { kind: "grammarSuggestion", word: "is" });
  const tr = s.update(spec);
  assert.equal(tr.state.doc.toString(), "This is a test. We recieve the mail.");
  assert.equal(tr.annotation(Transaction.userEvent), "input.grammar");
  assert.equal(tr.state.selection.main.head, 7, "caret behind the correction");
  assert.equal(choiceSpec(s, IS_IS, "was was", { kind: "grammarSuggestion", word: "was" }), null);
  assert.equal(choiceSpec(s, IS_IS, "is is", { kind: "ignoreGrammar" }), null, "ignoring changes no text");
  // spelling corrections keep their event
  assert.equal(s.update(choiceSpec(s, RECIEVE, "recieve", { kind: "suggestion", word: "receive" })).annotation(Transaction.userEvent), "input.spelling");
});

test("menu models: grammar = message line, up to five corrections, Ignore Grammar Issue; spelling unchanged", () => {
  const g = grammarMenuModel({ message: "Repeated word", suggestions: ["a", "b", "c", "d", "e", "f"] });
  assert.equal(g.header, "Repeated word");
  assert.deepEqual(g.corrections.map((c) => [c.text, c.choice.kind]), ["a", "b", "c", "d", "e"].map((w) => [w, "grammarSuggestion"]));
  assert.deepEqual(g.footer.map((f) => [f.text, f.choice.kind]), [["Ignore Grammar Issue", "ignoreGrammar"]]);
  assert.equal(g.none, undefined);
  assert.equal(grammarMenuModel({ message: "  ", suggestions: [] }).header, "Possible grammar issue");
  const sp = spellingMenuModel(["x"]);
  assert.equal(sp.header, undefined);
  assert.deepEqual(sp.footer.map((f) => f.text), ["Learn Spelling", "Ignore Spelling"]);
  assert.equal(sp.none, "No Guesses Found");
});

test("the mock bridge answers with both categories (doubled word = grammar)", async () => {
  const bridge = createMockBridge();
  const replies = [];
  bridge.spellingReply.connect((t, json) => replies.push(JSON.parse(json)));
  bridge.checkSpelling(1, JSON.stringify([{ from: 100, to: 138, text: DOC }]));
  await new Promise((r) => setTimeout(r, 80));
  assert.deepEqual(replies[0], [
    { from: 105, to: 110, category: "Grammar", message: "Repeated word", suggestions: ["is"] },
    { from: 122, to: 129, category: "Spelling" },
  ]);
});

// ---------------------------------------------------------------- CSS: the colours, and which underline wins on overlap
test("fw-grammar is a blue wavy underline; where spelling and grammar meet, one red line is shown (real cascade, filtered and exact Live)", () => {
  const css = readFileSync(new URL("../editor.css", import.meta.url), "utf8");
  assert.match(css, /\.fw-grammar \{[^}]*text-decoration: underline wavy #1a73e8;[^}]*text-decoration-skip-ink: none;/);
  const theme = "#write { text-decoration: none } p { text-decoration: none } a { text-decoration: none; color: red } body { text-decoration: none }";
  const mapped = mapThemeCss(theme);
  for (const exact of [false, true]) {
    const rules = [...parseRules(css), ...parseRules(theme, 10000), ...parseRules(mapped, 20000), ...(exact ? [] : parseRules(LIVE_OVERLAY_CSS, 30000))];
    const html = celem("html", { classes: [LIVE_ROOT_CLASS] });
    const write = celem("div", { id: "write", parent: celem("body", { parent: html }), classes: ["fw-mode-live"].concat(exact ? ["fw-live-exact"] : []) });
    const line = celem("div", { parent: write, classes: ["cm-line"] });
    const td = (e) => cascade("text-decoration", e, rules, { inherited: false });
    const grammarOnly = celem("span", { parent: line, classes: ["fw-grammar"] });
    const inLink = celem("span", { parent: celem("span", { parent: line, classes: ["fw-live-link"] }), classes: ["fw-grammar"] });
    assert.equal(td(grammarOnly), "underline wavy #1a73e8", "grammar alone is blue, exact=" + exact);
    assert.equal(td(inLink), "underline wavy #1a73e8", "grammar in a link, exact=" + exact);
    // the same element carries both classes: red
    assert.equal(td(celem("span", { parent: line, classes: ["fw-grammar", "fw-misspelled"] })), "underline wavy #d93025", "both on one element");
    assert.equal(td(celem("span", { parent: line, classes: ["fw-misspelled", "fw-grammar"] })), "underline wavy #d93025", "class order does not matter");
    // grammar nested in spelling: no second underline of its own
    const outer = celem("span", { parent: line, classes: ["fw-misspelled"] });
    assert.equal(td(outer), "underline wavy #d93025");
    assert.equal(td(celem("span", { parent: outer, classes: ["fw-grammar"] })), "none", "grammar inside spelling adds no line");
    // spelling nested in grammar: both keep their own colour; the grammar line sits lower (checked in the stylesheet text)
    const gOuter = celem("span", { parent: line, classes: ["fw-grammar"] });
    assert.equal(td(celem("span", { parent: gOuter, classes: ["fw-misspelled"] })), "underline wavy #d93025");
    assert.equal(cascade("text-underline-offset", gOuter, rules, { inherited: false }), "0.2em");
    assert.equal(cascade("text-underline-offset", celem("span", { parent: line, classes: ["fw-misspelled"] }), rules, { inherited: false }) ?? "", "", "spelling keeps the default offset");
  }
});

test("the host slot for Ignore Grammar Issue exists on the mock bridge and the engine drops the mark", () => {
  const bridge = sent();
  const engine = new SpellEngine(bridge);
  let s = checked(engine, bridge, stateOf(DOC, 0), reply(grammar(IS_IS)));
  s = apply(s, engine.ignoreGrammar(s, "is is"));
  assert.deepEqual(grammarRanges(s), []);
  const mock = createMockBridge();
  assert.equal(typeof mock.ignoreGrammar, "function", "the mock host accepts ignoreGrammar(text, message)");
  mock.ignoreGrammar("is is", "Repeated word");
  assert.deepEqual(mock.calls.filter((c) => c.name === "ignoreGrammar").map((c) => c.args), [["is is", "Repeated word"]]);
});
