# Native record — 25a

Native two dirty windows: New preserves first, Quit/Cancel preserves both, then Discard advances to second prompt. Tests cover new-window request and save-failure cancellation.

Remaining: Separate application launches are not consolidated. Saved-window restoration across restart and exhaustive crash/OS-quit/minimized-window matrix remain. Clean windows already closed before a later Cancel are not reopened automatically.
