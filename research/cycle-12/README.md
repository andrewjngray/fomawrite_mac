# Cycle 12 records

## Cycle 12 — collapsible organizer sections and native title bar

**Planned scope:** Favorites/Recents disclosure chevrons, aligned favorite actions, and investigation of the larger/lower native traffic lights and rounder frame in Andrew's iA reference.

**Implemented:** clickable section headers with down/right chevrons and persisted expanded states. Favorites collapses its entries and add actions; Recents collapses its entries without clearing them. Favorite folder/document use identical left padding and plus-icon columns. macOS now requests a native unified NSToolbar/title-bar style rather than a plain title bar; native controls retain their system behavior and dimensions.

**Verification:** build and 25 tests passed, including disclosure toggles and shortcut preservation. Native QA verified both collapses and Favorites restoration, aligned actions, and continued toolbar interaction. Screenshots under research/cycle-12/screenshots show the sample document and hide private recents. The screen-sharing badge overlays the traffic lights, so exact dot size/placement and corner-radius parity with iA remain unverified. No custom corner mask or enlarged imitation controls added. Full dark-mode, restart persistence and fullscreen transition checks remain outstanding.

**Artifacts:** dist/Omawrite.app and dist/Omawrite Dev.app. The running Dev bundle has the same functional implementation as the final source (subsequent changes only reindent QML/add tests). The sample acquired unsaved state during QA; no save/discard action was taken and that window was preserved.

**Optional exercise:** toggle both section headers, check aligned Favorite actions, restart after saving your work to check persistence, and compare the traffic lights and corners once app inspection stops.

Native API reference: https://developer.apple.com/documentation/appkit/nswindow/toolbarstyle-swift.enum/unified

Cycle 12 final packaging and deep/strict signature verification passed.
