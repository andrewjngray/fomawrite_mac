import test from "node:test";
import assert from "node:assert/strict";
import { EditorSelection, EditorState } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import {
  applyReplacement, buildLinkMarkdown, closeLinkEffect, decodeLinkUrl, encodeLinkUrl, escapeLabel, findLinkAt, linkContext,
  linkField, openLinkEffect, quoteTitle, removeReplacement, replacementSpec, unescapeLabel, unquoteTitle,
} from "../src/links.ts";

const stateOf = (doc, anchor = 0, head = anchor) =>
  EditorState.create({ doc, selection: EditorSelection.single(anchor, head), extensions: [markdown({ base: markdownLanguage }), linkField] });

// ---------------------------------------------------------------- detection
const DOC = 'see [the site](https://example.com/a "Home page") now';
const LINK_FROM = DOC.indexOf("["), LINK_TO = DOC.indexOf(")") + 1;

test("findLinkAt: caret inside a link returns its parts", () => {
  const l = findLinkAt(stateOf(DOC), DOC.indexOf("site"), DOC.indexOf("site"));
  assert.deepEqual(l, { from: LINK_FROM, to: LINK_TO, label: "the site", rawLabel: "the site", url: "https://example.com/a", title: "Home page" });
  // inside the URL part too
  assert.equal(findLinkAt(stateOf(DOC), DOC.indexOf("example"), DOC.indexOf("example")).from, LINK_FROM);
});

test("findLinkAt: touching either edge counts, one character away does not", () => {
  assert.equal(findLinkAt(stateOf(DOC), LINK_FROM, LINK_FROM)?.to, LINK_TO);
  assert.equal(findLinkAt(stateOf(DOC), LINK_TO, LINK_TO)?.from, LINK_FROM);
  assert.equal(findLinkAt(stateOf(DOC), LINK_FROM - 1, LINK_FROM - 1), null);
  assert.equal(findLinkAt(stateOf(DOC), LINK_TO + 1, LINK_TO + 1), null);
  assert.equal(findLinkAt(stateOf(DOC), 0, 0), null);
});

test("findLinkAt: a selection overlapping the link, or covering it, finds it", () => {
  assert.equal(findLinkAt(stateOf(DOC), 2, LINK_FROM + 3)?.from, LINK_FROM);
  assert.equal(findLinkAt(stateOf(DOC), LINK_FROM, LINK_TO)?.to, LINK_TO);
});

test("findLinkAt: link without a title, with a single-quoted or parenthesised title, with an angle-bracket URL", () => {
  const plain = "[a](b.md)";
  assert.deepEqual(findLinkAt(stateOf(plain), 2, 2), { from: 0, to: plain.length, label: "a", rawLabel: "a", url: "b.md", title: "" });
  assert.equal(findLinkAt(stateOf("[a](b 'it\\'s')"), 1, 1).title, "it's");
  assert.equal(findLinkAt(stateOf("[a](b (paren))"), 1, 1).title, "paren");
  assert.equal(findLinkAt(stateOf("[a](<my file.md>)"), 1, 1).url, "my file.md");
  assert.equal(findLinkAt(stateOf("[a]()"), 1, 1).url, "");
});

test("findLinkAt: URL escapes made by the editor are shown as typed", () => {
  assert.equal(findLinkAt(stateOf("[a](my%20file%20%281%29.md)"), 1, 1).url, "my file (1).md");
  assert.equal(findLinkAt(stateOf("[a](https://x.org/a%20b_%28c%29)"), 1, 1).url, "https://x.org/a b_(c)");
  assert.equal(findLinkAt(stateOf("[a](100%25.md)"), 1, 1).url, "100%.md");
  assert.equal(findLinkAt(stateOf("[a](https://x.org/100%25)"), 1, 1).url, "https://x.org/100%25"); // schemed URLs keep %25
});

test("findLinkAt: label with escaped brackets and inline markup", () => {
  const l = findLinkAt(stateOf("[a \\[1\\] **b**](u)"), 1, 1);
  assert.equal(l.label, "a [1] **b**");
  assert.equal(l.rawLabel, "a \\[1\\] **b**");
});

test("findLinkAt: not inline links - references, shortcuts, images, autolinks, unfinished", () => {
  for (const doc of ["[a][b]", "[a]", "![alt](u.png)", "<https://example.com>", "[a](b(c) \"t\"", "plain text"]) {
    for (let p = 0; p <= doc.length; p++) assert.equal(findLinkAt(stateOf(doc), p, p), null, `${doc} @${p}`);
  }
});

test("findLinkAt: an image inside a link label does not hide the link; the link is found at its own edges", () => {
  const doc = "[![i](u.png)](https://x.org)";
  const l = findLinkAt(stateOf(doc), 0, 0);
  assert.equal(l.from, 0);
  assert.equal(l.url, "https://x.org");
  assert.equal(l.label, "![i](u.png)");
});

