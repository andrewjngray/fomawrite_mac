import test from "node:test";
import assert from "node:assert/strict";
import { EditorSelection, EditorState, Transaction } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { mathMarkdown } from "../src/math.ts";
import { extrasMarkdown } from "../src/extras.ts";
import {
  SpellEngine, choiceSpec, dropWordSpec, misspelledAt, misspelledRanges, parseSpellRanges, parseSuggestions, proseSegments,
  setSpellEffect, spellField, spellingExtension, withholdCaretWord,
} from "../src/spelling.ts";
import { createMockBridge } from "../src/bridge.ts";

const stateOf = (doc, anchor = 0, head = anchor) =>
  EditorState.create({
    doc,
    selection: EditorSelection.single(anchor, head),
    extensions: [markdown({ base: markdownLanguage, extensions: [mathMarkdown, extrasMarkdown] }), spellField],
  });

/** A bridge that records what the page sends to the host. */
function fakeBridge() {
  const sent = { check: [], suggest: [], learned: [], ignored: [], logs: [] };
  return {
    sent,
    checkSpelling: (token, json) => sent.check.push({ token, segments: JSON.parse(json) }),
    spellingSuggestions: (token, word) => sent.suggest.push({ token, word }),
    learnWord: (w) => sent.learned.push(w),
    ignoreWord: (w) => sent.ignored.push(w),
    log: (m) => sent.logs.push(m),
  };
}
const whole = (state) => ({ from: 0, to: state.doc.length });
const apply = (state, spec) => state.update(spec).state;
const marked = (state) => misspelledRanges(state).map((r) => state.doc.sliceString(r.from, r.to));
/** Ranges of every occurrence of `word` in `doc`, as the host would report them. */
const occurrences = (doc, word) => [...doc.matchAll(new RegExp(`\\b${word}\\b`, "g"))].map((m) => ({ from: m.index, to: m.index + word.length }));

// ---------------------------------------------------------------- segment extraction
test("proseSegments: code, URLs, HTML, front matter and math are not sent; every offset still maps to the document", () => {
  const doc = [
    "---",
    "frontkey: frontmatter wrods",
    "---",
    "",
    "Plain prose with **bold** and _emphasised_ wrods here.",
    "",
    "Use `inlinecode wrods` and [a link label](https://exampel.com/pathh \"a titel\") plus <https://autolnk.org/wrods>.",
    "",
    "Bare https://barelnk.com/wrods and <span class=\"clss\">html text</span> and $x_wrods$ math.",
    "",
    "```js",
    "const fencd = wrods;",
    "```",
    "",
    "$$",
    "mathblok wrods",
    "$$",
    "",
    "[ref]: https://refurl.com/wrods",
    "",
    "Footnote[^zzfn] ref and a last lne.",
  ].join("\n");
  const state = stateOf(doc);
  const segs = proseSegments(state, 0, doc.length);
  const sent = segs.map((s) => s.text).join("\n");
  for (const hidden of ["frontkey", "frontmatter", "inlinecode", "exampel", "pathh", "autolnk", "barelnk", "clss", "fencd", "mathblok", "refurl", "zzfn"]) {
    assert.ok(!sent.includes(hidden), `${hidden} must not reach the checker`);
  }
  // What is prose does reach it: plain text, emphasis words without their markers, link label, link title, text between tags.
  for (const shown of ["Plain prose with", "bold", "emphasised", "a link label", "a titel", "html text", "last lne", "Bare", "math."])
    assert.ok(sent.includes(shown), `${shown} must reach the checker`);
  assert.ok(!sent.includes("**") && !sent.includes("_emphasised_"), "Markdown syntax characters are blanked");
  for (const s of segs) {
    assert.equal(s.text.length, s.to - s.from, "text keeps the document length");
    for (let i = 0; i < s.text.length; i++) if (s.text[i] !== " " && s.text[i] !== "\n") assert.equal(s.text[i], doc[s.from + i], "char " + i);
  }
});

test("proseSegments: consecutive prose lines are one segment, a blank or code-only line splits, ranges are line aligned", () => {
  const doc = "first line wrods\nsecond line\n\nthird\n```\ncode only\n```\nfourth";
  const segs = proseSegments(stateOf(doc), 5, 20); // from inside the first line to inside the second: extended to whole lines
  assert.equal(segs.length, 1);
  assert.deepEqual([segs[0].from, segs[0].to], [0, "first line wrods\nsecond line".length]);
  const all = proseSegments(stateOf(doc), 0, doc.length);
  assert.deepEqual(all.map((s) => s.text.trim()), ["first line wrods\nsecond line", "third", "fourth"]);
  assert.deepEqual(proseSegments(stateOf(""), 0, 0), []);
  assert.deepEqual(proseSegments(stateOf("12345 `code`"), 0, 12), []); // no letters at all
});

