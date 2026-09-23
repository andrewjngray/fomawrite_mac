# Cycle 36 — richer content blocks and footnotes

Multiline footnote definitions now retain paragraphs and repeated references gain return links. Included Markdown rebases ordinary relative links/images against its own directory; wiki fragments retain their destination anchor. Content blocks support bounded local images, escaped CSV tables (quoted fields, 100 rows/50 columns) and literal code files. Existing nesting, byte, symlink and path boundaries remain.

Build and 64 tests pass. Coverage includes multiline/repeated notes, nested links/images, preserved inline code, CSV escaping/quoted commas, code blocks and wiki fragments. Native combined preview verification follows. This is not a full CommonMark parser: deeply nested inline link syntax, nearest-match wiki search and semantic scroll alignment remain gaps.
