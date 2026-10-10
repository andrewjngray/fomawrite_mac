// Harper, the offline grammar engine (Automattic, Apache-2.0), loaded on demand
// as an ES module from qrc:/editor/dist/harper/ (fetched by bin/fetch-harper
// from the release pinned in harper.lock.json; the WebAssembly binary sits next
// to it). The engine runs on the page's own thread (LocalLinter): one paragraph
// lints in a few milliseconds, which is cheaper than a worker round trip.
//
// Spans are UTF-16 offsets (JavaScript string indices), the same units the
// host and CodeMirror use.
//
// The page is the host's lint service (README "Harper"): wireHarperBridge()
// connects the host's harperLoad / harperLint / harperImportWords signals to the
// engine and answers through the harperReady / harperReply / harperFailed slots.
// The queue, ready and token rules live in HarperService, which takes the engine
// as a parameter so node tests run it against a fake.

import type { Signal } from "./bridge";
import { mockHarperLint } from "./bridge";

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
  clearWords(): Promise<void>;
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
// a downloaded update first, else the bundled copy) under a versioned path,
// fomawrite://harper/<version>/<file>, where <version> is what
// fomawrite://harper/VERSION says now. ES module maps are keyed by URL, so an
// engine installed later (Help -> Check for Writing Checker Updates) loads from
// a URL this page has never imported, and the next harperLoad picks it up. The
// dev server has no scheme, so there the files are read next to the bundle. Not
// literals in import(): esbuild must leave the specifiers to the browser.
const inApp = typeof location !== "undefined" && location.protocol === "qrc:";

/** Where the engine files for `version` live (a folder URL ending in "/"). */
export function engineBase(version: string): string {
  return inApp ? `fomawrite://harper/${version}/` : "./harper/";
}

/** The version the host serves right now (read fresh every time; the scheme says no-store), or "unknown". */
export async function readServedVersion(): Promise<string> {
  try {
    const res = await fetch(inApp ? "fomawrite://harper/VERSION" : "./harper/VERSION", { cache: "no-store" });
    if (!res.ok) return UNKNOWN_VERSION;
    return (await res.text()).trim() || UNKNOWN_VERSION;
  } catch {
    return UNKNOWN_VERSION;
  }
}

type Loaded = { linter: HarperLinter; module: HarperModule; version: string };
let linterPromise: Promise<Loaded> | null = null;
let currentDialect: HarperDialect = "Australian";

declare global { interface Window { __harperStage?: string } } // loader stage, read by the Qt test
async function load(dialect: HarperDialect): Promise<Loaded> {
  window.__harperStage = "reading version";
  const version = await readServedVersion();
  if (inApp && version === UNKNOWN_VERSION) throw new Error("Harper's version could not be read from the app");
  const base = engineBase(version);
  const moduleUrl = base + "index.js", binaryUrl = base + "binary.js";
  window.__harperStage = "importing";
  const [module, binaryModule] = await Promise.all([import(moduleUrl) as Promise<HarperModule>, import(binaryUrl) as Promise<{ binary: unknown }>]);
  window.__harperStage = "constructing";
  const linter = new module.LocalLinter({ binary: binaryModule.binary, dialect: module.Dialect[dialect] });
  window.__harperStage = "setup";
  await linter.setup();
  window.__harperStage = "ready";
  return { linter, module, version };
}

/** The shared engine for `dialect` (a different dialect replaces it). Resolves once it can lint. */
function ensureLinter(dialect: HarperDialect): Promise<Loaded> {
  if (!linterPromise || dialect !== currentDialect) {
    currentDialect = dialect;
    const p = load(dialect);
    linterPromise = p;
    p.catch(() => { if (linterPromise === p) linterPromise = null; }); // a failed load is retried by the next request
  }
  return linterPromise;
}

/** Forget the loaded engine: the next request reads VERSION again and imports whatever the host serves now. */
export function resetHarperEngine(): void {
  linterPromise = null;
}