test("findLinkAt: with two links the one holding the head wins", () => {
  const doc = "[a](1) [b](2)";
  assert.equal(findLinkAt(stateOf(doc), 0, doc.length, doc.length)?.url, "2");
  assert.equal(findLinkAt(stateOf(doc), 0, doc.length, 0)?.url, "1");
  assert.equal(findLinkAt(stateOf(doc), 6, 8)?.url, "2"); // touches the end of the first and the start of the second: head (8) is in the second
});

// ---------------------------------------------------------------- context
test("linkContext: existing link", () => {
  const c = linkContext(stateOf(DOC, DOC.indexOf("site")));
  assert.equal(c.existing, true);
  assert.deepEqual([c.from, c.to, c.label, c.url, c.title], [LINK_FROM, LINK_TO, "the site", "https://example.com/a", "Home page"]);
});

test("linkContext: selected text becomes the label and is replaced on apply", () => {
  const s = stateOf("say hello world", 4, 9);
  const c = linkContext(s);
  assert.deepEqual(c, { from: 4, to: 9, label: "hello", url: "", title: "", existing: false, rawLabel: "" });
});

test("linkContext: caret outside a link gives an empty label and a zero-width range", () => {
  const c = linkContext(stateOf("say hello", 3));
  assert.deepEqual([c.from, c.to, c.label, c.existing], [3, 3, "", false]);
});

test("linkContext: a multi-line selection is not consumed", () => {
  const c = linkContext(stateOf("one\ntwo\nthree", 1, 9));
  assert.deepEqual([c.from, c.to, c.label, c.existing], [9, 9, "", false]);
});

// ---------------------------------------------------------------- text builder
test("encodeLinkUrl: spaces and parentheses are percent-encoded like image paths", () => {
  assert.equal(encodeLinkUrl("my file (1).md"), "my%20file%20%281%29.md");
  assert.equal(encodeLinkUrl("docs/100% sure.md"), "docs/100%25%20sure.md"); // relative: % encoded as in images.ts
  assert.equal(encodeLinkUrl("  notes.md  "), "notes.md");
  assert.equal(encodeLinkUrl("#heading"), "#heading");
});

test("encodeLinkUrl: a URL with a scheme is kept as typed apart from spaces and parentheses", () => {
  assert.equal(encodeLinkUrl("https://example.com/a%20b?q=1&r=%C3%A9#x"), "https://example.com/a%20b?q=1&r=%C3%A9#x");
  assert.equal(encodeLinkUrl("https://en.wikipedia.org/wiki/Foo_(bar)"), "https://en.wikipedia.org/wiki/Foo_%28bar%29");
  assert.equal(encodeLinkUrl("https://example.com/a b"), "https://example.com/a%20b");
  assert.equal(encodeLinkUrl("mailto:a@b.co"), "mailto:a@b.co");
  assert.equal(encodeLinkUrl("tel:+1555"), "tel:+1555");
});

test("decodeLinkUrl inverts encodeLinkUrl for the characters it encodes", () => {
  for (const u of ["my file (1).md", "docs/100% sure.md", "https://example.com/a b_(c)", "https://example.com/a%20b".replace("%20", " ")])
    assert.equal(decodeLinkUrl(encodeLinkUrl(u)), u);
});

test("escapeLabel / unescapeLabel: brackets escaped, line breaks flattened", () => {
  assert.equal(escapeLabel("a [1] b"), "a \\[1\\] b");
  assert.equal(unescapeLabel(escapeLabel("[x]")), "[x]");
  assert.equal(escapeLabel("two\nlines\r\nhere"), "two lines here");
  assert.equal(escapeLabel("**bold** `code`"), "**bold** `code`");
});

test("quoteTitle / unquoteTitle", () => {
  assert.equal(quoteTitle("Home"), '"Home"');
  assert.equal(quoteTitle('say "hi" \\ there'), '"say \\"hi\\" \\\\ there"');
  assert.equal(quoteTitle("a\nb"), '"a b"');
  assert.equal(unquoteTitle(quoteTitle('say "hi" \\ there')), 'say "hi" \\ there');
});

test("buildLinkMarkdown: title omitted when empty or blank, label falls back to the URL", () => {
  assert.equal(buildLinkMarkdown({ label: "site", url: "https://example.com", title: "" }), "[site](https://example.com)");
  assert.equal(buildLinkMarkdown({ label: "site", url: "https://example.com", title: "  " }), "[site](https://example.com)");
  assert.equal(buildLinkMarkdown({ label: "site", url: "https://example.com", title: "Home" }), '[site](https://example.com "Home")');
  assert.equal(buildLinkMarkdown({ label: "", url: "https://example.com", title: "" }), "[https://example.com](https://example.com)");
  assert.equal(buildLinkMarkdown({ label: "a [b]", url: "my file.md", title: 'q"' }), '[a \\[b\\]](my%20file.md "q\\"")');
});

