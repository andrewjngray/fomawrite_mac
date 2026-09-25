# Cycle 64 — visual editing baseline

Visual Edit is a guarded projection over canonical Markdown. It applies only safe single-span inline edits and routes protected/structural syntax to Source. It never serializes visual text back to Markdown. Automated integration currently reports 111 passing tests; native verification remains pending. Use the synthetic checklist in `research/usability/cycle-64.md`.

Final integrated checkpoint: Native Dev verified one bounded bold-text replacement changed only its mapped source span, and one Command-Z restored contiguous typing. A simple quote body also edited and undid without altering its marker. Dirty/recovery, IME and multiwindow native cases remain unverified. See `research/cycle-69/README.md` and `research/cycle-69/logs/`.
