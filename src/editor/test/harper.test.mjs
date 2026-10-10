import test from "node:test";
import assert from "node:assert/strict";
import { existsSync } from "node:fs";
import { createMockBridge, mockHarperLint } from "../src/bridge.ts";
import { HarperService, lintWith, mockHarperBackend, realHarperBackend, wireHarperBridge } from "../src/harper.ts";

// The page as the host's lint service: queue, ready, reply, failure and token rules, against a fake engine (no WebAssembly).

const tick = async (n = 6) => { for (let i = 0; i < n; i++) await new Promise((r) => setTimeout(r, 0)); };
const deferred = () => { let resolve, reject; const promise = new Promise((a, b) => { resolve = a; reject = b; }); return { promise, resolve, reject }; };
const finding = (start, end, kind = "Spelling", suggestions = ["x"]) => ({ start, end, kind, message: kind + " issue", suggestions });

/**
 * A fake backend: every load() is held until the test settles it (`loads[i].resolve()/reject()`), every engine records its
 * calls in `log` and answers lint(text) with one finding spanning the text (or what `lintImpl` returns).
 */
function fakeBackend({ version = "9.9.9", lintImpl } = {}) {
  const log = [];
  const loads = [];
  const backend = {
    log, loads,
    load(dialect) {
      const d = deferred();
      const entry = { dialect, ...d, engine: null };
      loads.push(entry);
      log.push("load " + dialect);
      return d.promise.then(() => {
        entry.engine = {
          dialect,
          lint: async (text) => { log.push(`lint ${dialect} ${text}`); return lintImpl ? lintImpl(text, dialect) : [finding(0, text.length)]; },
          importWords: async (words) => { log.push(`words ${dialect} ${words.join(",")}`); },
        };
        return entry.engine;
      });
    },
    version: async () => version,
  };
  return backend;
}

function setup(opts) {
  const bridge = createMockBridge();
  const backend = fakeBackend(opts);
  const service = wireHarperBridge(bridge, backend);
  const sent = (name) => bridge.calls.filter((c) => c.name === name).map((c) => c.args);
  return { bridge, backend, service, sent };
}

test("harperLoad: the engine loads, then harperReady(version) says so once", async () => {
  const { bridge, backend, service, sent } = setup();
  bridge.emit("harperLoad", "Australian");
  await tick();
  assert.deepEqual(backend.log, ["load Australian"]);
  assert.equal(service.ready, false);
  assert.deepEqual(sent("harperReady"), [], "nothing is announced before the engine can lint");
  backend.loads[0].resolve();
  await tick();
  assert.equal(service.ready, true);
  assert.deepEqual(sent("harperReady"), [["9.9.9"]]);
});

test("the version falls back to \"unknown\" when it cannot be read", async () => {
  const { bridge, backend, sent } = setup();
  backend.version = async () => { throw new Error("no VERSION file"); };
  bridge.emit("harperLoad", "British");
  backend.loads[0].resolve();
  await tick();
  assert.deepEqual(sent("harperReady"), [["unknown"]]);
  // and the real backend, which fetches fomawrite://harper/VERSION, never rejects (node has no such resource)
  assert.equal(await realHarperBackend().version(), "unknown");
});

test("requests that arrive before the engine is ready are queued and answered in order once it is", async () => {
  const { bridge, backend, service, sent } = setup();
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, "first text");
  bridge.emit("harperLint", 2, "second");
  bridge.emit("harperLint", 3, "third one");
  await tick();
  assert.equal(service.queued, 3);
  assert.deepEqual(sent("harperReply"), []);
  backend.loads[0].resolve();
  await tick(20);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1, 2, 3], "in order");
  assert.deepEqual(JSON.parse(sent("harperReply")[0][1]), [finding(0, 10)], "the reply is the engine's findings as JSON");
  assert.deepEqual(backend.log.filter((l) => l.startsWith("lint")), ["lint Australian first text", "lint Australian second", "lint Australian third one"]);
  assert.deepEqual(sent("harperFailed"), []);
  assert.equal(service.queued, 0);
});

