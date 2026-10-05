# Cycle 126 — theme gallery link

Review build **0.3.0-dev22 / macOS 0.3.0 (126)** adds **Get More Themes…** immediately above Import Theme in the publishing footer menu and View → Publishing Theme. Export and share offers the same action. Each opens exactly https://theme.typora.io/ in the default browser using Qt's external URL handler.

The existing theme-folder/import workflow installs downloaded CSS and resources. This action browses the gallery; it does not automatically install downloads. No new tests were added for this small URL action. Build and packaging succeeded. All **195 regressions pass**, with **157 native checks each on Dev and the installed app**, zero QML warnings and strict signature verification. The ordinary packaged executable matches the installed copy; it did not receive a separate native run. Exact identities are recorded in [verified-builds.json](verified-builds.json).

Optional review: open the publishing dropdown, choose Get More Themes…, confirm the Typora gallery opens, then return to the same document/theme. The native View menu and export action should open the same address. Physical browser handoff is not part of the synthetic native workflow checks.

Current copies: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. Native checks use synthetic Qt input on Cocoa. The browser handoff remains an optional physical review step.
