import test from "node:test";
import assert from "node:assert/strict";
import { EditorState, EditorSelection } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { LanguageDescription, ensureSyntaxTree } from "@codemirror/language";
import { fenceLanguages as curated } from "../src/languages.ts";
import { fenceLanguages, buildBlocks } from "../src/blocks.ts";

const match = (alias) => LanguageDescription.matchLanguageName(fenceLanguages, alias, true);

test("blocks.ts re-exports the curated list from languages.ts", () => {
  assert.equal(fenceLanguages, curated);
});

test("common info strings resolve to the right language", () => {
  const expected = {
    js: "JavaScript", javascript: "JavaScript", ts: "TypeScript", typescript: "TypeScript", jsx: "JSX", tsx: "TSX",
    python: "Python", py: "Python", c: "C", "c++": "C++", cpp: "C++", objc: "Objective-C", java: "Java", go: "Go",
    rust: "Rust", rs: "Rust", html: "HTML", vue: "HTML", css: "CSS", json: "JSON", jsonc: "JSON", xml: "XML",
    sql: "SQL", yaml: "YAML", yml: "YAML", markdown: "Markdown", md: "Markdown", php: "PHP",
    bash: "Shell", sh: "Shell", zsh: "Shell", shell: "Shell", swift: "Swift", kotlin: "Kotlin", kt: "Kotlin",
    ruby: "Ruby", rb: "Ruby", toml: "TOML", dockerfile: "Dockerfile", makefile: "Makefile", diff: "Diff",
    lua: "Lua", r: "R", scala: "Scala", perl: "Perl", haskell: "Haskell", clojure: "Clojure",
    powershell: "PowerShell", ps1: "PowerShell", nginx: "Nginx", ini: "Properties", properties: "Properties",
    cs: "C#", csharp: "C#", dart: "Dart",
  };
  for (const [alias, name] of Object.entries(expected)) assert.equal(match(alias)?.name, name, alias);
});

test("matching is case-insensitive", () => {
  assert.equal(match("JavaScript")?.name, "JavaScript");
  assert.equal(match("Python")?.name, "Python");
  assert.equal(match("YML")?.name, "YAML");
});

test("unknown and plain-text info strings resolve to null", () => {
  for (const alias of ["not-a-language", "plaintext", "text", "txt", "none", "klingon", ""])
    assert.equal(match(alias), null, JSON.stringify(alias));
});

test("every language description loads and has a parser", async () => {
  for (const d of fenceLanguages) {
    const support = await d.load();
    assert.ok(support.language, d.name);
  }
});

test("a fence with an unknown language stays plain code (no nested parse, no error)", async () => {
  for (const d of fenceLanguages) await d.load();
  const doc = "```klingon\nqapla' = 1 + 2\n```\n";
  const state = EditorState.create({
    doc,
    selection: EditorSelection.single(doc.length),
    extensions: [markdown({ base: markdownLanguage, codeLanguages: fenceLanguages })],
  });
  const tree = ensureSyntaxTree(state, doc.length, 1000);
  assert.equal(tree.resolveInner(doc.indexOf("qapla") + 1, 1).name, "CodeText", "no nested language tree is mounted");
  assert.ok(tree.resolve(0, 1).name === "FencedCode" || tree.resolve(0, 1).name === "CodeMark");
  assert.doesNotThrow(() => buildBlocks(state));

  // contrast: a known language does get a nested tree
  const js = "```js\nconst a = 1\n```\n";
  const jsState = EditorState.create({ doc: js, extensions: [markdown({ base: markdownLanguage, codeLanguages: fenceLanguages })] });
  const jsTree = ensureSyntaxTree(jsState, js.length, 1000);
  assert.notEqual(jsTree.resolveInner(js.indexOf("const") + 1, 1).name, "CodeText", "js fence is parsed as JavaScript");
});