test("lint answers keep the engine's order even when an earlier lint is slow", async () => {
  const slow = deferred();
  const { bridge, backend, sent } = setup({ lintImpl: (text) => (text === "slow" ? slow.promise : [finding(0, 1)]) });
  bridge.emit("harperLoad", "American");
  backend.loads[0].resolve();
  await tick();
  bridge.emit("harperLint", 1, "slow");
  bridge.emit("harperLint", 2, "fast");
  await tick();
  assert.deepEqual(sent("harperReply"), [], "the second waits for the first");
  slow.resolve([finding(0, 4)]);
  await tick(10);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1, 2]);
});

test("harperLint with empty text answers an empty array at once, before the engine is ready and without touching it", () => {
  const { bridge, backend, sent } = setup();
  bridge.emit("harperLint", 5, "");
  assert.deepEqual(sent("harperReply"), [[5, "[]"]], "synchronously");
  assert.deepEqual(backend.log, [], "no load was started for it");
  bridge.emit("harperLint", 5, ""); // empty is answered every time (nothing was queued)
  assert.equal(sent("harperReply").length, 2);
});

test("a lint before any harperLoad starts the default (Australian) engine and is answered when it is ready", async () => {
  const { bridge, backend, sent } = setup();
  bridge.emit("harperLint", 1, "hello");
  await tick();
  assert.deepEqual(backend.log, ["load Australian"]);
  backend.loads[0].resolve();
  await tick(10);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1]);
  assert.equal(sent("harperReady").length, 1);
});

test("a reload with another dialect keeps the queue; the abandoned load announces nothing; answers come from the new engine", async () => {
  const { bridge, backend, service, sent } = setup();
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, "one");
  bridge.emit("harperLoad", "British");
  bridge.emit("harperLint", 2, "two");
  await tick();
  assert.deepEqual(backend.log, ["load Australian", "load British"]);
  backend.loads[0].resolve(); // the abandoned one finishes first
  await tick(10);
  assert.equal(service.ready, false, "the replaced engine is not used");
  assert.deepEqual(sent("harperReady"), []);
  assert.deepEqual(sent("harperReply"), []);
  backend.loads[1].resolve();
  await tick(20);
  assert.deepEqual(sent("harperReady"), [["9.9.9"]], "one ready, for the engine that stayed");
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1, 2], "queued requests were not dropped");
  assert.deepEqual(backend.log.filter((l) => l.startsWith("lint")), ["lint British one", "lint British two"]);
});

test("a reload while a lint is in flight: that lint is answered once (by the engine it started on), the queue goes on with the new one", async () => {
  const hold = deferred();
  const { bridge, backend, sent } = setup({ lintImpl: (text) => (text === "held" ? hold.promise : [finding(0, 1)]) });
  bridge.emit("harperLoad", "Australian");
  backend.loads[0].resolve();
  await tick();
  bridge.emit("harperLint", 1, "held");
  bridge.emit("harperLint", 2, "after");
  await tick();
  bridge.emit("harperLoad", "Canadian");
  await tick();
  hold.resolve([finding(0, 4)]);
  await tick(10);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1], "answered once, the next one waits for the new engine");
  backend.loads[1].resolve();
  await tick(20);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1, 2]);
  assert.deepEqual(backend.log.filter((l) => l.startsWith("lint")), ["lint Australian held", "lint Canadian after"]);
});

test("harperLoad with the dialect already loading starts nothing; with it already loaded it announces ready again", async () => {
  const { bridge, backend, sent } = setup();
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLoad", "Australian");
  assert.equal(backend.loads.length, 1);
  backend.loads[0].resolve();
  await tick();
  assert.equal(sent("harperReady").length, 1);
  bridge.emit("harperLoad", "Australian");
  await tick();
  assert.equal(backend.loads.length, 1, "no second load");
  assert.equal(sent("harperReady").length, 2, "a host that waits for ready is not left hanging");
  bridge.emit("harperLoad", "Klingon"); // unknown names mean the default, which is the loaded one
  await tick();
  assert.equal(backend.loads.length, 1);
});

test("a token is never answered twice: a repeat while it is queued or being linted is ignored", async () => {
  const { bridge, backend, sent } = setup();
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 7, "text");
  bridge.emit("harperLint", 7, "text");
  backend.loads[0].resolve();
  await tick(20);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [7]);
  assert.equal(backend.log.filter((l) => l.startsWith("lint")).length, 1);
  // once answered, the token is free again (the host numbers its own requests)
  bridge.emit("harperLint", 8, "next");
  await tick(10);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [7, 8]);
});