// ---------------------------------------------------------------- marks and edits
test("replies become fw-misspelled marks; marks map through edits; an edit that changes the word drops its mark", () => {
  const doc = "Their going too the shop tomorow and teh end.\n";
  let s = stateOf(doc, doc.length);
  s = apply(s, { effects: setSpellEffect.of({ from: 0, to: doc.length, ranges: [...occurrences(doc, "tomorow"), ...occurrences(doc, "teh")], all: false }) });
  assert.deepEqual(marked(s), ["tomorow", "teh"]);
  // Text typed before the words shifts the marks and keeps them on the same words.
  s = apply(s, { changes: { from: 0, insert: ">> intro " }, userEvent: "input" });
  assert.deepEqual(marked(s), ["tomorow", "teh"]);
  const start = s.doc.toString().indexOf("tomorow");
  assert.deepEqual(misspelledAt(s, start + 3), { from: start, to: start + 7 });
  // A space typed next to a marked word does not unmark it.
  s = apply(s, { changes: { from: start, insert: " " }, userEvent: "input" });
  assert.deepEqual(marked(s), ["tomorow", "teh"]);
  // A letter inside it (tomorow -> tomorrow) changes the word: the stale mark goes until the next check.
  const at = s.doc.toString().indexOf("tomorow") + 6;
  s = apply(s, { changes: { from: at, insert: "r" }, userEvent: "input" });
  assert.deepEqual(marked(s), ["teh"]);
  // A letter typed at the end of a marked word extends the word: mark goes too. Deleting the word removes it.
  const teh = s.doc.toString().indexOf("teh");
  s = apply(s, { changes: { from: teh + 3, insert: "m" }, userEvent: "input" });
  assert.deepEqual(marked(s), []);
});

test("a reply replaces only the marks inside its window; `all` replaces everything; an empty reply clears", () => {
  const doc = "tomorow one\n\nsecond teh two\n";
  let s = stateOf(doc);
  s = apply(s, { effects: setSpellEffect.of({ from: 0, to: doc.length, ranges: [...occurrences(doc, "tomorow"), ...occurrences(doc, "teh")], all: false }) });
  assert.deepEqual(marked(s), ["tomorow", "teh"]);
  s = apply(s, { effects: setSpellEffect.of({ from: 0, to: 11, ranges: [], all: false }) }); // first paragraph is now fine
  assert.deepEqual(marked(s), ["teh"]);
  s = apply(s, { effects: setSpellEffect.of({ from: 0, to: 11, ranges: occurrences(doc, "tomorow"), all: true }) });
  assert.deepEqual(marked(s), ["tomorow"]);
  assert.deepEqual(marked(apply(s, dropWordSpec(s, "tomorow"))), []);
  assert.deepEqual(marked(apply(s, dropWordSpec(s, "other"))), ["tomorow"]);
});

test("parseSpellRanges / parseSuggestions drop anything malformed and never throw", () => {
  assert.deepEqual(parseSpellRanges('[{"from":4,"to":9},{"from":0,"to":2}]', 20), [{ from: 0, to: 2 }, { from: 4, to: 9 }]);
  assert.deepEqual(parseSpellRanges('[{"from":4,"to":99},{"from":-1,"to":2},{"from":3,"to":3},{"from":"1","to":2},null,5]', 20), []);
  assert.deepEqual(parseSpellRanges("not json", 20), []);
  assert.deepEqual(parseSpellRanges('{"from":1,"to":2}', 20), []);
  assert.deepEqual(parseSuggestions('["a","b","a","","  ",3,"c","d","e","f","g"]'), ["a", "b", "c", "d", "e"]);
  assert.deepEqual(parseSuggestions("nope"), []);
});

// ---------------------------------------------------------------- the request / reply flow
test("request sends the prose segments with a token; the reply marks the words", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  const doc = "Their going too the shop tomorow.\n\n```\nnocode wrods\n```\n";
  let s = stateOf(doc);
  assert.equal(engine.request(s, whole(s)), null);
  assert.equal(bridge.sent.check.length, 1);
  const { token, segments } = bridge.sent.check[0];
  assert.equal(segments.length, 1);
  assert.deepEqual(Object.keys(segments[0]).sort(), ["from", "text", "to"]);
  assert.equal(segments[0].text, "Their going too the shop tomorow.");
  s = apply(s, engine.reply(s, token, JSON.stringify(occurrences(doc, "tomorow"))));
  assert.deepEqual(marked(s), ["tomorow"]);
});

