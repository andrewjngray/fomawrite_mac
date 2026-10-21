# Windows target decision and evidence

Checked 22 October 2026 by a delegated GPT-6.1 Sol feasibility audit. This is package availability evidence, not a successful application build or runtime certification.

## Decision

Build **native ARM64 first** for Andrew's Windows 11 Enterprise ARM64 Parallels guest on Apple Silicon. Add a separate **x64** artifact for conventional Windows PCs. Keep Qt **6.11.3**, MSVC **2022**, qmake and the application's C++17 baseline. Do not use MinGW for this WebEngine application. ARM64EC is not the ARM64 target.

Read-only guest inspection returned `Arm64`, `Microsoft Windows 11 Enterprise`, `10.0.26200`. Git is available. Compiler/Qt/Node were not found on PATH; inspect installed tools and prefer CI builds before adding a full toolchain to the user's VM.

## Official package metadata

| Target | Base kit | WebEngine / PDF |
| --- | --- | --- |
| Native ARM64 | `qt.qt6.6113.win64_msvc2022_arm64` | `extensions.qtwebengine.6113.win64_msvc2022_arm64`; `extensions.qtpdf.6113.win64_msvc2022_arm64` |
| x64 | `qt.qt6.6113.win64_msvc2022_64` | `extensions.qtwebengine.6113.win64_msvc2022_64`; `extensions.qtpdf.6113.win64_msvc2022_64` |
| ARM64 cross-compiled on x64 | Base suffix `win64_msvc2022_arm64_cross_compiled` | ARM64 WebEngine/PDF packages are also published for cross compilation; validate host-tool dependencies in the chosen CI installer. |

WebChannel ARM64 package: `qt.qt6.6113.addons.qtwebchannel.win64_msvc2022_arm64`. Core, Widgets and Quick/QML are supplied by the base kit archives. WebEngine and PDF are extension repositories; their absence from the base add-on list does not mean they are unavailable. An installer action or aqt version may need explicit extension support; do not silently drop modules when an installer listing differs.

Primary sources inspected:

- [ARM64 base and WebChannel](https://download.qt.io/online/qtsdkrepository/windows_arm64/desktop/qt6_6113/qt6_6113/Updates.xml)
- [ARM64 WebEngine](https://download.qt.io/online/qtsdkrepository/windows_arm64/extensions/qtwebengine/6113/msvc2022_arm64/Updates.xml)
- [ARM64 PDF](https://download.qt.io/online/qtsdkrepository/windows_arm64/extensions/qtpdf/6113/msvc2022_arm64/Updates.xml)
- [x64 base and WebChannel](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6113/qt6_6113_msvc2022_64/Updates.xml)
- [x64 WebEngine](https://download.qt.io/online/qtsdkrepository/windows_x86/extensions/qtwebengine/6113/msvc2022_64/Updates.xml)
- [x64 PDF](https://download.qt.io/online/qtsdkrepository/windows_x86/extensions/qtpdf/6113/msvc2022_64/Updates.xml)
- [Qt Windows platform support](https://doc.qt.io/qt-6.11/windows.html)
- [WebEngine compiler/platform notes](https://doc.qt.io/qt-6.11/qtwebengine-platform-notes.html)

## Remaining target gate

Select and prove the runner/host tools; record exact dependency versions and checksums where available; compile and deploy; verify Chromium subprocess/sandbox/GPU behaviour and PDF/Live paint in the actual VM. If native ARM64 tooling blocks the first cycle, an explicitly labelled x64 emulation spike is useful evidence but does not close native ARM64 acceptance.


## Runner and installer proof (M1 bootstrap)

The repository is public. Native ARM64 uses GitHub's `windows-11-vs2026-arm`
image with its separately installed **MSVC 2022 v143** toolset; x64 uses
`windows-2022`. `bin/windows/common.ps1` discovers v143 explicitly and records
its exact version rather than accepting the ARM image's default VS2026 compiler.
The ARM image lists v143 14.44 and SDK 10.0.26100.0; the actual selected versions
must be read from each build's manifest. Sources: [ARM runner availability](https://github.blog/changelog/2025-08-07-arm64-hosted-runners-for-public-repositories-are-now-generally-available/),
[ARM image inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows11-VS2026-Arm64-Readme.md).

`windows/qt-6.11.3.json` pins official archive URLs and upstream SHA-1 hashes
for both native kits, including their WebEngine/PDF extension repositories.
The installer verifies those hashes before extraction, records SHA-256 hashes,
and verifies the relocated qmake version/prefix and required modules.
[Bootstrap run 39670164799](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39670164799)
installed and verified both native Qt SDKs. It then failed at the same
Visual Studio environment command quoting defect, before compiling the app.
This confirms native runner and kit availability, not a working executable.

Parallels `exec --current-user` runs in guest desktop session 1, matching
Explorer. The guest's local application-data directory is available for
isolated artifacts; its Desktop maps to the Mac share and is not a disposable
local test destination. No compiler, SDK or Qt installation was added to the VM.
