# Fomawrite — Independent Review (Claude, 6 October 2026)

Answers [DEVELOPER_HANDOFF.md](DEVELOPER_HANDOFF.md) §10's review brief. Baseline reviewed: `master` @ `559a1cd`
(Cycle 130, `0.3.0-dev26`). Read-only: no source was modified. Three parallel code reviews (architecture,
publishing lifecycle, data integrity) plus my own build/test run and strategic assessment.

**Evidence standard used here:** *CONFIRMED* = code path traced and, where marked *(probe)*, reproduced by a
small C++ program compiled against the installed Qt 6.11.2. *PLAUSIBLE* = traced but not executed.

---

## 1. Verdict in one paragraph

Fomawrite is a **genuinely substantial, working, honestly-documented app** — ~39k lines of your own code on a
tiny upstream seed, with a careful data-safety core (atomic `QSaveFile` saves, lock-slot crash recovery,
single undo stack, a bounded Source↔Visual mapper that I could not make mis-map an edit), strong security in the
Markdown→Chromium chain, and test evidence that reproduces exactly on your machine today (I ran it: **204 passed,
0 failed, 0 skipped**). But three things need to be said plainly:

1. **It has four silent data-corruption bugs in the load/save pipeline** — the editor itself is safe, but the
   bytes going in and out are not. A user can trust it with their only copy of a document *only if* that file is
   already LF, BOM-less UTF-8 with no no-break spaces. Anything else is rewritten on the first ⌘S with no
   modified flag and no message; a non-UTF-8 file is destroyed outright. These are each ~an afternoon to fix.
   **This is Cycle 131, before anything else.**
2. **Structural debt is now accruing faster than value** (architecture score 5/10). Two ~4k-line god objects,
   test code compiled into `main()`, a 1,261-line function with 90 lambdas, no CI, and a documentation process
   that costs ~5 doc writes per code change. The debt is the *cheap* kind — the seams are visible and the codebase
   contains its own correct pattern (`FileLibrary`) — but it needs a focused structural pass before more features.
3. **It has quietly stopped being a Typora clone.** Fomawrite today is an iA-Writer-style Source editor plus a
   bounded Visual Edit projection plus a read-only Marked-2-style publishing pane with Typora CSS themes. The
   Visual Edit mapper is *by design* "not a Markdown serializer" and cannot grow into Typora's inline WYSIWYG.
   That may be exactly the product you want — but it should be a conscious decision (see §6).

---

## 2. Data integrity — CONFIRMED defects (fix first)

> **Status (Cycle 131, same day):** C1–C6 below are fixed on `master` with byte round-trip regression tests — see [research/cycle-131/README.md](research/cycle-131/README.md). The table is kept as the record of what was found.

The editor core (undo/redo, mapper, atomic save, recovery, unsaved-change prompts) was verified safe. The
defects are all in `Backend::open()` → `QTextDocument` → `toPlainText()` → `saveTo()`.

| # | Sev | Defect | Where | Repro (probe) |
|---|---|---|---|---|
| **C1** | **CRITICAL** | **Non-UTF-8 files silently mangled and written back.** No encoding detection/validation; invalid bytes → U+FFFD; not marked modified; next Save destroys the original. | `backend.cpp:1877-1882`, `:2540` | Latin-1 `café` → opens as `caf�` → Save writes `caf�`. UTF-16LE file → garbage + embedded NULs. |
| **C2** | HIGH | **`toPlainText()` rewrites NBSP (U+00A0)→space and U+2028/2029→`\n` on every save**, never flagged as modified. Affects ordinary prose (French « », "10 km", Word/web pastes). | `backend.cpp:2787` (`currentDocumentText`) used by save/recovery/duplicate | `10 km` → Save → `10 km`. |
| **C3** | HIGH | **CR/CRLF**: (a) CRLF→LF on first save, unflagged; (b) lone-CR files have *all* line breaks deleted; (c) baseline stored normalized while disk is CRLF ⇒ for any CRLF file **autosave is permanently paused, Rename/Move refused, spurious "changed on disk" dialogs**. The mapper's CRLF logic + the CRLF test fixtures are dead code in the live app (`setPlainText` normalizes before they run) → false confidence. | `backend.cpp:1872` (`QIODevice::Text`), `:1884`, comparisons at `:578,2136,2188,2551,3550` | `a\rb\n` → `ab\n`. |
| **C4** | MEDIUM | UTF-8 BOM silently stripped on save. | `backend.cpp:1882`, `:2540` | `EF BB BF 23…` → `23…`. |
| **C5** | HIGH | **Manual ⌘S never checks the on-disk baseline.** Only autosave/another-view do. Scenario: edit → crash → another tool rewrites the file → relaunch recovers → ⌘S **overwrites the newer external version with no prompt**. Watcher is armed after the fact so cannot catch it. | `backend.cpp:2491-2599` (`saveTo`), `:2629-2650` | Extend existing `recoverySnapshotsSurviveProcessExit` fixture. |
| C6 | LOW | `m_lastDocumentText` baseline un-normalized → a no-op `editorTextChanged()` (e.g. Replace matching nothing) marks a clean NBSP/CRLF doc modified. | `backend.cpp:2455` vs `:2367` | — |

