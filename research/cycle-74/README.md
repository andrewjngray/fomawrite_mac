# Cycle 74 — rounded macOS app tile

Andrew's Dock screenshots showed the Cycle 72 writing mark on a small white card inside a larger white square. Against a light Dock, the app lacked the distinct rounded tile and scale of the Outlook icon beside it.

The source SVG now uses a transparent canvas and one large, rounded cool-gray tile, with a subtle border and shadow. The short connector, dark writing lines and blue caret remain the product mark, enlarged to read at Dock size. `./bin/make-app-icon` regenerated the PNG and multiresolution `.icns` from that SVG.

`./bin/build` passed; `./bin/test` passed **111/111** with zero failures and zero skips. Packaging and Dev preparation succeeded. Both refreshed apps launched to clean Untitled windows. The old installed app was clean and closed normally before replacement. The installed executable and `.icns` match the packaged app and pass strict local signature verification. macOS initially returned its cached old icon from `NSWorkspace`; refreshing Launch Services and restarting Dock made it return the new rounded tile. The 128 px macOS-rendered icon is saved here. No app-window screenshot was committed because the sidebar contains private locations.

Known gap: Andrew's visual judgment at his usual Dock size and desktop appearance remains the final design check. This is an ad-hoc local build, not a notarized public release.

[Build log](logs/build.log), [test log](logs/test.log), [package log](logs/package.log), [Dev log](logs/dev.log), [64 px source render](icon-64.png), and [128 px macOS-resolved icon](installed-icon-128.png).
