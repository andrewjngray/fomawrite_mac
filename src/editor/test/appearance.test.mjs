import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, Text } from "@codemirror/state";
import { EditorView } from "@codemirror/view";
import { indentUnit } from "@codemirror/language";
import {
  INITIAL_APPEARANCE, appearanceClasses, appearanceEffect, appearanceExtension, focusActiveLines, getAppearanceState,
  guideLevel, hangForLine, indentLevel, paragraphRange, parseAppearancePatch, planFor, reduceAppearance, shouldRecenter,
} from "../src/appearance.ts";
import { Session } from "../src/modes.ts";

// ---------------------------------------------------------------- (a) JSON -> classes / compartment plan
function apply(prev, json) {
  return reduceAppearance(prev, parseAppearancePatch(JSON.parse(json)));
}

test("appearance name maps to the fw-appearance-<name> class", () => {
  for (const name of ["manuscript", "editorial", "book", "code"]) {
    const s = apply(INITIAL_APPEARANCE, JSON.stringify({ appearance: name }));
    assert.deepEqual(appearanceClasses(s), [`fw-appearance-${name}`]);
  }
  assert.deepEqual(appearanceClasses(apply(INITIAL_APPEARANCE, '{"appearance":"Code"}')), ["fw-appearance-code"]);
});

test("focus and typewriter add their classes; missing keys leave the previous value", () => {
  let s = apply(INITIAL_APPEARANCE, '{"appearance":"book","focus":true,"typewriter":true}');
  assert.deepEqual(appearanceClasses(s), ["fw-appearance-book", "fw-focus", "fw-typewriter"]);
  s = apply(s, '{"fontSize":20,"dark":true}'); // unrelated keys
  assert.deepEqual(appearanceClasses(s), ["fw-appearance-book", "fw-focus", "fw-typewriter"]);
  s = apply(s, '{"focus":false}');
  assert.deepEqual(appearanceClasses(s), ["fw-appearance-book", "fw-typewriter"]);
  s = apply(s, '{"appearance":"editorial","typewriter":false}');
  assert.deepEqual(appearanceClasses(s), ["fw-appearance-editorial"]);
});

test("unknown keys and invalid appearance names are ignored; no-op patches keep identity", () => {
  assert.deepEqual(parseAppearancePatch({ appearance: "neon", bogus: 1 }), {});
  assert.deepEqual(parseAppearancePatch({ appearance: 3 }), {});
  assert.deepEqual(parseAppearancePatch(null), {});
  assert.deepEqual(parseAppearancePatch("code"), {});
  const s = apply(INITIAL_APPEARANCE, '{"appearance":"code"}');
  assert.equal(reduceAppearance(s, parseAppearancePatch({ appearance: "code" })), s);
  assert.equal(reduceAppearance(s, {}), s);
  assert.equal(apply(s, '{"appearance":"neon"}'), s);
});

test("compartment plan per appearance (source mode)", () => {
  const plan = (name, extra = {}) => planFor(reduceAppearance(INITIAL_APPEARANCE, { appearance: name, ...extra }));
  assert.deepEqual(planFor(INITIAL_APPEARANCE), { wrap: true, code: false, hanging: false, focus: false });
  assert.deepEqual(plan("manuscript"), { wrap: true, code: false, hanging: true, focus: false });
  assert.deepEqual(plan("editorial"), { wrap: true, code: false, hanging: false, focus: false });
  assert.deepEqual(plan("book"), { wrap: true, code: false, hanging: false, focus: false });
  assert.deepEqual(plan("code"), { wrap: false, code: true, hanging: false, focus: false });
  assert.deepEqual(plan("code", { focus: true }), { wrap: false, code: true, hanging: false, focus: true });
  // live mode keeps its own look: no code/hanging extras, wrapping stays on
  assert.deepEqual(plan("code", { mode: "live" }), { wrap: true, code: false, hanging: false, focus: false });
  assert.deepEqual(plan("manuscript", { mode: "live" }), { wrap: true, code: false, hanging: false, focus: false });
});

const wraps = (state) => state.facet(EditorView.contentAttributes).some((a) => /cm-lineWrapping/.test(a.class ?? ""));

test("extension reconfigures compartments in the same transaction, document untouched", () => {
  let state = EditorState.create({ doc: "a\n    b\n", extensions: [appearanceExtension()] });
  assert.equal(wraps(state), true);
  assert.equal(state.facet(indentUnit), "  ");

  state = state.update({ effects: appearanceEffect.of({ appearance: "code" }) }).state;
  assert.equal(getAppearanceState(state).appearance, "code");
  assert.equal(wraps(state), false);
  assert.equal(state.facet(indentUnit), "    ");
  assert.equal(state.doc.toString(), "a\n    b\n");

  state = state.update({ effects: appearanceEffect.of({ appearance: "book" }) }).state;
  assert.equal(wraps(state), true);
  assert.equal(state.facet(indentUnit), "  ");

  state = state.update({ effects: appearanceEffect.of({ appearance: "code" }) }).state;
  state = state.update({ effects: appearanceEffect.of({ mode: "live" }) }).state;
  assert.equal(wraps(state), true, "live mode re-enables wrapping");
});

