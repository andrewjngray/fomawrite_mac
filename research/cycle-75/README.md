# Cycle 75 — white-face rounded app icon

Andrew clarified that the Outlook comparison was about its rounded gray rim, not a gray interior. The Cycle 74 icon was still too gray. The vector now uses a white face, a narrow cool-gray rounded border and a subtle shadow while preserving the approved short connector, dark writing lines and blue caret.

`./bin/make-app-icon` regenerated both macOS icon assets. `./bin/build`, `./bin/test` (111 passed, zero failures or skips), `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. The installed app was closed before replacement; the signed installed bundle matches the packaged executable and `.icns`, and launched successfully. Launch Services registration and Dock restart refreshed macOS's cached icon. `NSWorkspace` resolved the installed app to the white-face icon saved here.

The Dock itself was not captured. Andrew should judge its border against light and dark backgrounds at his normal Dock size. This remains a local ad-hoc build rather than a notarized public release.

[Build log](logs/build.log), [test log](logs/test.log), [package log](logs/package.log), [Dev log](logs/dev.log), [128 px generated icon](icon-128.png), and [128 px macOS-resolved icon](installed-icon-128.png).
