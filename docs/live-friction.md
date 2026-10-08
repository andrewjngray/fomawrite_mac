# Live friction list

What Andrew finds while using Live as the daily editor. This list is the input to the next cycle (see [MISSION.md](../MISSION.md): the current measure of progress). Newest first. Each item: what happened, where, status.

| Date | What happened | Where | Status |
| --- | --- | --- | --- |
| 9 Oct 2026 | A document opened in **Preview Only** shows no way into Source or Live: the editing capsule is hidden in that layout and nothing hints that it exists. | Footer, Preview Only | Fixed in source (option 1): the layout capsule is **Single \| Split \| Preview**; one click each way. Test `footerLayoutCapsuleNamesPreviewOnly`. |
| 9 Oct 2026 | From Single or Split there is no visible way back to Preview Only; it exists only in **View → Preview Only**. | Footer | Fixed in source with the item above. |
| 9 Oct 2026 | **Lapis Dark** showed pale text on a white page in Live (and in Web/PDF): Typora themes set their colours as `:root` variables and rely on Typora's base stylesheet to paint the page. | Live, Web/PDF | Fixed in source (Cycle 140): a Typora base shim paints the page from the theme's variables ahead of the theme CSS. |
| 9 Oct 2026 | In Typora a dark theme darkens the whole app; Fomawrite's chrome stayed light. | Window chrome | Fixed in source (Cycle 140): Appearance Follows Output Style, default on. |
| 9 Oct 2026 | `==highlighted text==` kept a pale yellow background under a dark theme's light text (reviewer finding). | Live, Web/PDF | Fixed in source (build 141): highlight takes the theme's `--highlight-color` with a contrast-chosen text colour. |
| 9 Oct 2026 | After opening in Preview Only and choosing Split, **View → Editing** showed neither Source nor Live checked until Live was toggled once. | Native View menu | Fixed in source, not yet installed: the QML state was right all along (test `nativeEditingMenuChecksFollowTheLayoutFromTheStart`), the native Cocoa item showed a stale tick; the Editing and Layout submenus now re-assert their ticks when they open. Needs Andrew's confirmation on the next build. |

## Design options for the Preview Only routes (for Andrew to choose)

1. **Three-way layout capsule.** The right-hand capsule becomes **Single | Split | Preview**. Preview Only is then one click away from editing and one click back. The editing capsule still hides in Preview Only (nothing to edit there), but Single/Split are right beside it. Smallest change to the model; moves the two stationary buttons the footer tests pin at the window edge, so those tests change.
2. **An Edit button in Preview Only.** In Preview Only the footer's middle shows a single **Edit** button that returns to the last editing layout (Single or Split) in the last editing mode (Source or Live). Back to Preview Only stays in the View menu. Smaller visual change, but the return path stays hidden.
3. **Both.** Edit button in Preview Only plus Preview in the layout capsule. Most discoverable, most footer width.

Decision (Andrew, 9 Oct): option 1. Built with constant button widths so the capsule never moves between layouts and a 320 px preview pane keeps room for the Output Style control. One capsule that names all three layouts is easier to learn than a button that appears only in one state, and the View → Layout submenu already lists exactly those three.