/** Start loading the engine (idempotent). Resolves once it can lint. */
export function harperReady(dialect: HarperDialect = currentDialect): Promise<void> {
  return ensureLinter(dialect).then(() => undefined);
}

export async function lintWith(linter: HarperLinter, text: string): Promise<HarperFinding[]> {
  const lints = await linter.lint(text);
  return lints.map((lint) => {
    const span = lint.span();
    return { start: span.start, end: span.end, kind: lint.lint_kind(), message: lint.message(), suggestions: lint.suggestions().map((s) => s.get_replacement_text()) };
  });
}

/** Lint `text` with the engine (loading it first if needed). */
export async function harperLint(text: string, dialect: HarperDialect = currentDialect): Promise<HarperFinding[]> {
  const { linter } = await ensureLinter(dialect);
  return lintWith(linter, text);
}

/** Words the writer taught the checker (Learn Spelling). */
export async function harperImportWords(words: string[]): Promise<void> {
  const { linter } = await ensureLinter(currentDialect);
  // harper.js importWords appends; the host sends the whole list, so clear first.
  await linter.clearWords();
  await linter.importWords(words);
}

declare global {
  interface Window { fomawriteHarper?: { ready: typeof harperReady; lint: typeof harperLint; importWords: typeof harperImportWords } }
}
export function exposeHarperForTests(): void {
  window.fomawriteHarper = { ready: harperReady, lint: harperLint, importWords: harperImportWords };
}

// ---------------------------------------------------------------- the host's lint service

/** One loaded engine, as the service sees it. */
export interface LoadedEngine {
  lint(text: string): Promise<HarperFinding[]>;
  importWords(words: string[]): Promise<void>;
  /** The version this engine was loaded as, when the backend knows it. */
  version?: string;
}
/**
 * Where engines come from. `load` rejects when the engine cannot be loaded; `version` (the version the host serves
 * *now*, read fresh) never rejects; `reset` makes the next `load` build a new engine instead of returning the shared one.
 */
export interface HarperBackend {
  load(dialect: HarperDialect): Promise<LoadedEngine>;
  version(): Promise<string>;
  reset?(): void;
}

/** The slots and signals the service uses (the host side is EditorBridge; all optional so an older host just gets nothing). */
export interface HarperBridge {
  log?: (message: string) => void;
  harperReady?: (version: string) => void;
  harperReply?: (token: number, lintsJson: string) => void;
  harperFailed?: (token: number, error: string) => void;
  harperLoad?: Signal<[string]>;
  harperLint?: Signal<[number, string]>;
  harperImportWords?: Signal<[string]>;
}

const DIALECTS: readonly HarperDialect[] = ["American", "British", "Australian", "Canadian"];
/** The dialect used when a lint arrives before any harperLoad (the app's default). */
export const DEFAULT_DIALECT: HarperDialect = "Australian";
/** Reported when `fomawrite://harper/VERSION` cannot be read. */
export const UNKNOWN_VERSION = "unknown";

const errorText = (e: unknown): string => (e instanceof Error ? e.message || String(e) : String(e));

/**
 * Queue, ready and token rules between the host and the engine:
 * - `load(dialect)` starts a load; a different dialect replaces the engine (a load still running is abandoned); the same
 *   dialect while loading or loaded starts nothing (once loaded it answers `harperReady` again, so a host that waits for it
 *   never hangs);
 * - `lint(token, text)` with empty text is answered `[]` at once; otherwise it is queued and answered in order once the
 *   engine is ready; a reload (new dialect) keeps the queue. A token still queued or being linted is not queued twice, so
 *   one request is answered exactly once, whatever happens (a reload, a failure) in between;
 * - a lint that throws answers `harperFailed(token, ...)` for that token only; a load that fails answers
 *   `harperFailed(-1, ...)` once and fails every queued token; lints arriving after a failure fail at once until the
 *   next `load` retries;
 * - `importWords(list)` is kept and imported into the engine before any later lint, and again after every reload.
 */
