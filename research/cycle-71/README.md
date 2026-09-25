# Cycle 71 — writing-lines icon

Andrew selected the middle concept from the original three-icon board. The new original vector asset has two grey lines and a blue insertion caret. The short grey vertical stroke considered in an intermediate mockup is intentionally absent. The prior split-source/preview icon from Cycle 70 remains in Git history.

The current SVG is `macos/FomawriteIcon.svg`, rendered by `./bin/make-app-icon` to `macos/Fomawrite.icns`. A sanitized 64 px preview is saved here. The old Dev app was inspected before refresh; it contained a clean empty Untitled document and was closed normally. No private document or sidebar screenshot was captured.

`./bin/build` passed and `./bin/test` passed 111/111. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. Both generated bundles declare `CFBundleIconFile=Fomawrite.icns`, contain the same SHA-256 icon resource, and pass `codesign --verify --deep --strict`. The refreshed Dev app launched to a clean Untitled window. The installed `/Applications/Fomawrite.app` was not replaced. Finder/Dock appearance at different scales and on dark backgrounds remains for Andrew to review. [Build log](logs/build.log), [test log](logs/test.log), [package log](logs/package.log), [Dev log](logs/dev.log).
