# Live friction list

What Andrew finds while using Live as the daily editor. This list is the input to the next cycle (see [MISSION.md](../MISSION.md): the current measure of progress). Newest first. Each item: what happened, where, status.

| Date | What happened | Where | Status |
| --- | --- | --- | --- |
| 9 Oct 2026 | A document opened in **Preview Only** shows no way into Source or Live: the editing capsule is hidden in that layout and nothing hints that it exists. | Footer, Preview Only | Open — design options below |
| 9 Oct 2026 | From Single or Split there is no visible way back to Preview Only; it exists only in **View → Preview Only**. | Footer | Open — design options below |
| 9 Oct 2026 | After opening in Preview Only and choosing Split, **View → Editing** showed neither Source nor Live checked until Live was toggled once. | Native View menu | Bug; test `nativeEditingMenuChecksFollowTheLayoutFromTheStart` |

## Design options for the Preview Only routes (for Andrew to choose)

1. **Three-way layout capsule.** The right-hand capsule becomes **Single | Split | Preview**. Preview Only is then one click away from editing and one click back. The editing capsule still hides in Preview Only (nothing to edit there), but Single/Split are right beside it. Smallest change to the model; moves the two stationary buttons the footer tests pin at the window edge, so those tests change.
2. **An Edit button in Preview Only.** In Preview Only the footer's middle shows a single **Edit** button that returns to the last editing layout (Single or Split) in the last editing mode (Source or Live). Back to Preview Only stays in the View menu. Smaller visual change, but the return path stays hidden.
3. **Both.** Edit button in Preview Only plus Preview in the layout capsule. Most discoverable, most footer width.

Recommendation: option 1. One capsule that names all three layouts is easier to learn than a button that appears only in one state, and the View → Layout submenu already lists exactly those three.
