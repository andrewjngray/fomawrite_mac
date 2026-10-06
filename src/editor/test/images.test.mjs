import test from "node:test";
import assert from "node:assert/strict";
import { EditorState } from "@codemirror/state";
import {
  MAX_IMAGE_BYTES, addPendingEffect, base64ByteLength, encodeImagePath, imageAlt, imageMarkdown, insertionSpec, mapPending,
  pendingField, settleEffect, suggestFileName, validateImage, validateImagePayload,
} from "../src/images.ts";
import { createMockBridge } from "../src/bridge.ts";

// ---------------------------------------------------------------- validation
test("validateImage: only png/jpeg/gif/webp, non-empty, at most 20 MiB", () => {
  for (const m of ["image/png", "image/jpeg", "image/gif", "image/webp"]) assert.equal(validateImage(m, 10), null);
  for (const m of ["image/svg+xml", "image/tiff", "image/bmp", "text/plain", "", "IMAGE/PNG", "image/jpg"]) assert.match(validateImage(m, 10), /unsupported image type/);
  assert.equal(validateImage("image/png", MAX_IMAGE_BYTES), null);
  assert.match(validateImage("image/png", MAX_IMAGE_BYTES + 1), /too large/);
  assert.match(validateImage("image/png", 0), /empty/);
});

test("base64ByteLength / validateImagePayload", () => {
  assert.equal(base64ByteLength("AAAA"), 3);
  assert.equal(base64ByteLength("AAA="), 2);
  assert.equal(base64ByteLength("AA=="), 1);
  assert.equal(base64ByteLength(""), -1);
  assert.equal(base64ByteLength("AAA"), -1); // length not a multiple of 4
  assert.equal(base64ByteLength("AA A"), -1); // whitespace
  assert.equal(base64ByteLength("A=AA"), -1); // padding in the middle
  assert.equal(base64ByteLength("data:image/png;base64,AAAA"), -1);
  const png = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==";
  assert.equal(validateImagePayload("image/png", png), null);
  assert.match(validateImagePayload("image/png", "not base64!"), /base64/);
  assert.match(validateImagePayload("image/svg+xml", png), /unsupported/);
  const big = "A".repeat(Math.floor(MAX_IMAGE_BYTES / 3) * 4 + 4); // 20 MiB + 1 byte
  assert.match(validateImagePayload("image/png", big), /too large/);
  assert.equal(validateImagePayload("image/png", "A".repeat(Math.floor(MAX_IMAGE_BYTES / 3) * 4)), null);
});

// ---------------------------------------------------------------- names and Markdown text
test("suggestFileName keeps the file's name, fixes the extension, falls back to a timestamp", () => {
  const now = new Date(2026, 9, 6, 7, 5, 9);
  assert.equal(suggestFileName("photo.JPG", "image/jpeg", now), "photo.JPG");
  assert.equal(suggestFileName("my shot", "image/png", now), "my shot.png");
  assert.equal(suggestFileName("/Users/me/Desktop/a b.png", "image/png", now), "a b.png");
  assert.equal(suggestFileName("", "image/webp", now), "pasted-image-20261006-070509.webp");
  assert.equal(suggestFileName(undefined, "image/jpeg", now), "pasted-image-20261006-070509.jpg");
  assert.equal(suggestFileName("\u0000\n", "image/gif", now), "pasted-image-20261006-070509.gif");
});

test("imageMarkdown: alt is the stem, spaces and parentheses are percent-encoded", () => {
  assert.equal(imageMarkdown("assets/cat.png"), "![cat](assets/cat.png)");
  assert.equal(imageMarkdown("assets/my cat (1).png"), "![my cat (1)](assets/my%20cat%20%281%29.png)");
  assert.equal(imageMarkdown("a b/c.d.png"), "![c.d](a%20b/c.d.png)");
  assert.equal(imageMarkdown("100%.png"), "![100%](100%25.png)");
  assert.equal(imageMarkdown("x[1].png"), "![x\\[1\\]](x[1].png)");
  assert.equal(imageMarkdown("noext"), "![noext](noext)");
  assert.equal(imageMarkdown(".hidden"), "![.hidden](.hidden)");
  assert.equal(imageAlt("dir/"), "image");
  assert.equal(encodeImagePath("a\tb c"), "a%09b%C2%A0c");
});

// ---------------------------------------------------------------- pending positions
const stateWith = (doc, pos = doc.length) => EditorState.create({ doc, selection: { anchor: pos }, extensions: [pendingField] });
const add = (state, token, pos, extra = {}) =>
  state.update({ effects: addPendingEffect.of({ token, pending: { pos, batch: 1, index: 0, count: 1, ...extra } }) }).state;

