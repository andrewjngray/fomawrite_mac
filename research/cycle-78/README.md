# Cycle 78 — running Dock icon switches to the first dark concept

Andrew selected the first, cmux-matched negative concept for Fomawrite's running state. The committed bundle `.icns` remains the approved light icon for Finder and a pinned Dock tile after quit. The first dark SVG is rendered into a separate Qt resource for the running process.

On macOS, Qt gives windows the dark running icon. AppKit temporarily sets `NSApplication.applicationIconImage` to that same image when the event loop starts. The guarded quit path preserves unsaved-change prompts; `aboutToQuit` restores Qt's light icon and clears AppKit's temporary override. Apple documents that assigning `nil` restores the original bundle icon. Linux retains its existing themed icon behavior.

`./bin/build` passed; `./bin/test` passed 111/111. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded, and both bundles passed strict local signature verification. Their `.icns` hashes match the unchanged approved light source. The running SVG rendered identically to the first dark concept at 128 px. The refreshed Dev app launched. A narrow screen capture displayed a remote Windows taskbar rather than the macOS Dock, so the actual Dock tile transition has not yet been visually confirmed. Once both old processes exited, `/Applications/Fomawrite.app` was replaced with a signed bundle matching the packaged executable and light `.icns`. Its Finder-facing icon resolved to the approved light render, and the installed app launched.

Known gaps: visual confirmation of the running dark Dock tile and the light pinned tile after quit. These are local ad-hoc builds, not notarized for public release. No screen capture of private writing was committed.

[Build log](logs/build.log), [test log](logs/test.log), [package log](logs/package.log), [Dev log](logs/dev.log), [closed light render](closed-light-128.png), [running dark render](running-dark-128.png), [installed bundle light icon](installed-bundle-light-128.png).
