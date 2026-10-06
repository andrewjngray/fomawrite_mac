import test from "node:test";
import assert from "node:assert/strict";
import { Transaction } from "@codemirror/state";
import { Session, listIndent, listOutdent } from "../src/modes.ts";

function fakeView(session) {
  const view = {
    state: session.createState(""),
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

const DOC = "# Title\n\nSome **bold** and *italic* text with `code` and [a link](https://x.y).\n\n- item\n\n> quote\n";

test("source -> live -> source preserves text and revision", () => {
  const session = new Session();
  fakeView(session);
  session.setDocument(DOC, 42);
  assert.equal(session.getMode(), "source");

  session.setMode("live");
  assert.equal(session.getMode(), "live");
  assert.equal(session.getText(), DOC);
  assert.equal(session.revision, 42);

  session.setMode("source");
  assert.equal(session.getMode(), "source");
  assert.equal(session.getText(), DOC);
  assert.equal(session.revision, 42);
});

test("mode survives setDocument and mode switches are not reported as edits", () => {
  const session = new Session();
  fakeView(session);
  const reports = [];
  session.onDocChanged = (j, r) => reports.push([j, r]);
  session.setMode("live");
  session.setDocument("hello", 3);
  assert.equal(session.mode, "live");
  session.setMode("source");
  assert.deepEqual(reports, []);
  assert.equal(session.revision, 3);
});

test("rejects unknown mode", () => {
  const session = new Session();
  fakeView(session);
  assert.throws(() => session.setMode("wysiwyg"));
  assert.equal(session.mode, "source");
});

test("Tab indents by two spaces inside lists, declines elsewhere", () => {
  const session = new Session();
  const view = fakeView(session);
  session.setDocument("para\n\n- one\n- two\n", 0);
  view.dispatch({ selection: { anchor: 9 } }); // inside "- one"
  assert.equal(listIndent(view), true);
  assert.equal(session.getText(), "para\n\n  - one\n- two\n");
  assert.equal(listOutdent(view), true);
  assert.equal(session.getText(), "para\n\n- one\n- two\n");
  view.dispatch({ selection: { anchor: 2 } }); // in the paragraph
  assert.equal(listIndent(view), false);
  assert.equal(session.getText(), "para\n\n- one\n- two\n");
});
