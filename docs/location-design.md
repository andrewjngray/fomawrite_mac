# Locations and Favorites: decision record

## Decision — Cycle 46

Andrew approved non-overlapping Locations after supplying iA Writer's overlap warning. A Location represents an entire directory tree; a Favorite is a convenient bookmark into that tree. Adding a parent and a child as separate Locations is rejected in either order. The rejection identifies the existing Location and offers **Add to Favorites instead**. Adding the exact same Location is also rejected.

## Reasoning

One directory tree should have one library entry point. This avoids presenting the same files as separate collections and keeps future cross-location indexing, watching and search easier to deduplicate. A child Favorite provides direct access without registering another Location. This is an Omawrite design decision, not a claim that overlapping folders inherently corrupt files.

The iA screenshot establishes the observed restriction. Its public documentation confirms that Locations are indexed and Favorites are shortcuts to files/folders, but does not explicitly state why overlap is prohibited. Avoiding duplicate indexing/results and ambiguous location-relative paths is our architectural inference, not a confirmed explanation from iA.

References:
- https://ia.net/writer/support/library/organize
- https://ia.net/writer/support/help/trouble-shooting

## Behavior and migration

- Explicit **Locations +** validates readable local folders and canonicalizes paths, including symbolic links. Comparisons use path-component boundaries, so `Notes` and `Notes-old` remain independent.
- Opening a folder, navigating up/back/forward, opening by path, or following a Favorite can show an overlapping folder without registering a new Location. An independent folder opened through existing folder workflows remains automatically registered.
- On loading existing preferences, retain the outermost Locations, remove exact duplicates, and preserve nested entries as Favorites. This is order-independent, preserves the currently viewed folder and touches no documents or directories. Favorites are added idempotently.
- To replace child Locations with a parent, remove those shortcuts first, then explicitly add the parent. The app does not silently replace existing Locations in response to a rejected addition.
- Removing a Location still removes only its shortcut. Folder rename remains a separate physical operation.

## Boundaries

This is not a global search-index implementation. Separate Dev/everyday processes still have independent in-memory views of shared preferences until restarted. Missing locations use normalized absolute paths; unavailable-volume aliases and platform-specific case behavior beyond canonicalization are not fully certified. Existing sidebar-only aliases on nested entries become normal Favorites showing actual disk names.
