# Cycle 43: context menus

Added screenshot-backed location actions and path/Finder actions on files/folders.

Build and 71 tests pass. Native QA on the stable Dev identity verified right-click location menu, rename dialog, label update and restoration, file context menu without changing current document, and Finder selecting the clicked sample Workbench.md. No document edits made. Test coverage verifies shortcut removal preserves disk files and aliases survive recreation of FileLibrary.

Screenshots contain only sample writing. Finder was checked through accessibility; its broader sidebar was not captured for the repository. Both stable local app bundles refreshed; versioned GitHub RC1 binary is unchanged. Menu-key, dark-mode and VoiceOver native checks remain open.
