# Cycle 52 statistics and chrome decisions

Omawrite keeps its existing rendered plain-text word count and Unicode-scalar character counts. Reading time is the existing estimate at 200 words per minute, rounded up for nonempty text. Speaking time uses 130 words per minute, also rounded up. Empty text reports zero minutes. These are estimates, not measured reading or speech durations.

Sentence counting is a punctuation heuristic over rendered plain text: `.`, `?`, `!` and their CJK equivalents terminate a sentence when followed by whitespace or the end, allowing closing quotation/bracket characters. A remaining alphanumeric run counts once. It is not a language-aware grammar and can count code text or unusual abbreviations imperfectly.

Task count recognizes bullet and numbered `[ ]`, `[x]` or `[X]` markers in Markdown source outside backtick/tilde fenced blocks. Completed and open tasks both count once. Nested constructs and arbitrary extensions are not interpreted.

Human, AI and Reference counts are words in the Markdown source whose full UTF-16 span carries one manual authorship label. Partially labelled and unlabelled words are excluded. The labels are user assertions held in the authorship sidecar and are not verified provenance; they can be lost outside Omawrite. Source-basis category counts need not add up to the rendered word count.

The screenshot shows Stats Only as a multi-select submenu with ten independent checkmarks. Omawrite persists each selected metric; Default and Stats Only are exclusive presentation modes. The compact footer may elide a long combined label, so hover/accessibility expose the full selected set and the statistics dialog gives all values. This differs in placement from iA's top toolbar and remains a parity limitation.

Omawrite implements Title Bar and Toolbar fade as a 160 ms opacity transition when the pointer leaves the 44 px top chrome strip; hovering or focusing a control reveals it. Hide removes only QML toolbar button groups, retaining the native traffic-light/drag-safe strip and native View menu access. Exact iA fade timing was not captured.
