import test from "node:test";
import assert from "node:assert/strict";
import { EditorState } from "@codemirror/state";
import { Session } from "../src/modes.ts";
import { marksFor, showMarksEffect, showMarksField, marksExtension, parseShowMarks } from "../src/marks.ts";

const collect = (set) => {
  const out = [];
  set.between(0, 1e9, (from, to, v) => { out.push({ from, to, cls: v.spec.class ?? (v.spec.widget ? "eol" : "?") }); });
  return out;
};
const whole = (state) => [{ from: 0, to: state.doc.length }];

test("spaces, tabs and every line end (empty lines too) get marks", () => {
  const state = EditorState.create({ doc: "a b\n\n\tc  \nz" });
  const m = collect(marksFor({ state }, whole(state)));
  assert.deepEqual(m.filter((x) => x.cls === "fw-ws-space").map((x) => x.from), [1, 7, 8]);
  assert.deepEqual(m.filter((x) => x.cls === "fw-ws-tab").map((x) => x.from), [5]);
  assert.deepEqual(m.filter((x) => x.cls === "eol").map((x) => x.from), [3, 4, 9, 11]);
});

test("only the given (visible) ranges are decorated", () => {
  const state = EditorState.create({ doc: "a b\nc d\ne f" });
  const m = collect(marksFor({ state }, [{ from: 4, to: 7 }]));
  assert.deepEqual(m.map((x) => x.from), [5, 7]);
});

test("parseShowMarks accepts only booleans", () => {
  assert.equal(parseShowMarks({ showMarks: true }), true);
  assert.equal(parseShowMarks({ showMarks: false }), false);
  assert.equal(parseShowMarks({}), undefined);
  assert.equal(parseShowMarks({ showMarks: "yes" }), undefined);
  assert.equal(parseShowMarks(null), undefined);
});

test("the effect toggles the field without touching the document", () => {
  const state = EditorState.create({ doc: "a b", extensions: [marksExtension()] });
  assert.equal(state.field(showMarksField), false);
  const tr = state.update({ effects: showMarksEffect.of(true) });
  assert.equal(tr.docChanged, false);
  assert.equal(tr.state.field(showMarksField), true);
  assert.equal(tr.state.doc.toString(), "a b");
});

for (const mode of ["live", "source"]) {
  test(`Session ${mode}: showMarks toggles, survives setDocument, never reaches the host`, () => {
    const session = new Session();
    const view = {
      state: session.createState("a b\n"),
      dispatch(...specs) { this.state = this.state.update(...specs).state; },
      setState(s) { this.state = s; },
    };
    session.attach(view);
    const sent = [];
    session.onDocChanged = (j, r) => sent.push([j, r]);
    session.setMode(mode);
    session.setAppearance({ showMarks: true });
    assert.equal(view.state.field(showMarksField), true);
    session.setAppearance({ fontSize: 15 }); // absent key leaves it alone
    assert.equal(view.state.field(showMarksField), true);
    session.setDocument("x y", 3);
    assert.equal(view.state.field(showMarksField), true);
    session.setAppearance({ showMarks: false });
    assert.equal(view.state.field(showMarksField), false);
    assert.equal(view.state.doc.toString(), "x y");
    assert.equal(session.revision, 3);
    assert.deepEqual(sent, []);
  });
}
