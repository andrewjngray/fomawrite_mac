# Windows test regime

Approved plan: [PLAN.md](PLAN.md). All cases below are **planned, not passed**, until evidence is linked from [STATUS.md](STATUS.md).

## Layers and environments

| Layer | Where / trigger | Required result |
| --- | --- | --- |
| Editor | Existing Node lockfile install, pinned Harper, tests/typecheck/build on PRs and relevant branch pushes | Preserve the baseline tests; committed JS/CSS assets reproduce exactly. |
| Qt | Windows MSVC runner for each supported architecture, plus existing Mac job | Compile and run portable C++/QML regressions. Enumerate genuinely platform-specific skips with reasons. |
| Native app | Interactive Windows desktop, deployed artifacts, disposable profiles | Send real app key/pointer events, inspect canonical content and visible rendered output. Headless checks alone do not pass this gate. |
| Parallels | Andrew's Windows 11 ARM64 guest on Apple Silicon | Test each claimed architecture; label x64 emulation explicitly. Exercise normal VM graphics without disabling browser sandbox. |
| Distribution | Clean Windows environment without Qt on PATH | Portable launch, install, reopen, upgrade and uninstall; application data and documents preserved. |
| Mac protection | Mac CI for shared changes, local native acceptance for editor/publishing changes | No regression of current Mac behaviour; do not replace installed Mac apps to test Windows. |

## Behaviour matrix

| ID | Scenario | Assertions / evidence |
| --- | --- | --- |
| W01 | Open/save/reopen UTF-8, emoji, combining characters, LF/CRLF, BOM, final-newline variants | Exact expected bytes; no unrequested rewrite. Test no-edit and edit paths separately. |
| W02 | Source and Live typing, selection, paste, format, Ctrl+Z/Ctrl+Y | Page/backend agree; history and caret survive mode switches and save. Test physical key input in user follow-through. |
| W03 | Cold startup, restored workspace and unsaved recovery | Correct files/content; no blank preview; prompts preserve unsaved writing. Test two windows and normal quit. |
| W04 | Rapid A/B/A file switching with deliberately delayed Web/PDF jobs | Title, source, page contents and rendered pixels all match the final active document. Stale completions never overwrite it. |
| W05 | Web/PDF toggling, themes, resize, independent zoom, Single/Split | Correct populated output, stable paint after settling, no flicker/reload loop; errors leave actionable visible state. |
| W06 | CSS theme import/reload, nested resource folders, local fonts/images | One selected theme; assets work offline; missing assets cannot silently blank the document. Validate remote-resource and unsafe-path handling. |
| W07 | Markdown fixtures: lists, nested quotes, fences, tables, links, images, footnotes | Source unchanged, Live interactions correct, exported HTML/PDF content agrees with preview. |
| W08 | Save As, export, print, rename, folder reveal, recent documents, external edit | Native dialogs work; cancellation leaves state intact; Explorer targets correct file; read-only/locked paths report errors. |
| W09 | Guest NTFS paths with spaces/Unicode, drive roots, case variants and long paths | File URLs and asset resolution correct; locks/recovery reliable. Record any long-path policy dependency. |
| W10 | Parallels shared folder / UNC path, disconnected shared path | File and asset resolution correct; failure is recoverable; shared-folder behaviour is not inferred from NTFS tests. |
| W11 | Harper offline readiness, suggestions and engine choices | Available engines truthful; no macOS fallback label; spelling refresh does not change Markdown or document revision. |
| W12 | Harper update activation, failed extraction/download, rollback and restart | Integrity/path validation retained; failed update preserves working engine; no administrator symlink requirement. |
| W13 | Windows menus, Ctrl shortcuts, Alt/menu keyboard navigation, F6, clipboard | Commands operate on intended pane; unsupported platform items absent or explained; no focus traps. |
| W14 | Light/dark, 100/150/200% display scaling, resize, multi-monitor if available | Readable fonts, correct hit targets and no clipped controls; imported fonts and fallback fonts verified. |
| W15 | Idle, large document, rapid input, VM suspend/resume, graphics recovery | No sustained unexpected CPU/repaint loop, crashes or document loss; record latency/memory, do not invent performance targets. |
| W16 | Portable and installed build on clean Windows | QtWebEngineProcess, DLLs, QML plugins, locales, PDF and Harper all load without developer tools/PATH. |
| W17 | Upgrade/uninstall and optional Markdown file association | Correct executable/version shown; user files/settings/recovery preserved; associations opt-in and reversible. |
| W18 | Shared-code Mac regression | Existing Mac and editor suites green; affected native rendering/Live workflows checked on Mac. |

## Evidence format

For each tested artifact record commit, build/version, architecture, Qt/MSVC versions, OS build, SHA-256 and install path. Save a machine-readable report, test logs and non-private sample captures under an ignored evidence directory, with a reviewed Markdown summary and manifest committed. Record actual pass/fail/skip separately for each row and environment.

Before internal beta: W01–W08, basic W09 path/save coverage, W11, W13, W16 and W18 must pass. W17 is required before distributing an installer, and W12 before exposing updater commands; other material limitations must be resolved or explicitly accepted and reflected in supported features. W04/W05 are hard correctness gates given previous blank/stale preview regressions. Windows support cannot be declared solely from Qt tests or a successful installer build. M4 requires the full applicable matrix; untested hardware-specific cases remain labelled untested.