export class HarperService {
  private dialect: HarperDialect | null = null;
  private generation = 0;
  private engine: LoadedEngine | null = null;
  private failure: string | null = null;
  private queue: { token: number; text: string }[] = [];
  private pending = new Set<number>();
  private words: string[] | null = null;
  private wordsVersion = 0;
  private importedVersion = 0;
  private pumping = false;
  private cachedVersion: string | null = null;
  private reloadRequested = false;

  constructor(private bridge: HarperBridge, private backend: HarperBackend) {}

  get ready(): boolean { return this.engine !== null; }
  get queued(): number { return this.queue.length; }

  private log(message: string): void {
    try { this.bridge.log?.(message); } catch { /* ignore */ }
  }
  private send(what: "ready" | "reply" | "failed", a: string | number, b?: string): void {
    try {
      if (what === "ready") this.bridge.harperReady?.(a as string);
      else if (what === "reply") this.bridge.harperReply?.(a as number, b!);
      else this.bridge.harperFailed?.(a as number, b!);
    } catch (e) {
      this.log(`harper ${what} failed: ${errorText(e)}`);
    }
  }

  /** `harperLoad(dialect)`. An unknown dialect name loads the default. */
  load(dialect: string): void {
    const d = (DIALECTS as readonly string[]).includes(dialect) ? (dialect as HarperDialect) : DEFAULT_DIALECT;
    if (d === this.dialect && this.failure === null) {
      // Already loaded: the host may have installed a newer engine since (it sends harperLoad again to say so), so look
      // at what is served now: a different version is a fresh engine, the same one is just announced again.
      if (this.engine) void this.recheck(this.generation);
      else this.reloadRequested = true; // still loading: look again once it has loaded
      return;
    }
    this.dialect = d;
    const generation = ++this.generation;
    this.engine = null;
    this.failure = null;
    this.reloadRequested = false;
    void this.start(d, generation);
  }

  /** An engine is loaded and the host asked again: a newer version served means load it, else announce the one we have. */
  private async recheck(generation: number): Promise<void> {
    let served: string;
    try { served = await this.backend.version(); } catch { served = UNKNOWN_VERSION; }
    if (generation !== this.generation || !this.engine) return;
    if (served !== UNKNOWN_VERSION && this.cachedVersion !== null && this.cachedVersion !== UNKNOWN_VERSION && served !== this.cachedVersion) {
      this.backend.reset?.();
      this.cachedVersion = null;
      this.engine = null;
      this.reloadRequested = false;
      void this.start(this.dialect!, ++this.generation); // queued lints wait for the new engine
      return;
    }
    await this.announceReady(generation);
  }

  private async start(dialect: HarperDialect, generation: number): Promise<void> {
    let engine: LoadedEngine;
    try {
      engine = await this.backend.load(dialect);
      this.importedVersion = this.wordsVersion;
      if (this.words && this.words.length > 0) {
        try { await engine.importWords(this.words); } catch (e) { this.log(`harperImportWords failed: ${errorText(e)}`); }
      }
    } catch (e) {
      if (generation !== this.generation) return; // abandoned: the newer load answers
      this.failure = errorText(e) || "Harper failed to load";
      this.send("failed", -1, this.failure);
      this.failQueued();
      return;
    }
    if (generation !== this.generation) return; // a newer load replaced this one
    if (engine.version) this.cachedVersion = engine.version; // what this engine is, not what an earlier one was
    this.engine = engine;
    if (this.reloadRequested) { this.reloadRequested = false; await this.recheck(generation); } // asked again while loading
    else await this.announceReady(generation);
    if (generation === this.generation) void this.pump();
  }

  private async announceReady(generation: number): Promise<void> {
    if (this.cachedVersion === null) {
      try { this.cachedVersion = (await this.backend.version()) || UNKNOWN_VERSION; } catch { this.cachedVersion = UNKNOWN_VERSION; }
    }
    if (generation !== this.generation || !this.engine) return;
    this.send("ready", this.cachedVersion);
  }