**Fixes (all small):** decode with `QStringDecoder` + `hasError()` and refuse/warn (C1); add `canonicalText()` from
`toRawText()` replacing only `ParagraphSeparator` (C2); open without `QIODevice::Text`, keep raw bytes as the
baseline, detect dominant line ending and re-apply on save (C3); remember BOM (C4); re-read disk in `saveTo` and
route a mismatch to `ExternalChangeDialog` with an explicit Overwrite option (C5).
**Required test shape:** *byte round-trip fixtures* — open → (no-op | save) → `QCOMPARE(readAll(), original)`
for Latin-1, UTF-16, CRLF, CR, BOM, NBSP, U+2028. None exist today; every fixture is clean LF/UTF-8.

PLAUSIBLE (lower): O(n²) `appendInline` + double full-document reprojection per Visual keystroke → long pasted
lines can freeze Visual Edit (P1); Source `smartReturn` only counts ``` fences, not `~~~` (P2); recovery slots
silently cap at 100 (P4).

---

## 3. Publishing / preview lifecycle (the incident area)

> **Status (Cycle 132):** A1–A8 and E4 below are addressed on `master` with tests that fail on the previous code — see [research/cycle-132/README.md](research/cycle-132/README.md). The explicit QML state machine and off-thread generation remain open.

**Backend protocol is now sound** — per-request captured identity, content-hash cache, true cancellation
(`QPointer` + generation guards, correct destruction order). I could not construct an A→B→C path where a stale
result lands on the wrong document. **The design around it remains fragile:**

| # | Sev | Finding |
|---|---|---|
| **A1** | HIGH | **Any file change anywhere in the themes folder cancels every in-flight render and restarts the debounce** — even `.DS_Store` after *Open Themes Folder*, editor swap/backup files, or a log — and even when a *basic* (non-folder) style is selected. Any write cadence between ~220 ms and Chromium render time reproduces the Cycle 129 symptom exactly. The fix compared folder *snapshots*; it should compare the *selected theme's sanitized CSS + catalog listing*. (`backendpublishing.inc:137-178`, `backend.cpp:535-542`, `PublishingPreview.qml:279`) |
| **A2** | HIGH | **Theme CSS re-read + base64 of all fonts (up to 64 MiB), full image decode, and SHA-256 over the whole HTML run synchronously on the GUI thread on every debounce tick — *before* the cache is consulted.** Cache hits are not cheap; the pane "hangs". Memoize `PublishingThemes::css`, compute the cache key before generating HTML, move generation off-thread. |
| A3 | MED | A **new Chromium profile + page + renderer process per PDF render** (`publishingpdf.cpp:153-183`). (Observed: your running app has 8 `QtWebEngineProcess` helpers.) Reuse one profile/page per renderer. |
| A4 | MED | **Timeout/failure discards the last good output** (`failRefresh → clearOutput`). A >30 s document can *never* be previewed; any transient failure blanks a good render. Keep last displayed output + banner. |
| A5 | MED | No `onRenderProcessTerminated` in Web mode → renderer crash = silent blank pane. |
| A6 | LOW-MED | Web-mode completion depends on a JS round-trip with two early-return paths that never call `finishRefresh` → stuck Loading until the 30 s deadline → A4 blanks it. |
| A7 | LOW-MED | Watcher silently stops at 511 paths; unsorted `QDirIterator` makes the watched subset unstable (the Cycle 129 churn mechanism, bounded not zero). Watch directories + top-level `*.css` only. |
| A8 | LOW | Every refresh `mkpath`s the themes folder — a user who moves it for backup gets an empty one recreated 200 ms later. |

**Security: strong.** Raw HTML escaped; `javascript:`/`data:` hrefs rejected; images inlined to `data:`; CSS
sanitiser decodes escapes and rejects `\`/`<`/remote `url()`/`@import`; CSP + interceptor + no file-URL access on
the PDF path. Minor: the QML preview `WebEngineView` still has `localContentCanAccessFileUrls: true` and no
interceptor (E4); `OutputCss::load` uses a bypassable regex deny-list (E3); bundled-theme `root` is empty (E5).

**Tests:** cycle124 integration tests are solid; cycle128 component tests are useful but assert implementation
fields (`pendingRequest`, `appliedRevision`…). **Web mode has zero component coverage**, Export Hub consumer
untested, and — as the handoff admits — the Cycle 129 watcher tests pass the old code. A deterministic red/green
test exists: `utimes()` the themes dir 10× at 100 ms on macOS and assert zero `publishingThemesChanged`; and A1's
test (unrelated-file churn with a real preview attached) fails on *current* code.

**Direction:** replace `PublishingPreview.qml`'s ~15 mutable fields and six-callback guard soup with an explicit
state machine (`Idle | Debouncing | Requested | Loading | Displayed | Failed`) and one `activeRequest` record.
Every "stuck in Loading" path found is a callback that returns early with nobody owning completion.

---

## 4. Architecture & maintainability — 5/10

**Earns points:** careful data-safety core; "why" comments; descriptive names; `FileLibrary` and
`WorkspaceCommands` are clean, correctly-factored modules; the acceptance harness verifies the shipped bundle's
embedded QML hashes against the checkout.

**Costs points (all CONFIRMED):**

- **`Backend`** = 3,919 + 444 (`.inc`) lines, 27 `Q_PROPERTY`, 28 signals, ~130 public methods, **~110 member
  variables**, holding **19 distinct responsibilities** (I/O, recovery, nav history, theming incl. a dead Omarchy
  TOML watcher, typography, 20 string-dispatched edit actions, visual bridge, analysis, search, output styles,
  publishing, print, Versions, writing review, authorship, library ops, clipboard, window geometry, and a
  quit-message-bus for QML↔`main.cpp`). The 127→129 incident is the measurable cost: one feature's state in
  three places took three cycles to localize.
- **`backendpublishing.inc`** is `#include`d at `backend.cpp:3455` — a file-length workaround, not a module
  (shared private state, un-testable anonymous-namespace helpers, listed as `DISTFILES`).