test("a window with no prose is answered locally: its marks are cleared and nothing is sent", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  let s = stateOf("```\nwrods\n```\n");
  s = apply(s, { effects: setSpellEffect.of({ from: 0, to: 4, ranges: [{ from: 4, to: 9 }], all: false }) });
  assert.deepEqual(marked(s), ["wrods"]);
  const spec = engine.request(s, whole(s));
  assert.equal(bridge.sent.check.length, 0);
  assert.deepEqual(marked(apply(s, spec)), []);
});

test("a reply that arrives after edits is mapped through them: marks stay on their words, touched ones are dropped", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  const doc = "alpha tomorow beta teh gamma wrold\n";
  let s = stateOf(doc, doc.length);
  engine.request(s, whole(s));
  const { token } = bridge.sent.check[0];
  const edit = (spec) => { const tr = s.update(spec); engine.noteChanges(tr.changes); s = tr.state; };
  edit({ changes: { from: 0, insert: "NEW LEAD " }, userEvent: "input" }); // before every word
  edit({ changes: { from: s.doc.toString().indexOf("teh") + 1, to: s.doc.toString().indexOf("teh") + 2, insert: "E" }, userEvent: "input" }); // inside "teh"
  const reply = engine.reply(s, token, JSON.stringify([...occurrences(doc, "tomorow"), ...occurrences(doc, "teh"), ...occurrences(doc, "wrold")]));
  assert.ok(reply, "a late reply is still applied");
  s = apply(s, reply);
  assert.deepEqual(marked(s), ["tomorow", "wrold"], "tomorow and wrold shifted; teh was edited so it is not marked");
});

test("a reply for a document that shrank or a window that vanished is dropped, never throws", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  let s = stateOf("tomorow\n");
  engine.request(s, whole(s));
  const tr = s.update({ changes: { from: 0, to: s.doc.length, insert: "" } });
  engine.noteChanges(tr.changes);
  assert.equal(engine.reply(tr.state, bridge.sent.check[0].token, '[{"from":0,"to":7}]'), null);
});

test("stale tokens are ignored: an older request's reply, an unknown token, a replayed reply", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  const doc = "tomorow teh\n";
  const s = stateOf(doc);
  engine.request(s, whole(s));
  engine.request(s, whole(s));
  const [first, second] = bridge.sent.check.map((c) => c.token);
  assert.notEqual(first, second);
  assert.equal(engine.reply(s, first, JSON.stringify(occurrences(doc, "tomorow"))), null, "superseded by a newer request");
  assert.equal(engine.reply(s, 9999, JSON.stringify(occurrences(doc, "tomorow"))), null, "never issued");
  const good = engine.reply(s, second, JSON.stringify(occurrences(doc, "teh")));
  assert.deepEqual(marked(apply(s, good)), ["teh"]);
  assert.equal(engine.reply(s, second, JSON.stringify(occurrences(doc, "teh"))), null, "a reply applies once");
  // reset (document replaced) forgets the request in flight
  engine.request(s, whole(s));
  engine.reset();
  assert.equal(engine.reply(s, bridge.sent.check[2].token, "[]"), null);
});

test("ranges outside the window that was asked, or past the document, are ignored", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  const doc = "aaa wrods\n\nbbb tomorow\n";
  const s = stateOf(doc);
  engine.request(s, { from: 0, to: 9 });
  const reply = engine.reply(s, bridge.sent.check[0].token, JSON.stringify([{ from: 4, to: 9 }, { from: 15, to: 22 }, { from: 15, to: 999 }]));
  assert.deepEqual(marked(apply(s, reply)), ["wrods"]);
});

// ---------------------------------------------------------------- the caret word
test("withholdCaretWord: a word the empty caret ends is held back, anything else is kept", () => {
  const ranges = [{ from: 0, to: 4 }, { from: 10, to: 15 }];
  assert.deepEqual(withholdCaretWord(ranges, { empty: true, head: 15 }), { keep: [{ from: 0, to: 4 }], withheldAt: 15 });
  assert.deepEqual(withholdCaretWord(ranges, { empty: true, head: 12 }), { keep: ranges, withheldAt: null }, "caret inside is not withheld");
  assert.deepEqual(withholdCaretWord(ranges, { empty: true, head: 16 }), { keep: ranges, withheldAt: null }, "caret after a space");
  assert.deepEqual(withholdCaretWord(ranges, { empty: false, head: 15 }), { keep: ranges, withheldAt: null }, "a selection is not typing");
});