test("a lint that throws answers harperFailed for that token only; later ones are fine", async () => {
  const { bridge, backend, sent } = setup({ lintImpl: (text) => { if (text === "boom") throw new Error("lint exploded"); return [finding(0, 1)]; } });
  bridge.emit("harperLoad", "Australian");
  backend.loads[0].resolve();
  await tick();
  bridge.emit("harperLint", 1, "fine");
  bridge.emit("harperLint", 2, "boom");
  bridge.emit("harperLint", 3, "fine again");
  await tick(20);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1, 3]);
  assert.deepEqual(sent("harperFailed"), [[2, "lint exploded"]]);
});

test("a load that fails answers harperFailed(-1) once and fails every queued token; later lints fail at once; harperLoad retries", async () => {
  const { bridge, backend, service, sent } = setup();
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, "one");
  bridge.emit("harperLint", 2, "two");
  backend.loads[0].reject(new Error("wasm would not load"));
  await tick(10);
  assert.deepEqual(sent("harperFailed"), [[-1, "wasm would not load"], [1, "wasm would not load"], [2, "wasm would not load"]]);
  assert.deepEqual(sent("harperReady"), []);
  assert.equal(service.queued, 0);
  bridge.emit("harperLint", 3, "three");
  assert.deepEqual(sent("harperFailed").at(-1), [3, "wasm would not load"], "no waiting on an engine that is not coming");
  bridge.emit("harperLint", 4, ""); // empty text never needs the engine
  assert.deepEqual(sent("harperReply"), [[4, "[]"]]);
  // the next harperLoad tries again
  bridge.emit("harperLoad", "Australian");
  assert.equal(backend.loads.length, 2);
  backend.loads[1].resolve();
  await tick(10);
  assert.deepEqual(sent("harperReady"), [["9.9.9"]]);
  bridge.emit("harperLint", 5, "five");
  await tick(10);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [4, 5]);
});

test("a failure of an abandoned load is not reported (the newer load decides)", async () => {
  const { bridge, backend, sent } = setup();
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, "one");
  bridge.emit("harperLoad", "British");
  backend.loads[0].reject(new Error("old one died"));
  await tick(10);
  assert.deepEqual(sent("harperFailed"), []);
  backend.loads[1].resolve();
  await tick(20);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1]);
});

test("harperImportWords: kept until the engine is there, imported before the first lint, and again after every reload", async () => {
  const { bridge, backend, sent } = setup();
  bridge.emit("harperImportWords", JSON.stringify(["Fomawrite", "Potentia", "", 3, " "]));
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, "one");
  backend.loads[0].resolve();
  await tick(20);
  assert.deepEqual(backend.log.slice(1), ["words Australian Fomawrite,Potentia", "lint Australian one"], "words first, only real words");
  assert.equal(sent("harperReply").length, 1);
  // a later batch while ready goes in before the next lint
  bridge.emit("harperImportWords", JSON.stringify(["Fomawrite", "Harper"]));
  bridge.emit("harperLint", 2, "two");
  await tick(20);
  assert.deepEqual(backend.log.slice(3), ["words Australian Fomawrite,Harper", "lint Australian two"]);
  // a reload imports the last list into the new engine
  bridge.emit("harperLoad", "British");
  bridge.emit("harperLint", 3, "three");
  backend.loads[1].resolve();
  await tick(20);
  assert.deepEqual(backend.log.slice(5), ["load British", "words British Fomawrite,Harper", "lint British three"]);
  // not JSON, not an array: ignored, the last list stays
  bridge.emit("harperImportWords", "not json");
  bridge.emit("harperImportWords", JSON.stringify({ words: ["x"] }));
  bridge.emit("harperLint", 4, "four");
  await tick(20);
  assert.deepEqual(backend.log.slice(8), ["lint British four"]);
  // an empty list is a list too (the writer forgot every word): the engine gets it
  bridge.emit("harperImportWords", "[]");
  bridge.emit("harperLint", 5, "five");
  await tick(20);
  assert.deepEqual(backend.log.slice(9), ["words British ", "lint British five"]);
});

