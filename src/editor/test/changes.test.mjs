import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, Transaction } from "@codemirror/state";
import { changesToJson, applyWireChanges, parseChanges } from "../src/changes.ts";
import { Session } from "../src/modes.ts";

// Plain JS reference implementation, deliberately independent of src/changes.ts.
function referenceApply(text, json) {
  const list = JSON.parse(json);
  for (let i = 1; i < list.length; i++) assert.ok(list[i].from >= list[i - 1].to, "ascending, non-overlapping");
  let out = text;
  for (let i = list.length - 1; i >= 0; i--) out = out.slice(0, list[i].from) + list[i].insert + out.slice(list[i].to);
  return out;
}

const TEXTS = {
  ascii: "alpha beta gamma\ndelta epsilon\nzeta eta theta\n",
  emoji: "pre 👨‍👩‍👧‍👦 family 🏳️‍🌈 flag\nsecond 😀 line\n",
  combining: "café naïve résumé\nsecond\n",
  cjk: "你好，世界\nこんにちは世界\n한국어 텍스트\n",
};

for (const [name, text] of Object.entries(TEXTS)) {
  test(`changesToJson round-trips multi-change transaction (${name})`, () => {
    const state = EditorState.create({ doc: text });
    const n = text.length;
    const tr = state.update({
      changes: [
        { from: 0, insert: "👩‍💻 inserted́ " }, // pure insert at start
        { from: 3, to: Math.min(7, n) }, // pure delete
        { from: Math.floor(n / 2), to: Math.floor(n / 2) + 2, insert: "世界🌍" }, // replace
        { from: n, insert: "\ntail 🎉" }, // insert at end
      ],
    });
    const json = changesToJson(tr.changes);
    assert.equal(referenceApply(text, json), tr.state.doc.toString());
    assert.equal(applyWireChanges(text, JSON.parse(json)), tr.state.doc.toString());
  });
}

test("inserts spanning newlines serialize with LF", () => {
  const state = EditorState.create({ doc: "a\nb\nc" });
  const tr = state.update({ changes: { from: 1, to: 4, insert: "X\nY\nZ" } });
  const json = changesToJson(tr.changes);
  assert.deepEqual(JSON.parse(json), [{ from: 1, to: 4, insert: "X\nY\nZ" }]);
  assert.equal(referenceApply("a\nb\nc", json), tr.state.doc.toString());
});

test("parseChanges validates", () => {
  assert.throws(() => parseChanges('[{"from":5,"to":2,"insert":""}]', 10));
  assert.throws(() => parseChanges('[{"from":0,"to":20,"insert":""}]', 10));
  assert.throws(() => parseChanges('[{"from":4,"to":6,"insert":""},{"from":2,"to":3,"insert":""}]', 10));
  assert.throws(() => parseChanges("{}", 10));
});

// ---- Session.simulateUserChanges / undo / redo with a DOM-free fake view
function fakeView(session, doc) {
  const view = {
    state: session.createState(doc),
    dispatch(...specs) {
      const tr = specs[0] instanceof Transaction ? specs[0] : this.state.update(...specs);
      this.state = tr.state;
      session.handleTransactions([tr]);
    },
    setState(s) {
      this.state = s;
    },
  };
  session.attach(view);
  return view;
}

test("simulateUserChanges is a user transaction reported through documentChanged", () => {
  const session = new Session();
  const text = "héllo 👨‍👩‍👧 wörld\nsecond line\n";
  const view = fakeView(session, text);
  session.setDocument(text, 10);
  const reports = [];
  session.onDocChanged = (json, rev) => reports.push({ json, rev });

  const incoming = JSON.stringify([
    { from: 0, to: 1, insert: "H" },
    { from: 6, to: 8, insert: "🌍" },
    { from: text.length, to: text.length, insert: "tail\n" },
  ]);
  session.simulateUserChanges(incoming);

  assert.equal(reports.length, 1);
  assert.equal(reports[0].rev, 11);
  assert.equal(session.revision, 11);
  // The serialized report is the same edit, and applies to the old text to give the new text.
  assert.equal(referenceApply(text, reports[0].json), view.state.doc.toString());
  assert.equal(referenceApply(text, incoming), view.state.doc.toString());
  assert.deepEqual(JSON.parse(reports[0].json), JSON.parse(incoming));
});

test("applyChanges is not echoed; undo/redo are reported like user edits", () => {
  const session = new Session();
  const view = fakeView(session, "");
  session.setDocument("abc", 0);
  const reports = [];
  session.onDocChanged = (json, rev) => reports.push({ json, rev });

  session.applyChanges('[{"from":3,"to":3,"insert":"!"}]', 5);
  assert.equal(reports.length, 0);
  assert.equal(session.revision, 5);
  assert.equal(session.getText(), "abc!");

  session.simulateUserChanges('[{"from":0,"to":0,"insert":"X"}]');
  assert.equal(session.getText(), "Xabc!");
  assert.equal(reports.length, 1);
  assert.equal(reports[0].rev, 6);

  assert.ok(session.undo());
  assert.equal(session.getText(), "abc!");
  assert.equal(reports.length, 2);
  assert.equal(reports[1].rev, 7);
  assert.equal(referenceApply("Xabc!", reports[1].json), "abc!");

  assert.ok(session.redo());
  assert.equal(session.getText(), "Xabc!");
  assert.equal(reports.length, 3);
  assert.equal(reports[2].rev, 8);
  void view;
});