- **`Main.qml`** = 3,746 lines, 138 functions, 302 `backend.` refs, 24 dialogs, 11 timers; contains a 508-line
  native menu bar (zero logic), ~800-line source editor, and **Markdown logic that belongs in C++** (link
  escaping, `smartReturn`, `visualDiff` — the surrogate-aware diff that feeds the mapper — and a `LinkSyntax.js`
  grammar duplicated by a C++ regex "kept in sync by hand").
- **Test/acceptance code inside the product.** `documentviewcheck.cpp` (1.5k lines, one 1,261-line function
  with 90 lambdas, `#include`ing 5 `.inc`s *inside the function body*, one of them twice) is in `SOURCES` and
  ships in the app; `main.cpp` `#include`s `../tests/*.inc` inside `main()` under `#ifdef`s. Two of the three
  test modes rebuild the entire WebEngine-linked app into a temp dir.
- **Cycle-numbered test files** (`cycle124-publishing.inc`, `cycle128-publishing.inc`, `cycle125-theme-folder.inc`
  + ~16 inline slots = five places for publishing tests), one ~10k-line test TU, every test links WebEngine.
- **Coupling:** 722 `findChild<>("objectName")` + 97 `invokeMethod` as the primary C++→QML interface; 283 bindings
  to `backend.palette` (a `QVariantMap` rebuilt per theme change); 12 scattered function-local `extern`s for the
  mac bridge (no header); 50 `QSettings()` call sites with no key registry; a 250 ms polling timer for
  `nativeTabInset()`; output styles compared as magic ints (`== 3`, `== 7`).