test("Session routes setAppearance to the extension and restores it after setDocument", () => {
  const session = new Session([appearanceExtension()]);
  const view = {
    state: session.createState(""),
    dispatch(...specs) {
      const tr = this.state.update(...specs);
      this.state = tr.state;
      session.handleTransactions([tr]);
    },
    setState(s) {
      this.state = s;
    },
  };
  session.attach(view);
  session.setAppearance({ appearance: "code", focus: true, typewriter: true, fontSize: 15, unknown: 1 });
  assert.deepEqual(getAppearanceState(view.state), { appearance: "code", focus: true, typewriter: true, mode: "source" });
  assert.equal(session.appearance.fontSize, 15);
  assert.equal(session.appearance.typewriter, undefined); // owned by appearance.ts now
  session.setAppearance({ focus: false });
  assert.equal(getAppearanceState(view.state).appearance, "code");
  session.setMode("live");
  session.setDocument("# new\n", 3);
  assert.deepEqual(getAppearanceState(view.state), { appearance: "code", focus: false, typewriter: true, mode: "live" });
  assert.equal(wraps(view.state), true);
  session.setMode("source");
  assert.equal(wraps(view.state), false);
  assert.equal(session.getText(), "# new\n");
  assert.equal(session.revision, 3);
});

// ---------------------------------------------------------------- (b) focus mode paragraph ranges
const SAMPLE = [
  "# Title", //          1
  "", //                 2
  "First line of p1.", // 3
  "second line of p1.", //4
  "", //                 5
  "", //                 6
  "Single line p2.", //  7
  "", //                 8
  "p3 a", //             9
  "p3 b", //             10
  "p3 c", //             11
].join("\n");
const doc = Text.of(SAMPLE.split("\n"));
const posOf = (line, col = 0) => doc.line(line).from + col;

test("paragraphRange: run of non-blank lines around the line", () => {
  assert.deepEqual(paragraphRange(doc, 1), [1, 1]);
  assert.deepEqual(paragraphRange(doc, 3), [3, 4]);
  assert.deepEqual(paragraphRange(doc, 4), [3, 4]);
  assert.deepEqual(paragraphRange(doc, 7), [7, 7]);
  assert.deepEqual(paragraphRange(doc, 9), [9, 11]);
  assert.deepEqual(paragraphRange(doc, 11), [9, 11]);
});

test("paragraphRange: a blank line is its own paragraph; whitespace-only counts as blank", () => {
  assert.deepEqual(paragraphRange(doc, 2), [2, 2]);
  assert.deepEqual(paragraphRange(doc, 6), [6, 6]);
  const d = Text.of(["a", "  \t", "b", "c"]);
  assert.deepEqual(paragraphRange(d, 1), [1, 1]);
  assert.deepEqual(paragraphRange(d, 4), [3, 4]);
});

test("focusActiveLines: cursor positions, selections, multiple carets", () => {
  const at = (...ps) => focusActiveLines(doc, ps.map((p) => ({ from: p, to: p })));
  assert.deepEqual(at(0), [[1, 1]]);
  assert.deepEqual(at(posOf(3, 5)), [[3, 4]]);
  assert.deepEqual(at(posOf(4, 18)), [[3, 4]]);
  assert.deepEqual(at(posOf(5)), [[5, 5]]);
  assert.deepEqual(at(doc.length), [[9, 11]]);
  assert.deepEqual(at(posOf(3), posOf(10)), [[3, 4], [9, 11]]);
  // a selection from p1 into p3 keeps everything it covers (including the blank lines between) lit
  assert.deepEqual(focusActiveLines(doc, [{ from: posOf(4), to: posOf(9, 2) }]), [[3, 11]]);
  // overlapping / adjacent ranges merge
  assert.deepEqual(at(posOf(3), posOf(4)), [[3, 4]]);
});

test("guide levels and hanging prefixes (pure helpers)", () => {
  assert.equal(indentLevel("    x"), 1);
  assert.equal(indentLevel("\t\tx"), 2);
  assert.equal(indentLevel("      x"), 1);
  assert.equal(indentLevel("x"), 0);
  const d = Text.of(["if (a) {", "        a();", "", "    b();", "}"]);
  assert.equal(guideLevel(d, 3), 1, "blank line continues the shallower neighbour");
  assert.equal(guideLevel(d, 2), 2);
  assert.deepEqual(hangForLine("## Title"), { hang: 3, lead: 0 });
  assert.deepEqual(hangForLine("- item"), { hang: 2, lead: 0 });
  assert.deepEqual(hangForLine("    - nested"), { hang: 6, lead: 4 });
  assert.deepEqual(hangForLine("12. step"), { hang: 4, lead: 0 });
  assert.deepEqual(hangForLine("> > quote"), { hang: 4, lead: 0 });
  assert.equal(hangForLine("#hashtag"), null);
  assert.equal(hangForLine("-not a list"), null);
  assert.equal(hangForLine("plain"), null);
  assert.equal(hangForLine("-"), null);
});

// ---------------------------------------------------------------- (c) typewriter decision
const base = { typewriter: true, justEnabled: false, docChanged: false, selectionSet: false, userDriven: true, head: 5, prevHead: 5 };

test("shouldRecenter: only when on, and only for user-driven caret/doc changes", () => {
  assert.equal(shouldRecenter({ ...base, typewriter: false, docChanged: true }), false);
  assert.equal(shouldRecenter({ ...base, docChanged: true, head: 6 }), true, "typing");
  assert.equal(shouldRecenter({ ...base, selectionSet: true, head: 9 }), true, "caret moved");
  assert.equal(shouldRecenter({ ...base, selectionSet: true }), false, "selection object changed but head did not");
  assert.equal(shouldRecenter(base), false, "nothing happened (e.g. our own scroll transaction)");
  assert.equal(shouldRecenter({ ...base, docChanged: true, userDriven: false }), false, "host edits do not scroll");
  assert.equal(shouldRecenter({ ...base, justEnabled: true, userDriven: false }), true, "turning typewriter on centres once");
  assert.equal(shouldRecenter({ ...base, typewriter: false, justEnabled: true }), false);
});
