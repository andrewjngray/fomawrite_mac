# Cycle 73 — Dock icon appears at runtime

Andrew's screenshots show the generic macOS placeholder in the Dock for both Fomawrite and Fomawrite Dev. The `.icns` resource and `CFBundleIconFile` entry were present, and macOS `NSWorkspace` resolved the chosen icon from the installed bundle. The runtime code instead called `QIcon::fromTheme("fomawrite")`; that lookup returned a null icon on this Mac. Qt documents `QWindow::setIcon()` as the Dock icon mechanism for a running macOS app.

The app now embeds a PNG rendered from the same approved SVG used to generate the `.icns`. On macOS it sets that image as both the application and each QML window icon. Other platforms retain their theme-icon lookup. `./bin/make-app-icon` regenerates both image resources from the vector source, preventing them from drifting.

`./bin/build` passed; `./bin/test` passed 111/111. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. A targeted Qt resource check found the new icon and rendered a non-null 64 px pixmap. The generated ordinary and Dev bundles passed strict local signature checks. The old installed app had a clean empty Untitled window and was closed normally; `/Applications/Fomawrite.app` was replaced with the generated ordinary bundle, its executable and icon matched, and it launched to a clean Untitled window. Dev also launched cleanly. No app screenshot was committed because the sidebar contains private locations. The Dock itself was not captured through the UI tool, so Andrew's visual check remains useful in case macOS displays a cached pinned tile.

[Build log](logs/build.log), [test log](logs/test.log), [package log](logs/package.log), [Dev log](logs/dev.log), [runtime icon check](logs/runtime-icon.txt), and [sanitized 64 px icon](icon-64.png).