- **Naming trap confirmed:** `PreviewPane.qml` is instantiated as `visualEditorPane`; `PublishingPreview.qml` as
  `previewPane`; both declare `objectName: "previewPane"`.
- **Dead code:** `saveForClose`/`closeAfterSave` path (handler can never fire), `wordCount` debounce machinery
  with zero bindings, Omarchy watcher, legacy settings kept only to be overridden, `outputHtml` ignoring its
  `QTextDocument` arg.
- **Build:** no CI; Qt unpinned; `fomawrite.pro` append-only (`QT += pdf` twice); `bin/package-mac` is a
  hand-rolled `otool`/`install_name_tool` loop that will break silently on any Homebrew/Qt layout change; no
  notarization path; version string in two places.
- **Docs:** 26 docs + handoff + 133 cycle READMEs; `build-cycles.md` is 231 KB and append-mandated by
  `AGENTS.md`; 14/26 docs call themselves or each other superseded; README asserts two different "current"
  cycles (129 and 130); `research/` is 1.0 GB on disk. No single source of truth.

---

## 5. What I could *not* verify (honest scope limits)

- **Native/physical acceptance.** Your installed Fomawrite was running during review and `build/Fomawrite.app`
  shares its bundle identity, Qt preferences and launch-forwarding, so I did not launch a second copy or run
  `bin/check-document-views`. Physical input, VoiceOver, multi-display, and sustained-use remain your acceptance
  work, as the handoff states.
- **Linux path** — plausibly no longer compiles (nothing exercises it since WebEngine/PDF work).
- **Performance under real load** — A2/A3/P1 are traced, not profiled.

---

## 6. Strategic assessment — what *is* Fomawrite, and where should it go?

The original brief (`potentia_typora/PROJECT-PLAN.md`) was "replicate Typora": a single surface where Markdown
renders in place and syntax markers hide until the cursor enters them. Fomawrite is a different — and
legitimate — product:

| | Typora model | Fomawrite today |
|---|---|---|
| Editing surface | One inline-rendered WYSIWYG surface | **Source** (literal, highlighted) + **Visual Edit** (bounded projection: paragraph/heading/list-item/image/table-row only; everything else `SourceOnly`) |
| Preview | None (it *is* the editor) | Separate read-only Web/PDF publishing pane (Marked 2 / MacDown model) |
| Closest kin | Typora, Obsidian Live Preview | **iA Writer + Marked 2**, with Typora CSS themes bolted on |

The Visual Edit mapper is honest about this (`sourcevisualmapping.h:7-10`: "not a Markdown serializer").
Extending it toward true WYSIWYG means re-implementing a Markdown editor as offset mappings over
`QTextDocument` — a hard ceiling, and each increment ("richer safe lists", "row/column restructuring") gets more
expensive. Three real options:

- **(a) Own the iA-Writer-plus-publishing identity.** Stop expanding Visual Edit; invest in Source typography,
  focus/typewriter, library, export. Fomawrite is already most of the way here and it's a coherent product.
- **(b) Hybrid: put a real inline-preview editor inside the Chromium you already ship.** A CodeMirror 6
  live-preview editor (the Obsidian/Atomic-Editor technique from the Nitro Gravity plan) hosted in a
  `WebEngineView` with a thin bridge to the C++ `DocumentStore`. Reuses the entire Fomawrite shell (library,
  recovery, publishing, themes, menus); replaces only the Visual Edit pane. Markdown stays the source of truth
  in C++. Non-trivial bridge work, but the only route from here to the Typora feel without a rewrite.
- **(c) Nitro Gravity Editor as planned (Electron + CM6), treating Fomawrite as lessons-learned.** Cleanest
  path to Typora parity; discards a working app and 39k lines.

**My recommendation:** don't decide this now. Do Cycles 131–133 below first (they're needed under every
option), then decide (a)/(b)/(c) with a stable base and a working editor you've used for a few more weeks.
If you still want Typora's inline feel, **(b)** is the path I'd take.