  /** `harperLint(token, text)`. */
  lint(token: number, text: string): void {
    if (this.pending.has(token)) return; // already queued or being linted: it will be answered once
    if (text === "") { this.send("reply", token, "[]"); return; }
    if (this.failure !== null) { this.send("failed", token, this.failure); return; }
    this.pending.add(token);
    this.queue.push({ token, text });
    if (this.dialect === null) this.load(DEFAULT_DIALECT); // a lint before any harperLoad starts the default engine
    else void this.pump();
  }

  /** `harperImportWords(wordsJson)`: a JSON array of words; anything else is ignored. */
  importWords(wordsJson: string): void {
    let list: unknown;
    try { list = JSON.parse(wordsJson); } catch { this.log("harperImportWords: not JSON"); return; }
    if (!Array.isArray(list)) { this.log("harperImportWords: not an array"); return; }
    this.words = list.filter((w): w is string => typeof w === "string" && w.trim() !== "");
    this.wordsVersion++;
    void this.pump();
  }

  private failQueued(): void {
    const failed = this.queue; this.queue = [];
    for (const { token } of failed) { this.pending.delete(token); this.send("failed", token, this.failure ?? "Harper failed to load"); }
  }

  /** Imports pending words, then answers queued lints in order, while an engine is ready. */
  private async pump(): Promise<void> {
    if (this.pumping) return;
    this.pumping = true;
    try {
      while (this.engine) {
        const engine = this.engine;
        if (this.importedVersion !== this.wordsVersion) {
          this.importedVersion = this.wordsVersion;
          try { await engine.importWords(this.words ?? []); } catch (e) { this.log(`harperImportWords failed: ${errorText(e)}`); }
          continue; // the engine (or the list) may have changed meanwhile
        }
        const job = this.queue.shift();
        if (!job) break;
        try {
          const findings = await engine.lint(job.text);
          this.pending.delete(job.token);
          this.send("reply", job.token, JSON.stringify(findings));
        } catch (e) {
          this.pending.delete(job.token);
          this.send("failed", job.token, errorText(e) || "lint failed");
        }
      }
    } finally {
      this.pumping = false;
    }
  }
}

/** The engine in this page, behind the shared loader above. */
export function realHarperBackend(): HarperBackend {
  return {
    async load(dialect) {
      const { linter, version } = await ensureLinter(dialect);
      return { lint: (text) => lintWith(linter, text), importWords: async (words) => { await linter.clearWords(); await linter.importWords(words); }, version };
    },
    version: readServedVersion,
    reset: resetHarperEngine,
  };
}

/** A stand-in engine for the dev server: no WebAssembly, version "mock", loads in 10 ms, fixed findings. */
export function mockHarperBackend(): HarperBackend {
  return {
    load: () => new Promise((resolve) => setTimeout(() => resolve({ lint: async (text) => mockHarperLint(text), importWords: async () => {} }), 10)),
    version: async () => "mock",
  };
}

/** Connect the host's three Harper signals to a service; `backend` defaults to the real engine. */
export function wireHarperBridge(bridge: HarperBridge, backend: HarperBackend = realHarperBackend()): HarperService {
  const service = new HarperService(bridge, backend);
  const guarded = <A extends unknown[]>(name: string, fn: (...a: A) => void) => (...a: A) => {
    try { fn(...a); } catch (e) { try { bridge.log?.(`${name} failed: ${errorText(e)}`); } catch { /* ignore */ } }
  };
  bridge.harperLoad?.connect?.(guarded("harperLoad", (dialect: string) => service.load(String(dialect))));
  bridge.harperLint?.connect?.(guarded("harperLint", (token: number, text: string) => service.lint(token, String(text ?? ""))));
  bridge.harperImportWords?.connect?.(guarded("harperImportWords", (json: string) => service.importWords(String(json))));
  return service;
}
