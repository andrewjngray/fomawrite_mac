#ifndef TYPORABASE_H
#define TYPORABASE_H
#include <QMap>
#include <QString>

// Typora themes colour the page through CSS custom properties on :root
// (--bg-color, --text-color, ...) and rely on Typora's own base stylesheet to
// paint the page from them. Fomawrite applies the theme CSS but has no such
// base, so this module supplies the small part of it that matters:
//
//   css()      the shim, one text defined once. Web/PDF output emits it before
//              the theme CSS (Publisher::html) and the Live page receives it
//              ahead of the theme CSS (Backend::liveThemeCss), so any explicit
//              theme rule of equal or higher specificity wins.
//   liveCss()  css() plus the rules that only make sense on the editor page
//              (CodeMirror's own selection layer, caret, Live code lines).
//   highlightText() / highlightCss()
//              the text colour for ==highlight== (a dark one unless the theme's
//              --highlight-color, laid over its page colour, needs a light one):
//              highlightCss() is the one-rule stylesheet that publishes it as
//              --fw-highlight-fg, emitted after css() ahead of the theme's text.
//   themeVariables() / themeVariable()
//              the :root custom properties a theme defines, parsed so the host
//              can read them (for example the page background colour).
namespace TyporaBase {
QString css();
QString liveCss();
// "#1a1a1a" or "#ffffff": whichever the theme's highlight ground needs for 4.5:1
// (WCAG) contrast. Dark text is kept whenever it reads, and when the theme defines
// no usable --highlight-color. The ground is --highlight-color composited over
// --bg-color (white when the theme defines none).
QString highlightText(const QString &themeCss);
QString highlightCss(const QString &themeCss);
// The same from variables the caller already parsed (Publisher memoises them per theme).
QString highlightText(const QMap<QString, QString> &variables);
QString highlightCss(const QMap<QString, QString> &variables);
// Custom properties declared by top-level `:root { }` / `html { }` rules (rules
// inside @media, @supports ... are ignored; the last declaration wins; the
// cascade's !important and specificity are not modelled). Values are resolved:
// var(--x) and var(--x, fallback) are substituted (nested references up to 16
// levels, cycles are dropped). A variable that cannot be resolved is omitted.
QMap<QString, QString> themeVariables(const QString &css);
// One variable by name, with or without the leading `--`; empty when undefined.
QString themeVariable(const QString &css, const QString &name);
}
#endif
