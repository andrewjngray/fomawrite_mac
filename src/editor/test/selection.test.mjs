import test from "node:test";
import assert from "node:assert/strict";
import { EditorState } from "@codemirror/state";
import { createMockBridge } from "../src/bridge.ts";
import { replySelection } from "../src/selection.ts";

// The host's requestSelection(token) is answered from the live editor state,
// never from the debounced cursorChanged report.

test("requestSelection is part of the mock bridge and selectionReply is recorded", () => {
  const bridge = createMockBridge();
  const seen = [];
  bridge.requestSelection.connect((token) => seen.push(token));
  bridge.emit("requestSelection", 7);
  assert.deepEqual(seen, [7]);
  bridge.selectionReply(7, 1, 2);
  assert.deepEqual(bridge.calls.at(-1), { name: "selectionReply", args: [7, 1, 2] });
});

test("replySelection answers with the token and the current main selection (anchor, head)", () => {
  const bridge = createMockBridge();
  const view = { state: EditorState.create({ doc: "one two three", selection: { anchor: 4, head: 7 } }) };
  replySelection(bridge, view, 41);
  assert.deepEqual(bridge.calls.at(-1), { name: "selectionReply", args: [41, 4, 7] });
});

test("replySelection reports a backwards selection as anchor > head, and a caret as anchor == head", () => {
  const bridge = createMockBridge();
  replySelection(bridge, { state: EditorState.create({ doc: "abcdef", selection: { anchor: 5, head: 2 } }) }, 1);
  assert.deepEqual(bridge.calls.at(-1).args, [1, 5, 2]);
  replySelection(bridge, { state: EditorState.create({ doc: "abcdef", selection: { anchor: 3 } }) }, 2);
  assert.deepEqual(bridge.calls.at(-1).args, [2, 3, 3]);
});

test("replySelection reads the state at request time: a selection changed just before is reported at once", () => {
  const bridge = createMockBridge();
  // The host last heard (anchor 0, head 0) from the debounced report; the
  // state has since moved. The reply must not be the stale value.
  const view = { state: EditorState.create({ doc: "one two three", selection: { anchor: 0 } }) };
  bridge.cursorChanged(0, 0, true); // the last debounced report the host holds
  view.state = view.state.update({ selection: { anchor: 4, head: 7 } }).state;
  replySelection(bridge, view, 9);
  assert.deepEqual(bridge.calls.at(-1), { name: "selectionReply", args: [9, 4, 7] });
});