---

## 7. Process observations

- The cycle evidence is **honest** — numbers reproduce, failures are recorded rather than hidden, limits are
  stated. That is rarer than it should be and worth preserving.
- The cost is the overhead: per-cycle README + manifest + `build-cycles.md` append + ledger rewrite + README
  rewrite + `.gitignore` carve-out. Replace with: `STATUS.md` (one place for the current build), `CHANGELOG.md`
  (one line per cycle), `ARCHITECTURE.md` (living), `docs/archive/` for the 11 stale docs, and **CI as the
  evidence** instead of hand-recorded manifests.
- Without CI, "204 tests pass" is a claim made by the same agent that wrote the code. A GitHub Actions macOS
  job pinning Qt (via `aqtinstall`) running `./bin/build && ./bin/test` closes that gap and is a half-day job.

---

## 8. Proposed next cycles

Each cycle is bounded, has a test gate, and does not depend on the strategic decision in §6.

### Cycle 131 — Byte-exact persistence (data integrity) · ~1 week
Fix C1–C6. Add the byte round-trip fixture family (Latin-1, UTF-16, CRLF, CR, BOM, NBSP, U+2028, no-final-
newline). Add "manual Save respects disk baseline" test and an Overwrite option to `ExternalChangeDialog`.
Remove the dead CRLF code path in the mapper or make it reachable. **Gate:** every fixture round-trips byte-for-
byte or the save is refused with a visible reason. *Non-negotiable before any feature work.*

### Cycle 132 — Publishing robustness · ~1 week
A1 (compare selected-theme CSS + catalog, not folder snapshot; never bump settings generation for a no-op),
A2 (memoize theme CSS/image validation; cache key before generation), A4/A5/A6 (explicit state machine in
`PublishingPreview.qml`; keep last good output on failure; handle `renderProcessTerminated`), A3 (reuse
profile/page), A7/A8. **Gate:** the two macOS red/green tests (dir-mtime churn; unrelated-file churn with a real
preview) + first WebEngine component test for Web mode.

### Cycle 133 — Structure & CI · ~2 weeks
In safety order: (1) GitHub Actions macOS CI with pinned Qt; (2) extract `Publisher` from `Backend` as
`backend.publisher` (follow `FileLibrary`); (3) `src/` as a static lib, tests split by module into separate TUs
(fast core vs. slow QML-driving), `cycle*.inc` renamed by feature; (4) move `documentviewcheck` + its `.inc`s to
a separate `fomawrite-viewcheck` target and the `#ifdef`-spliced test includes out of `main()`; (5) `Main.qml`
mechanical extractions (`NativeMenuBar.qml`, `SourceEditor.qml`, `WorkspaceSettings.qml`, one file per dialog)
and `PreviewPane.qml → VisualEditPane.qml`; (6) move `visualDiff`, `smartReturn`, link escaping and the
`LinkSyntax.js` grammar into C++ (one parser); (7) `macbridge.h`, output-style enum, delete dead code; (8) docs
consolidation (`STATUS/CHANGELOG/ARCHITECTURE`, archive). **Gate:** CI green; test count unchanged or higher;
`backend.cpp` < 2,000 lines; no test code in the product binary.

### Cycle 134 — Product direction decision
With a stable base: choose (a)/(b)/(c) from §6. If (b): a 1-week spike embedding a CM6 live-preview editor in a
`WebEngineView` with a read/write bridge to the C++ document, measuring round-trip fidelity and latency.

### Further review cycles (as opposed to development)
- **R1 — Native acceptance pass** using `bin/check-document-views` + `bin/check-publishing-startup` with your
  app closed, plus the handoff's manual preview matrix. (Blocked today only because your app was open.)
- **R2 — Performance profile** of A2/A3/P1 with a real font-heavy theme and a 20-page document (Instruments).
- **R3 — Fuzz the mapper** — random strings over `-*_#|[]()\n ` + random visual edits; the reprojection guard
  makes this cheap to add and would be the strongest integrity evidence the project has.

---

*Probe sources that reproduced C1–C4 are in the review session's scratchpad (`probe.cpp`, `lockprobe.cpp`) and
can be turned directly into the Cycle 131 regression fixtures.*
