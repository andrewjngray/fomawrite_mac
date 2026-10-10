// Harper, the offline grammar engine (Automattic, Apache-2.0), loaded on demand
// as an ES module from qrc:/editor/dist/harper/ (fetched by bin/fetch-harper
// from the release pinned in harper.lock.json; the WebAssembly binary sits next
// to it). The engine runs on the page's own thread (LocalLinter): one paragraph
// lints in a few milliseconds, which is cheaper than a worker round trip.
//
// Spans are UTF-16 offsets (JavaScript string indices), the same units the
// host and CodeMirror use.

export interface HarperFinding {
  start: number;
  end: number;
  kind: string; // Harper's lint kind, e.g. Spelling, Agreement, Repetition, WordChoice, Style
  message: string;
  suggestions: string[];
}

export type HarperDialect = "American" | "British" | "Australian" | "Canadian";

interface HarperModule {
  LocalLinter: new (init: { binary: unknown; dialect?: number }) => HarperLinter;
  Dialect: Record<string, number>;
}
interface HarperLinter {
  setup(): Promise<void>;
  lint(text: string): Promise<HarperLint[]>;
  importWords(words: string[]): Promise<void>;
  getLintConfig(): Promise<Record<string, boolean | undefined>>;
  setLintConfig(config: Record<string, boolean | undefined>): Promise<void>;
}
interface HarperLint {
  span(): { start: number; end: number };
  lint_kind(): string;
  message(): string;
  suggestions(): { get_replacement_text(): string }[];
}

// In the app the engine is served by the fomawrite: scheme (src/harperscheme.cpp:
// a downloaded update first, else the bundled copy). The dev server has no
// scheme, so there the files are read next to the bundle. Not literals in
// import(): esbuild must leave the specifiers to the browser.
const inApp = typeof location !== "undefined" && location.protocol === "qrc:";
const MODULE_URL = inApp ? "fomawrite://harper/index.js" : "./harper/index.js";
const BINARY_URL = inApp ? "fomawrite://harper/binary.js" : "./harper/binary.js";

let linterPromise: Promise<{ linter: HarperLinter; module: HarperModule }> | null = null;
let currentDialect: HarperDialect = "Australian";

declare global { interface Window { __harperStage?: string } } // loader stage, read by the Qt test
async function load(dialect: HarperDialect): Promise<{ linter: HarperLinter; module: HarperModule }> {
  const moduleUrl = MODULE_URL, binaryUrl = BINARY_URL;
  window.__harperStage = "importing";
  const [module, binaryModule] = await Promise.all([import(moduleUrl) as Promise<HarperModule>, import(binaryUrl) as Promise<{ binary: unknown }>]);
  window.__harperStage = "constructing";
  const linter = new module.LocalLinter({ binary: binaryModule.binary, dialect: module.Dialect[dialect] });
  window.__harperStage = "setup";
  await linter.setup();
  window.__harperStage = "ready";
  return { linter, module };
}

/** Start loading the engine (idempotent). Resolves once it can lint. */
export function harperReady(dialect: HarperDialect = currentDialect): Promise<void> {
  if (!linterPromise || dialect !== currentDialect) {
    currentDialect = dialect;
    linterPromise = load(dialect);
  }
  return linterPromise.then(() => undefined);
}

/** Lint `text` with the engine (loading it first if needed). */
export async function harperLint(text: string, dialect: HarperDialect = currentDialect): Promise<HarperFinding[]> {
  await harperReady(dialect);
  const { linter } = await linterPromise!;
  const lints = await linter.lint(text);
  return lints.map((lint) => {
    const span = lint.span();
    return { start: span.start, end: span.end, kind: lint.lint_kind(), message: lint.message(), suggestions: lint.suggestions().map((s) => s.get_replacement_text()) };
  });
}

/** Words the writer taught the checker (Learn Spelling). */
export async function harperImportWords(words: string[]): Promise<void> {
  await harperReady();
  const { linter } = await linterPromise!;
  await linter.importWords(words);
}

declare global {
  interface Window { fomawriteHarper?: { ready: typeof harperReady; lint: typeof harperLint; importWords: typeof harperImportWords } }
}
export function exposeHarperForTests(): void {
  window.fomawriteHarper = { ready: harperReady, lint: harperLint, importWords: harperImportWords };
}