test("a pending position is mapped through later edits (before: shifts, after: unchanged, at: stays before typed text)", () => {
  let s = add(stateWith("hello world", 5), 1, 5);
  s = s.update({ changes: { from: 0, insert: ">>> " }, userEvent: "input" }).state; // before the position
  assert.equal(s.field(pendingField).get(1).pos, 9);
  s = s.update({ changes: { from: s.doc.length, insert: "!!!" } }).state; // after the position
  assert.equal(s.field(pendingField).get(1).pos, 9);
  s = s.update({ changes: { from: 9, insert: "XY" } }).state; // typed exactly at the position: image goes before it
  assert.equal(s.field(pendingField).get(1).pos, 9);
  s = s.update({ changes: { from: 2, to: 12, insert: "" } }).state; // deleted across the position
  assert.equal(s.field(pendingField).get(1).pos, 2);
});

test("mapPending works on a bare ChangeSet", () => {
  const s = stateWith("abcdef");
  const cs = s.update({ changes: [{ from: 1, insert: "123" }, { from: 5, to: 6 }] }).changes;
  const out = mapPending(new Map([[7, { pos: 3, batch: 1, index: 0, count: 1 }], [8, { pos: 6, batch: 1, index: 0, count: 1 }]]), cs);
  assert.equal(out.get(7).pos, 6);
  assert.equal(out.get(8).pos, 8);
});

test("insertionSpec inserts at the mapped position and settles the token; the caret follows only when it sat there", () => {
  let s = add(stateWith("ab cd", 2), 1, 2);
  s = s.update({ changes: { from: 0, insert: "XX" } }).state; // typed before; caret maps to 4, pending to 4
  const spec = insertionSpec(s, 1, "assets/my pic.png");
  const next = s.update(spec).state;
  assert.equal(next.doc.toString(), "XXab![my pic](assets/my%20pic.png) cd");
  assert.equal(next.selection.main.head, 4 + "![my pic](assets/my%20pic.png)".length);
  assert.equal(next.field(pendingField).size, 0);
  assert.equal(insertionSpec(next, 1, "assets/x.png"), null); // already settled
  // caret elsewhere: left alone
  const t = add(stateWith("ab cd", 5), 2, 2);
  const after = t.update(insertionSpec(t, 2, "p.png")).state;
  assert.equal(after.doc.toString(), "ab![p](p.png) cd");
  assert.equal(after.selection.main.head, after.doc.length);
});

test("a typed-after-paste character stays after the image", () => {
  let s = add(stateWith("ab", 2), 1, 2);
  s = s.update({ changes: { from: 2, insert: "Z" }, selection: { anchor: 3 } }).state;
  s = s.update(insertionSpec(s, 1, "p.png")).state;
  assert.equal(s.doc.toString(), "ab![p](p.png)Z");
});

test("a failure (settle without text) only forgets the token; unknown tokens give no spec", () => {
  let s = add(stateWith("abc"), 3, 1);
  s = s.update({ effects: settleEffect.of({ token: 3, len: 0 }) }).state;
  assert.equal(s.field(pendingField).size, 0);
  assert.equal(insertionSpec(s, 3, "p.png"), null);
  assert.equal(insertionSpec(stateWith("abc"), 99, "p.png"), null);
});

test("a multi-file drop inserts one image per line, in file order, whatever order the host answers in", () => {
  const run = (order) => {
    let s = stateWith("a|b", 1);
    for (let i = 0; i < 3; i++) s = add(s, i + 1, 1, { batch: 5, index: i, count: 3 });
    for (const t of order) s = s.update(insertionSpec(s, t, `${t}.png`)).state;
    return s.doc.toString();
  };
  const want = "a![1](1.png)\n![2](2.png)\n![3](3.png)|b";
  assert.equal(run([1, 2, 3]), want);
  assert.equal(run([3, 1, 2]), want);
  assert.equal(run([2, 3, 1]), want);
});

test("a replaced document (new state) forgets pending saves", () => {
  const s = add(stateWith("abc"), 1, 1);
  assert.equal(insertionSpec(stateWith("new doc"), 1, "p.png"), null);
  assert.equal(s.field(pendingField).size, 1);
});

// ---------------------------------------------------------------- mock round trip
test("mock bridge answers saveImage with imageSaved(token, assets/<name>, '') after ~50 ms", async () => {
  const bridge = createMockBridge();
  const got = [];
  bridge.imageSaved.connect((...a) => got.push(a));
  const t0 = Date.now();
  bridge.saveImage(7, "shot one.png", "image/png", "AAAA");
  assert.equal(got.length, 0);
  await new Promise((r) => setTimeout(r, 120));
  assert.deepEqual(got, [[7, "assets/shot one.png", ""]]);
  assert.ok(Date.now() - t0 >= 40);
  assert.deepEqual(bridge.calls.find((c) => c.name === "saveImage").args, [7, "shot one.png", "image/png", "AAAA"]);
});