test("the word being typed is not underlined until the caret leaves it", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  const doc = "good tomorow";
  let s = stateOf(doc, doc.length); // caret right behind the half-typed word, nothing after it
  engine.request(s, whole(s));
  s = apply(s, engine.reply(s, bridge.sent.check[0].token, JSON.stringify(occurrences(doc, "tomorow"))));
  assert.deepEqual(marked(s), [], "withheld");
  assert.equal(engine.withheldAt, doc.length, "remembered, so the plugin re-checks when the caret moves");
  // The caret moves off; the plugin schedules a check; this time the word is marked.
  s = apply(s, { selection: EditorSelection.cursor(2) });
  engine.request(s, whole(s));
  s = apply(s, engine.reply(s, bridge.sent.check[1].token, JSON.stringify(occurrences(doc, "tomorow"))));
  assert.deepEqual(marked(s), ["tomorow"]);
  assert.equal(engine.withheldAt, null);
});

// ---------------------------------------------------------------- setSpellCheck
test("setSpellCheck(false) clears every mark and stops requesting; true checks again", () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  const doc = "tomorow here\n";
  let s = stateOf(doc);
  engine.request(s, whole(s));
  s = apply(s, engine.reply(s, bridge.sent.check[0].token, JSON.stringify(occurrences(doc, "tomorow"))));
  assert.deepEqual(marked(s), ["tomorow"]);
  engine.request(s, whole(s)); // a request in flight when the setting is switched off
  s = apply(s, engine.setEnabled(false));
  assert.deepEqual(marked(s), []);
  assert.equal(engine.active, false);
  const before = bridge.sent.check.length;
  assert.equal(engine.request(s, whole(s)), null);
  assert.equal(bridge.sent.check.length, before, "nothing is sent while off");
  assert.equal(engine.reply(s, bridge.sent.check[before - 1].token, JSON.stringify(occurrences(doc, "tomorow"))), null, "a late reply after switching off is dropped");
  assert.equal(engine.setEnabled(true), null);
  assert.equal(engine.active, true);
  engine.request(s, whole(s), true);
  assert.equal(bridge.sent.check.length, before + 1);
});

test("a host that has no checkSpelling slot is never asked", () => {
  const engine = new SpellEngine({});
  const s = stateOf("tomorow\n");
  assert.equal(engine.active, false);
  assert.equal(engine.request(s, whole(s)), null);
});

// ---------------------------------------------------------------- suggestions and the menu's choices
test("suggestions: only the latest request's reply counts; late and unknown replies are ignored; a silent host times out to []", async () => {
  const bridge = fakeBridge();
  const engine = new SpellEngine(bridge);
  const got = [];
  const t1 = engine.requestSuggestions("tomorow", (w) => got.push(["one", w]), 1000);
  const t2 = engine.requestSuggestions("teh", (w) => got.push(["two", w]), 1000);
  assert.deepEqual(bridge.sent.suggest, [{ token: t1, word: "tomorow" }, { token: t2, word: "teh" }]);
  assert.equal(engine.suggestionsReply(t1, '["tomorrow"]'), false);
  assert.equal(engine.suggestionsReply(12345, '["x"]'), false);
  assert.equal(engine.suggestionsReply(t2, '["the","tea","ten","tee","tech","tel"]'), true);
  assert.deepEqual(got, [["two", ["the", "tea", "ten", "tee", "tech"]]]);
  assert.equal(engine.suggestionsReply(t2, '["again"]'), false);
  const silent = await new Promise((resolve) => {
    const t = engine.requestSuggestions("wrods", resolve, 20);
    setTimeout(() => assert.equal(engine.suggestionsReply(t, '["late"]'), false, "late reply after the timeout"), 60);
  });
  assert.deepEqual(silent, []);
  await new Promise((r) => setTimeout(r, 80));
  // a host without the slot answers [] at once
  let none = null;
  new SpellEngine({ checkSpelling() {} }).requestSuggestions("x", (w) => (none = w));
  assert.deepEqual(none, []);
});

