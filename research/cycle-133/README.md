# Cycle133 — structure, CI and the measured hot path

Build **0.3.0-dev29 / macOS 0.3.0 (133)**. Source/test/build changes only; no packaged bundles were refreshed.

Acts on the independent review's §4 (architecture), §7 (process) and the review cycles R2/R3 (§8). The gate the review proposed was ambitious ("CI green, test count ≥ 212, `backend.cpp` < 2,000 lines, no test code in the product binary"); this record says exactly which parts landed, which were deliberately not done and why.

## Landed

| Item | What changed |
| --- | --- |
| **CI** | `.github/workflows/ci.yml`: macOS (Apple Silicon) runner, pinned Qt **6.11.3** via `install-qt-action` with the WebEngine/PDF/Positioning/WebChannel modules, `./bin/build && ./bin/test`, log artifact on failure, triggers on push to `master` and pull requests. The agent verified against the real aqt index that these modules exist for arm64. **First run pending the push of this cycle.** |
| **Docs consolidated** | `STATUS.md` (the only place the current build lives), `CHANGELOG.md` (one line per cycle, 0–133), `ARCHITECTURE.md` (living source map); 15 stale docs and the frozen `build-cycles.md` moved to `docs/archive/` with every link rewritten (1,127 links resolved, one pre-existing broken link left as found); `README` points at STATUS; `AGENTS.md` now defines a cycle as code + red/green tests + record + CHANGELOG + STATUS; `.gitignore`'s per-cycle blocks became one rule (tracked-file count unchanged: 991). |
| **Publisher extracted** | `backendpublishing.inc` (an `#include`d file-length split sharing all of `Backend`'s private state) became `src/publisher.{h,cpp}`: a `Publisher` QObject behind a 13-method `PublishingSource` interface that `Backend` implements. `Backend` keeps one-line forwarders and relays the signals under their old names; `backend.publisher` is exposed for new callers. Zero QML/test changes were needed. `Backend` now holds no publishing state. |
| **Dead code removed** | `saveForClose()` and the `closeAfterSave` signal (no callers; its QML handler could never fire), the `wordCount` property with its 120 ms timer (no bindings), `setCustomReviewWords()` and `spellingIssues()` (no callers), 7 `m_closeAfterSave` resets in `saveTo`. |
| **`macbridge.h`** | The 12 function-local `extern` prototypes for `windowchrome_mac.mm` are declared once, included by callers and by the `.mm` itself, so a signature mismatch is a compile error. `main.cpp`'s lone `Q_OS_MAC` is now `Q_OS_MACOS`. |
| **`NativeMenuBar.qml`** | The ~500-line native menu bar (and the two inline components only it uses) moved out of `Main.qml`, which went from 3,746 to ~3,230 lines. Instantiated from the same `Loader`, so it resolves the window, backend, dialogs and panes through the same context. |
| **`PreviewPane.qml` → `VisualEditPane.qml`** | The left editor file now says what it is; `objectName`s are unchanged because the harness and tests look them up by name. `bin/check-document-views` verifies the two new QML files' embedded copies. |
| **R2 — performance profile** | `tests/perf-profile.inc` (4 measuring slots) and [research/perf/README.md](../perf/README.md). Honest corrections to the review: the pre-132 theme path was ~150 ms per keystroke pause at 16 MiB of fonts (a stutter, not a hang); Cycle 132's Chromium profile sharing saved nothing measurable (profile creation is 0.35 ms); page reuse would save ~48 ms of ~177 ms behind a 200 ms debounce and is **not worth building**. The mapper's O(n²) was the largest real cost. |
| **P1 fixed** | `SourceVisualMapping::appendInline` now matches at an offset and maps runs of plain characters in one span. `perfMapperScaling`: growth exponent **1.99 → 0.97**; 100 KB line **4,127 ms → 0.71 ms**; 1 MiB line **7.5 ms** (was skipped, ~400 s extrapolated); one Visual keystroke on a 1 MiB document **~1,530 ms → ~58 ms**. |

## Verification

- Full offscreen suite after every step: 212 → **216 passed, 0 failed, 0 skipped** (212 + 4 perf measurements). The Publisher extraction, the dead-code sweep and the mapper rewrite each ran green against the complete suite before being committed.
- The mapper change is behavior-preserving by construction (`appendMapped` merges adjacent spans) and is covered by the existing mapper/visual-edit tests; R3's property/fuzz test is the additional gate (see below).
- **Intermittent failure recorded, not hidden:** `formattingRejectsAuxiliaryAndPreviewFocus` failed once (the `linkEditorLabel` scenario) in the run after the menu-bar extraction and passed on 7 of 8 reruns of the same binary and in the two full runs since. It is a timing flake in the link-dialog focus sequence; not attributable to the extraction with the evidence available.

## Deliberately not done (and why)

- **Test/acceptance code out of the product binary.** `bin/check-document-views` runs `--check-document-views` *inside the shipped bundles* (Dev, packaged, installed) precisely to verify the real embedded QML and signatures. A separate `fomawrite-viewcheck` executable would verify a different binary. Until that verification model is redesigned, `documentviewcheck.cpp` stays in the app. The `#ifdef`-spliced `tests/*.inc` in `main.cpp` stay for the same reason (the startup/window-routing harnesses are compile-time variants of the real `main`).
- **`src/` as a static library + tests split by module.** A large build-system change; stacking it on the first CI runs would make failures ambiguous. Next structural cycle, after CI is green on an unchanged build.
- **Further `Backend` extractions** (output styles, document analysis, Markdown editing, authorship, writing review), **`SourceEditor.qml` / dialog extractions**, **JS→C++ parser unification**, **`PublishingPreview.qml` state enum**: not started. `backend.cpp` is ~4.1k lines, so the review's `< 2,000` gate is **not met**; `Publisher` establishes the pattern for the rest.
- **Omarchy theme loader**: the review called it dead I/O on macOS, but it is the retained Linux implementation with its own test (`loadsCurrentOmarchyTheme`) and `AGENTS.md` asks to keep the Linux path separate, not deleted. Left as is.

## Limits

- Offscreen only; no bundle refreshed; no native harness rerun (installed app in use).
- CI is written and reviewed but has not yet produced a run; the Qt download on the runner is the likely first failure point and the YAML comment carries the Homebrew fallback.