test("an engine whose importWords throws still lints (the dictionary is a convenience)", async () => {
  const { bridge, backend, sent } = setup();
  const original = backend.load.bind(backend);
  backend.load = (d) => original(d).then((engine) => { engine.importWords = async () => { throw new Error("no dictionary"); }; return engine; });
  bridge.emit("harperImportWords", '["a"]');
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, "one");
  backend.loads[0].resolve();
  await tick(20);
  assert.equal(sent("harperReady").length, 1);
  assert.deepEqual(sent("harperReply").map((a) => a[0]), [1]);
  assert.deepEqual(sent("harperFailed"), []);
});

test("a host without the Harper slots, or a throwing slot, does not break the service", async () => {
  const bare = { log: () => {} };
  const service = new HarperService(bare, fakeBackend());
  service.lint(1, ""); // no harperReply slot: nothing to call, nothing thrown
  const logged = [];
  const angry = { log: (m) => logged.push(m), harperReply() { throw new Error("slot blew up"); } };
  new HarperService(angry, fakeBackend()).lint(2, "");
  assert.equal(logged.length, 1);
  assert.match(logged[0], /slot blew up/);
});

test("the mock backend (dev server): ready as \"mock\" after 10 ms, fixed findings", async () => {
  const bridge = createMockBridge();
  wireHarperBridge(bridge, mockHarperBackend());
  const t0 = Date.now();
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, "We recieve it. This is is very unique.");
  await new Promise((r) => setTimeout(r, 80));
  const ready = bridge.calls.filter((c) => c.name === "harperReady");
  assert.deepEqual(ready.map((c) => c.args), [["mock"]]);
  assert.ok(Date.now() - t0 >= 8);
  const reply = bridge.calls.find((c) => c.name === "harperReply");
  assert.equal(reply.args[0], 1);
  const text = "We recieve it. This is is very unique.";
  const found = JSON.parse(reply.args[1]).map((f) => [f.kind, text.slice(f.start, f.end), f.suggestions]);
  assert.deepEqual(found, [["Spelling", "recieve", ["receive"]], ["Repetition", "is is", ["is"]], ["Enhancement", "very unique", ["unique"]]]);
  assert.deepEqual(mockHarperLint(""), []);
});

test("the Harper signals exist on the mock bridge and are optional on the interface", () => {
  const bridge = createMockBridge();
  for (const s of ["harperLoad", "harperLint", "harperImportWords"]) assert.equal(typeof bridge[s].connect, "function", s);
  for (const s of ["harperReady", "harperReply", "harperFailed"]) assert.equal(typeof bridge[s], "function", s);
  assert.doesNotThrow(() => wireHarperBridge({}, fakeBackend()), "an older host without the signals is simply never asked");
});

// ---------------------------------------------------------------- the real engine (only where bin/fetch-harper has run)
const WASM = new URL("../dist/harper/harper_wasm_bg.wasm", import.meta.url);
test("real LocalLinter: findings are UTF-16 spans (an emoji before the word shifts them by two)", { skip: !existsSync(WASM) && "dist/harper/harper_wasm_bg.wasm is not there (run bin/fetch-harper)" }, async () => {
  const { LocalLinter, Dialect } = await import("harper.js");
  const { binary } = await import("harper.js/binary");
  const linter = new LocalLinter({ binary, dialect: Dialect.Australian });
  await linter.setup();
  const text = "\u{1F600} We recieve the the mail. Colour is fine.";
  const found = await lintWith(linter, text);
  const word = found.find((f) => f.kind === "Spelling");
  assert.ok(word, "recieve is found");
  assert.equal(text.slice(word.start, word.end), "recieve");
  assert.equal(word.start, text.indexOf("recieve"));
  assert.equal(word.start, 6, "the emoji is two UTF-16 units, plus a space and \"We \"");
  assert.ok(word.suggestions.includes("receive"));
  const repeat = found.find((f) => text.slice(f.start, f.end) === "the the");
  assert.ok(repeat, "the doubled word is found: " + JSON.stringify(found.map((f) => [f.kind, text.slice(f.start, f.end)])));
  assert.ok(!found.some((f) => /Colour/.test(text.slice(f.start, f.end))), "Australian spelling is not flagged");
  // through the service, with the real engine behind it
  const bridge = createMockBridge();
  wireHarperBridge(bridge, {
    load: async () => ({ lint: (t) => lintWith(linter, t), importWords: (w) => linter.importWords(w) }),
    version: async () => "real",
  });
  bridge.emit("harperLoad", "Australian");
  bridge.emit("harperLint", 1, text);
  await new Promise((r) => setTimeout(r, 300));
  const reply = bridge.calls.find((c) => c.name === "harperReply");
  assert.ok(reply, "answered");
  assert.deepEqual(JSON.parse(reply.args[1]), found);
});