test("choosing a suggestion is a user-event change (reported to the host, undoable); Learn / Ignore change no text", () => {
  const doc = "Their going too the shop tomorow.\n";
  const s = stateOf(doc);
  const range = { from: doc.indexOf("tomorow"), to: doc.indexOf("tomorow") + 7 };
  const spec = choiceSpec(s, range, "tomorow", { kind: "suggestion", word: "tomorrow" });
  const tr = s.update(spec);
  assert.equal(tr.state.doc.toString(), "Their going too the shop tomorrow.\n");
  assert.equal(tr.annotation(Transaction.userEvent), "input.spelling");
  assert.equal(tr.state.selection.main.head, range.from + "tomorrow".length, "caret lands behind the new word");
  assert.deepEqual(tr.changes.toJSON(), [range.from, [7, "tomorrow"], doc.length - range.to]);
  assert.equal(choiceSpec(s, range, "other", { kind: "suggestion", word: "x" }), null, "the document no longer holds that word there");
  assert.equal(choiceSpec(s, range, "tomorow", { kind: "learn" }), null);
  assert.equal(choiceSpec(s, range, "tomorow", { kind: "ignore" }), null);
});

// ---------------------------------------------------------------- the mock bridge and the extension
test("the mock bridge answers checkSpelling / spellingSuggestions so the page runs without Qt", async () => {
  const bridge = createMockBridge();
  const replies = [];
  bridge.spellingReply.connect((t, json) => replies.push(["ranges", t, JSON.parse(json)]));
  bridge.suggestionsReply.connect((t, json) => replies.push(["words", t, JSON.parse(json)]));
  bridge.checkSpelling(7, JSON.stringify([{ from: 10, to: 30, text: "Their going tomorow ok " }]));
  bridge.spellingSuggestions(8, "tomorow");
  bridge.learnWord("zzz");
  bridge.ignoreWord("yyy");
  await new Promise((r) => setTimeout(r, 80));
  assert.deepEqual(replies, [["ranges", 7, [{ from: 22, to: 29 }]], ["words", 8, ["tomorrow"]]]);
  assert.deepEqual(bridge.calls.filter((c) => /^(learn|ignore)Word$/.test(c.name)).map((c) => [c.name, c.args[0]]), [["learnWord", "zzz"], ["ignoreWord", "yyy"]]);
});

test("spellingExtension connects the three host signals and installs the mark field", () => {
  const bridge = createMockBridge();
  const connected = [];
  for (const name of ["spellingReply", "suggestionsReply", "setSpellCheck"]) {
    const original = bridge[name].connect;
    bridge[name].connect = (fn) => { connected.push(name); original(fn); };
  }
  const state = EditorState.create({ doc: "x", extensions: [spellingExtension(bridge)] });
  assert.deepEqual(connected.sort(), ["setSpellCheck", "spellingReply", "suggestionsReply"]);
  assert.ok(state.field(spellField));
  // Signals fired before any view exists are harmless.
  bridge.emit("setSpellCheck", false);
  bridge.emit("spellingReply", 1, "[]");
  bridge.emit("suggestionsReply", 1, "[]");
});

// ---------------------------------------------------------------- the underline survives the Live theme overlay (real cascade)
import { readFileSync } from "node:fs";
import { computed as cascade, el as celem, parseRules } from "./cascade.mjs";
import { LIVE_OVERLAY_CSS, LIVE_ROOT_CLASS } from "../src/livetheme.ts";
import { mapThemeCss } from "../src/thememap.ts";

test("fw-misspelled keeps its red wavy underline in filtered and exact Live, whatever the theme says about text-decoration", () => {
  const css = readFileSync(new URL("../editor.css", import.meta.url), "utf8");
  assert.match(css, /\.fw-misspelled \{[^}]*text-decoration: underline wavy #d93025;[^}]*text-decoration-skip-ink: none;/);
  const theme = "#write { text-decoration: none } p { text-decoration: none } a { text-decoration: none; color: red } strong { text-decoration: none } body { text-decoration: none }";
  const mapped = mapThemeCss(theme);
  for (const exact of [false, true]) {
    const rules = [...parseRules(css), ...parseRules(theme, 10000), ...parseRules(mapped, 20000), ...(exact ? [] : parseRules(LIVE_OVERLAY_CSS, 30000))];
    const html = celem("html", { classes: [LIVE_ROOT_CLASS] });
    const write = celem("div", { id: "write", parent: celem("body", { parent: html }), classes: ["fw-mode-live"].concat(exact ? ["fw-live-exact"] : []) });
    const line = celem("div", { parent: write, classes: ["cm-line"] });
    const plain = celem("span", { parent: line, classes: ["fw-misspelled"] });
    const inLink = celem("span", { parent: celem("span", { parent: line, classes: ["fw-live-link"] }), classes: ["fw-misspelled"] });
    for (const span of [plain, inLink]) assert.equal(cascade("text-decoration", span, rules, { inherited: false }), "underline wavy #d93025", "exact=" + exact);
  }
});
