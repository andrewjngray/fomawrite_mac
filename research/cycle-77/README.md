# Cycle 77 — cmux-inspired off-white icon

Andrew found the pure-white Cycle 76 tile too bright and supplied a screenshot of the cmux Dock icon. Sample points on the cmux face ranged from approximately RGB 254/254/254 near the top to 240/240/240 near the bottom. The Fomawrite vector now uses a subtle vertical gradient from `#fefefe` to `#eeeeee`, keeping the borderless rounded shape, soft bottom shadow, writing lines, short connector and blue caret.

`./bin/make-app-icon` regenerated the PNG and multiresolution `.icns`. `./bin/build` passed, `./bin/test` passed 111/111 with zero failures or skips, and both packaging scripts completed. The generated 128 px icon is saved here. The ordinary installed app was left untouched while it was running, then replaced after it exited. The signed installed executable and `.icns` match the packaged bundle. Launch Services registration and a Dock restart refreshed the cached icon; the installed app launched, and `NSWorkspace` resolved the off-white icon saved here.

Known gap: the Dock appearance on Andrew's wallpaper still needs his visual review. This is a local ad-hoc build, not a notarized public release.

[Build log](logs/build.log), [test log](logs/test.log), [package log](logs/package.log), [Dev log](logs/dev.log), and [128 px generated icon](icon-128.png), and [128 px macOS-resolved icon](installed-icon-128.png).