// ---- Cycle 146: an engine installed after the page loaded one -------------------------------------------------------
/** A backend whose served version can change between loads, as after Help > Check for Writing Checker Updates. */
function updatingBackend(initial = "2.10.0") {
  const state = { served: initial, loads: [], resets: 0 };
  return {
    state,
    async load(dialect) {
      const version = state.served; // what the page reads when it starts loading
      state.loads.push(`${dialect}@${version}`);
      return { version, lint: async (text) => [{ start: 0, end: text.length, kind: "Spelling", message: version, suggestions: [] }], importWords: async () => {} };
    },
    version: async () => state.served,
    reset() { state.resets++; },
  };
}

test("harperLoad after an update: the version is read again, a fresh engine loads from it, and its version is announced", async () => {
  const bridge = createMockBridge();
  const backend = updatingBackend("2.10.0");
  wireHarperBridge(bridge, backend);
  const ready = () => bridge.calls.filter((c) => c.name === "harperReady").map((c) => c.args[0]);
  bridge.emit("harperLoad", "Australian");
  await tick();
  assert.deepEqual(ready(), ["2.10.0"]);
  // nothing new is served: the same engine says it is ready again, no second load
  bridge.emit("harperLoad", "Australian");
  await tick();
  assert.deepEqual(ready(), ["2.10.0", "2.10.0"]);
  assert.deepEqual(backend.state.loads, ["Australian@2.10.0"]);
  assert.equal(backend.state.resets, 0);
  // an update is installed: the same harperLoad now means a new engine
  backend.state.served = "2.12.0";
  bridge.emit("harperLoad", "Australian");
  await tick();
  assert.deepEqual(ready(), ["2.10.0", "2.10.0", "2.12.0"]);
  assert.deepEqual(backend.state.loads, ["Australian@2.10.0", "Australian@2.12.0"]);
  assert.equal(backend.state.resets, 1, "the shared engine was dropped so the new import is not the old module");
  bridge.emit("harperLint", 5, "recieve");
  await tick();
  const reply = bridge.calls.find((c) => c.name === "harperReply" && c.args[0] === 5);
  assert.equal(JSON.parse(reply.args[1])[0].message, "2.12.0", "lints are answered by the new engine");
  // rolled back: the bundled version is served again and loads again
  backend.state.served = "2.10.0";
  bridge.emit("harperLoad", "Australian");
  await tick();
  assert.deepEqual(ready().slice(-1), ["2.10.0"]);
  assert.equal(backend.state.loads.length, 3);
});

test("a harperLoad that arrives while the engine is still loading is checked once it has loaded", async () => {
  const bridge = createMockBridge();
  const backend = updatingBackend("2.10.0");
  const first = backend.load.bind(backend);
  const gate = deferred();
  let calls = 0;
  backend.load = async (d) => { const engine = await first(d); if (calls++ === 0) await gate.promise; return engine; };
  wireHarperBridge(bridge, backend);
  bridge.emit("harperLoad", "Australian");
  await tick();
  backend.state.served = "2.12.0"; // installed while the old one was loading
  bridge.emit("harperLoad", "Australian"); // the host reloads
  gate.resolve();
  await tick(); await tick();
  const ready = bridge.calls.filter((c) => c.name === "harperReady").map((c) => c.args[0]);
  assert.deepEqual(ready, ["2.12.0"], "only the version that is actually loaded is announced");
  assert.deepEqual(backend.state.loads, ["Australian@2.10.0", "Australian@2.12.0"]);
});