test("built Markdown parses back to the same values", () => {
  const cases = [
    { label: "a [b] c", url: "https://example.com/x_(y) z", title: 'He said "hi"' },
    { label: "plain", url: "notes/my file (2).md", title: "" },
    { label: "p", url: "../100% done.md", title: "t" },
  ];
  for (const v of cases) {
    const md = buildLinkMarkdown(v);
    assert.deepEqual(
      (({ label, url, title }) => ({ label, url, title }))(findLinkAt(stateOf(md), 1, 1)),
      v,
      md,
    );
  }
});

// ---------------------------------------------------------------- replacement ranges
test("applyReplacement: existing link is replaced as a whole", () => {
  const s = stateOf(DOC, DOC.indexOf("site"));
  const ctx = linkContext(s);
  const r = applyReplacement(ctx, { label: "new", url: "https://b.org", title: "" });
  assert.deepEqual(r, { from: LINK_FROM, to: LINK_TO, insert: "[new](https://b.org)", caret: LINK_FROM + "[new](https://b.org)".length });
  const tr = s.update(replacementSpec(s, r, "input.link"));
  assert.equal(tr.newDoc.toString(), "see [new](https://b.org) now");
  assert.equal(tr.newSelection.main.head, LINK_FROM + 20);
  assert.equal(tr.newSelection.main.empty, true);
  assert.equal(tr.isUserEvent("input.link"), true);
  assert.equal(tr.isUserEvent("input"), true);
});

test("applyReplacement: selection is replaced by the link, caret goes after it", () => {
  const s = stateOf("say hello world", 4, 9);
  const r = applyReplacement(linkContext(s), { label: "hello", url: "https://e.org", title: "Greeting" });
  const tr = s.update(replacementSpec(s, r, "input.link"));
  assert.equal(tr.newDoc.toString(), 'say [hello](https://e.org "Greeting") world');
  assert.equal(tr.newSelection.main.head, 4 + '[hello](https://e.org "Greeting")'.length);
});

test("applyReplacement: a caret inserts at the caret; an empty URL applies nothing", () => {
  const s = stateOf("ab", 1);
  const ctx = linkContext(s);
  assert.equal(applyReplacement(ctx, { label: "x", url: "  ", title: "" }), null);
  const r = applyReplacement(ctx, { label: "x", url: "u", title: "" });
  assert.deepEqual([r.from, r.to], [1, 1]);
  assert.equal(s.update(replacementSpec(s, r, "input.link")).newDoc.toString(), "a[x](u)b");
});

test("applyReplacement: applying unchanged values to an existing link is a no-op", () => {
  for (const doc of [DOC, "[a](my%20file.md)", '[a \\[1\\]](https://x.org/q_%28w%29 "t \\"q\\"")']) {
    const s = stateOf(doc, doc.indexOf("[") + 2);
    const ctx = linkContext(s);
    assert.equal(ctx.existing, true, doc);
    const r = applyReplacement(ctx, ctx);
    assert.equal(replacementSpec(s, r, "input.link"), null, doc);
  }
});

test("removeReplacement: the link becomes its label text, caret after it", () => {
  const s = stateOf(DOC, DOC.indexOf("site"));
  const r = removeReplacement(linkContext(s));
  assert.deepEqual(r, { from: LINK_FROM, to: LINK_TO, insert: "the site", caret: LINK_FROM + 8 });
  const tr = s.update(replacementSpec(s, r, "delete.link"));
  assert.equal(tr.newDoc.toString(), "see the site now");
  assert.equal(tr.newSelection.main.head, LINK_FROM + 8);
  assert.equal(tr.isUserEvent("delete.link"), true);
  assert.equal(tr.isUserEvent("delete"), true);
});

test("removeReplacement keeps the label's markup and escapes; it is unavailable without a link", () => {
  const s = stateOf("x [a \\[1\\] **b**](u) y", 4);
  assert.equal(removeReplacement(linkContext(s)).insert, "a \\[1\\] **b**");
  assert.equal(removeReplacement(linkContext(stateOf("plain", 2))), null);
});

test("an applied link is undoable as one user change", async () => {
  const { history, undo } = await import("@codemirror/commands");
  let s = EditorState.create({ doc: "hi there", selection: { anchor: 0, head: 2 }, extensions: [markdown({ base: markdownLanguage }), history()] });
  const r = applyReplacement(linkContext(s), { label: "hi", url: "u", title: "" });
  s = s.update(replacementSpec(s, r, "input.link")).state;
  assert.equal(s.doc.toString(), "[hi](u) there");
  let out = null;
  undo({ state: s, dispatch: (tr) => (out = tr.state) });
  assert.equal(out.doc.toString(), "hi there");
});

// ---------------------------------------------------------------- panel state field
test("linkField is opened and closed by effects", () => {
  const s = stateOf("a", 0);
  assert.equal(s.field(linkField), null);
  const ctx = linkContext(s);
  const open = s.update({ effects: openLinkEffect.of(ctx) }).state;
  assert.equal(open.field(linkField), ctx);
  assert.equal(open.update({ effects: closeLinkEffect.of(null) }).state.field(linkField), null);
});
