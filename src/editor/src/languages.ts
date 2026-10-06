// Curated set of languages highlighted inside fenced code blocks (```lang).
//
// Why not `@codemirror/language-data`: it describes every language with a lazy `import()`, but the
// editor ships as a single esbuild IIFE (no code splitting), so every grammar ends up inlined
// (+1.3 MB and a much slower page load). Here each language is imported explicitly, so only the
// ones listed below are in the bundle. Unknown info strings resolve to `null` and render as plain
// code (no highlight, no error).
//
// `LanguageDescription.matchLanguageName` compares only against `alias` (the name is NOT checked),
// so `lang()` adds the lowercase name to the aliases. Aliases must be lowercase. Grammars are built
// lazily in `load` (the code is bundled either way) so that opening the page does not pay for
// constructing ~40 parsers up front.
import { LanguageDescription, LanguageSupport, StreamLanguage, StreamParser } from "@codemirror/language";
import { javascript } from "@codemirror/lang-javascript";
import { python } from "@codemirror/lang-python";
import { cpp } from "@codemirror/lang-cpp";
import { java } from "@codemirror/lang-java";
import { go } from "@codemirror/lang-go";
import { rust } from "@codemirror/lang-rust";
import { html } from "@codemirror/lang-html";
import { css } from "@codemirror/lang-css";
import { json } from "@codemirror/lang-json";
import { xml } from "@codemirror/lang-xml";
import { sql } from "@codemirror/lang-sql";
import { yaml } from "@codemirror/lang-yaml";
import { markdown } from "@codemirror/lang-markdown";
import { php } from "@codemirror/lang-php";
import { shell } from "@codemirror/legacy-modes/mode/shell";
import { swift } from "@codemirror/legacy-modes/mode/swift";
import { ruby } from "@codemirror/legacy-modes/mode/ruby";
import { toml } from "@codemirror/legacy-modes/mode/toml";
import { dockerFile } from "@codemirror/legacy-modes/mode/dockerfile";
import { diff } from "@codemirror/legacy-modes/mode/diff";
import { lua } from "@codemirror/legacy-modes/mode/lua";
import { r } from "@codemirror/legacy-modes/mode/r";
import { perl } from "@codemirror/legacy-modes/mode/perl";
import { haskell } from "@codemirror/legacy-modes/mode/haskell";
import { clojure } from "@codemirror/legacy-modes/mode/clojure";
import { powerShell } from "@codemirror/legacy-modes/mode/powershell";
import { nginx } from "@codemirror/legacy-modes/mode/nginx";
import { properties } from "@codemirror/legacy-modes/mode/properties";
import { csharp, dart, kotlin, objectiveC, scala } from "@codemirror/legacy-modes/mode/clike";

function lang(name: string, alias: string[], extensions: string[], load: () => LanguageSupport): LanguageDescription {
  return LanguageDescription.of({
    name,
    alias: [name.toLowerCase(), ...alias],
    extensions,
    load: async () => load(),
  });
}

/** A legacy (CodeMirror 5 style) stream parser as a language. */
function legacy(name: string, alias: string[], extensions: string[], parser: StreamParser<unknown>): LanguageDescription {
  return lang(name, alias, extensions, () => new LanguageSupport(StreamLanguage.define(parser)));
}

/** Pass to `markdown({ codeLanguages })`. */
export const fenceLanguages: LanguageDescription[] = [
  lang("JavaScript", ["js", "mjs", "cjs"], ["js", "mjs", "cjs"], () => javascript()),
  lang("TypeScript", ["ts", "mts", "cts"], ["ts", "mts", "cts"], () => javascript({ typescript: true })),
  lang("JSX", [], ["jsx"], () => javascript({ jsx: true })),
  lang("TSX", [], ["tsx"], () => javascript({ jsx: true, typescript: true })),
  lang("Python", ["py", "python3", "py3"], ["py", "pyw"], () => python()),
  lang("C", ["h"], ["c", "h"], () => cpp()),
  lang("C++", ["cpp", "cc", "cxx", "hpp", "cplusplus"], ["cpp", "cc", "cxx", "hpp", "hh"], () => cpp()),
  legacy("Objective-C", ["objc", "obj-c", "objectivec", "mm"], ["m", "mm"], objectiveC),
  lang("Java", [], ["java"], () => java()),
  lang("Go", ["golang"], ["go"], () => go()),
  lang("Rust", ["rs"], ["rs"], () => rust()),
  lang("HTML", ["htm", "xhtml", "vue", "svelte"], ["html", "htm", "vue", "svelte"], () => html()),
  lang("CSS", [], ["css"], () => css()),
  lang("JSON", ["jsonc", "json5", "jsonl", "geojson"], ["json", "jsonc", "json5"], () => json()),
  lang("XML", ["svg", "xsl", "xslt", "plist"], ["xml", "svg", "xsl", "plist"], () => xml()),
  lang("SQL", ["mysql", "postgres", "postgresql", "pgsql", "sqlite", "plsql", "tsql"], ["sql"], () => sql()),
  lang("YAML", ["yml"], ["yaml", "yml"], () => yaml()),
  lang("Markdown", ["md", "mdx", "commonmark"], ["md", "markdown", "mdx"], () => markdown()),
  lang("PHP", [], ["php"], () => php()),
  legacy("Shell", ["sh", "bash", "zsh", "shell-session", "console", "ksh", "fish"], ["sh", "bash", "zsh"], shell),
  legacy("Swift", [], ["swift"], swift),
  legacy("Kotlin", ["kt", "kts"], ["kt", "kts"], kotlin),
  legacy("Ruby", ["rb", "gemfile", "rakefile"], ["rb", "gemspec"], ruby),
  legacy("TOML", [], ["toml"], toml),
  legacy("Dockerfile", ["docker", "containerfile"], ["dockerfile"], dockerFile),
  // legacy-modes has no Makefile grammar; the shell grammar (comments, strings, $VAR) is close enough.
  legacy("Makefile", ["make", "mk", "gnumake"], ["mk"], shell),
  legacy("Diff", ["patch", "udiff"], ["diff", "patch"], diff),
  legacy("Lua", [], ["lua"], lua),
  legacy("R", ["rscript"], ["r"], r),
  legacy("Scala", [], ["scala", "sc"], scala),
  legacy("Perl", ["pl", "pm"], ["pl", "pm"], perl),
  legacy("Haskell", ["hs"], ["hs"], haskell),
  legacy("Clojure", ["clj", "cljs", "edn"], ["clj", "cljs", "edn"], clojure),
  legacy("PowerShell", ["ps1", "pwsh", "ps"], ["ps1", "psm1"], powerShell),
  legacy("Nginx", ["nginxconf"], ["conf"], nginx),
  legacy("Properties", ["ini", "dotenv", "editorconfig"], ["properties", "ini", "cfg", "env"], properties),
  legacy("C#", ["cs", "csharp", "dotnet"], ["cs"], csharp),
  legacy("Dart", [], ["dart"], dart),
];
