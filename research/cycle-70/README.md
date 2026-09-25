# Cycle 70 — selected app icon

Andrew selected the third concept: a split Markdown-source and formatted-output page with a blue divider. `macos/FomawriteIcon.svg` is the editable original; `./bin/make-app-icon` generates `macos/Fomawrite.icns` using QtSvg and `iconutil`. qmake embeds the icon, and `macos/Info.plist` names it. No reference-app artwork was copied.

A 1024 px render and the 64 px Dock-size render were visually checked for recognizable source/preview columns. A sanitized 64 px render is saved as `icon-64.png`. The Dev sample had an unsaved synthetic edit; it was saved before the old Dev window was closed normally. No private writing or sidebar screenshot was captured.

`./bin/build` passed. `./bin/test` passed 111/111. `./bin/package-mac` and `./bin/prepare-dev-app` passed. Both generated bundles contain the same SHA-256 icon resource and `CFBundleIconFile=Fomawrite.icns`; both passed `codesign --verify --deep --strict`. Refreshed Dev launched to a clean Untitled window. Finder/Dock appearance at different scale and dark backgrounds still needs Andrew’s review. The ordinary installed app was not replaced. [Build log](logs/build.log), [test log](logs/test.log), [package log](logs/package.log), [Dev log](logs/dev.log).
