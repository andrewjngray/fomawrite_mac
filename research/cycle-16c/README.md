# Cycle 16c — two-second path tooltips

20 September 2026. Andrew reported an immediate full-path tooltip in Recents. Locations, Favorites and Recents now pass their full path through the shared ChromeButton hint implementation, with tooltipDelay set to 2000ms. Library path tooltips also use 2000ms. Toolbar hints keep the existing 700ms delay. Accessibility labels remain the short names.

Build and all 33 existing regression tests passed; code diff whitespace checks passed. Logs: logs/build.log and logs/test.log. No extra automated test added for this low-impact UI timing adjustment. The user-supplied screenshot was inspected in the conversation and not copied into the repository.

Dev refresh and live timing check are pending. Automatic approval review rejected app inspection due to potential private content; renewed permission was requested, despite earlier authorization. Running Dev was not replaced. Native screenshots were not captured.

The ordinary dist/Omawrite.app distributable was packaged and passed strict/deep signature verification; see logs/package-mac.log and logs/package-signature.log. Stable dist/Omawrite Dev.app still contains Cycle 16b until the approved safe refresh. This polish pass leaves Cycle 17 as the next feature increment. Changes remain uncommitted alongside prior work. [Optional hover exercise](../usability/cycle-16c.md).


21 September update: Dev refreshed with Cycle 17 after authorized inspection and normal clean close. Tooltip change is now in both bundles. Exact live timing remains unverified.
