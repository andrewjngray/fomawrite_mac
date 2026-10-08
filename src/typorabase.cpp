#include "typorabase.h"
#include <QColor>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <cmath>

namespace {
// Every rule mirrors what Typora's base stylesheet does with a theme variable.
// Every fallback is the value the property would have without the rule, so a
// theme that defines none of the variables renders as it did before: `--fw-*`
// are the editor page's own variables (undefined in published output, where
// the next fallback applies); CanvasText, LinkText and Highlight are the
// browser's default colours.
const char *const shim = R"CSS(/* Typora base shim (Fomawrite). It precedes the theme, so every explicit theme rule wins. */
/* Typora paints the page (<body>, the whole window) with --bg-color and the text with --text-color. */
html { background: var(--bg-color, var(--fw-bg, transparent)); color: var(--text-color, var(--fw-fg, CanvasText)); }
/* body carries no final colour fallback: with neither variable defined the declaration is invalid at computed-value time and `color` inherits from html, so a theme (or basic preset) that colours html alone keeps its colour. */
body { background: var(--bg-color, var(--fw-bg, transparent)); color: var(--text-color, var(--fw-fg)); }
/* Typora's #write sits on that page and inherits both colours, so it needs no rule. */
/* Typora colours the selection with --select-text-bg-color. */
::selection { background: var(--select-text-bg-color, Highlight); }
/* Typora colours code with --code-color on a --block-bg-color ground. */
code, tt { color: var(--code-color, currentcolor); background-color: var(--block-bg-color, var(--fw-code-bg, transparent)); }
pre, .md-fences { color: var(--code-color, currentcolor); background-color: var(--block-bg-color, transparent); }
pre code { background-color: transparent; }
/* Typora's links use --primary-color; a theme's own `a` rule overrides this fallback. */
a { color: var(--primary-color, var(--fw-accent, LinkText)); }
/* Typora colours list markers with --marker-color and Markdown source text (.md-meta) with --md-char-color. */
::marker { color: var(--marker-color, currentcolor); }
.md-meta { color: var(--md-char-color, currentcolor); }
/* ==highlight==: Typora marks it up as <mark> on a --highlight-color ground; Fomawrite's published HTML spells it <span class="highlight">. The ground falls back to the editor's --fw-highlight-bg, then to pale yellow. The text colour is --fw-highlight-fg, never inherited: a dark theme's light text would vanish on the pale yellow. highlightCss() sets it per theme (dark text unless the theme's ground needs light text); a theme's own `mark` rule still wins. */
mark, .highlight { background: var(--highlight-color, var(--fw-highlight-bg, #fff2a8)); color: var(--fw-highlight-fg, #1a1a1a); }
)CSS";

// The editor page draws its own selection layer and caret (CodeMirror), so the
// variables above need matching hooks there. These selectors equal the page's
// own rules in specificity and come later, so they win.
const char *const live = R"CSS(/* Live page only: CodeMirror draws its own selection and caret. */
#editor .cm-editor .cm-selectionBackground { background: var(--select-text-bg-color, var(--fw-selection)) !important; }
#editor .cm-content { caret-color: var(--text-color, var(--fw-fg)); }
#editor .cm-cursor, #editor .cm-dropCursor { border-left-color: var(--text-color, var(--fw-fg)); }
#write.fw-mode-live .fw-list-mark { color: var(--marker-color, var(--fw-muted)); }
#write.fw-mode-live .fw-code-line { background: var(--block-bg-color, var(--fw-code-bg)); color: var(--code-color, currentcolor); }
#write.fw-mode-live .fw-highlight { background-color: var(--highlight-color, var(--fw-highlight-bg)); color: var(--fw-highlight-fg); }
)CSS";

QString stripComments(const QString &source) {
    QString out;
    out.reserve(source.size());
    QChar quote;
    for (qsizetype i = 0; i < source.size(); ++i) {
        const QChar c = source[i];
        if (!quote.isNull()) {
            out += c;
            if (c == QLatin1Char('\\') && i + 1 < source.size()) out += source[++i];
            else if (c == quote) quote = QChar();
            continue;
        }
        if (c == QLatin1Char('"') || c == QLatin1Char('\'')) { quote = c; out += c; continue; }
        if (c == QLatin1Char('/') && i + 1 < source.size() && source[i + 1] == QLatin1Char('*')) {
            const qsizetype end = source.indexOf(QStringLiteral("*/"), i + 2);
            if (end < 0) break;
            out += QLatin1Char(' ');
            i = end + 1;
            continue;
        }
        out += c;
    }
    return out;
}

// Index of the character that closes the bracket opened at `open`, or -1.
qsizetype matching(const QString &s, qsizetype open) {
    const QChar opener = s[open], closer = opener == QLatin1Char('(') ? QLatin1Char(')') : QLatin1Char('}');
    int depth = 0;
    QChar quote;
    for (qsizetype i = open; i < s.size(); ++i) {
        const QChar c = s[i];
        if (!quote.isNull()) {
            if (c == QLatin1Char('\\')) ++i;
            else if (c == quote) quote = QChar();
            continue;
        }
        if (c == QLatin1Char('"') || c == QLatin1Char('\'')) quote = c;
        else if (c == opener) ++depth;
        else if (c == closer && --depth == 0) return i;
    }
    return -1;
}

// Splits at top-level occurrences of `separator` (not inside quotes or parentheses).
QStringList splitTopLevel(const QString &s, QChar separator) {
    QStringList parts;
    QChar quote;
    int depth = 0;
    qsizetype start = 0;
    for (qsizetype i = 0; i < s.size(); ++i) {
        const QChar c = s[i];
        if (!quote.isNull()) {
            if (c == QLatin1Char('\\')) ++i;
            else if (c == quote) quote = QChar();
            continue;
        }
        if (c == QLatin1Char('"') || c == QLatin1Char('\'')) quote = c;
        else if (c == QLatin1Char('(')) ++depth;
        else if (c == QLatin1Char(')') && depth > 0) --depth;
        else if (c == separator && depth == 0) { parts.append(s.mid(start, i - start)); start = i + 1; }
    }
    parts.append(s.mid(start));
    return parts;
}

bool selectsRoot(const QString &selectors) {
    for (const QString &selector : splitTopLevel(selectors, QLatin1Char(','))) {
        const QString trimmed = selector.trimmed();
        if (trimmed == QLatin1String(":root") || trimmed.compare(QLatin1String("html"), Qt::CaseInsensitive) == 0) return true;
    }
    return false;
}

// A CSS colour with its alpha: #rgb, #rgba, #rrggbb, #rrggbbaa, rgb()/rgba()/hsl()/hsla() (comma or space
// separated, optional `/ alpha`) and named colours. Anything else (oklch(), color-mix(), ...) -> invalid.
QColor parseColour(QString value) {
    value = value.trimmed();
    static const QRegularExpression functional(QStringLiteral("^(rgba?|hsla?)\\(\\s*([^)]*)\\)$"), QRegularExpression::CaseInsensitiveOption);
    if (const auto match = functional.match(value); match.hasMatch()) {
        const QStringList parts = match.captured(2).split(QRegularExpression(QStringLiteral("[\\s,/]+")), Qt::SkipEmptyParts);
        if (parts.size() < 3) return QColor();
        const auto channel = [](QString part, double scale) {
            bool ok = false;
            const bool percent = part.endsWith(QLatin1Char('%'));
            if (percent) part.chop(1);
            const double v = part.toDouble(&ok);
            return !ok ? -1.0 : percent ? v / 100.0 : v / scale;
        };
        const bool hsl = match.captured(1).startsWith(QLatin1String("hsl"), Qt::CaseInsensitive);
        const double a = channel(parts.at(0), hsl ? 360.0 : 255.0), b = channel(parts.at(1), hsl ? 1.0 : 255.0), c = channel(parts.at(2), hsl ? 1.0 : 255.0);
        const double alpha = parts.size() >= 4 ? channel(parts.at(3), 1.0) : 1.0;
        if (a < 0 || b < 0 || c < 0 || alpha < 0) return QColor();
        QColor colour = hsl ? QColor::fromHslF(qBound(0.0, a, 1.0), qBound(0.0, b, 1.0), qBound(0.0, c, 1.0))
                            : QColor::fromRgbF(qBound(0.0, a, 1.0), qBound(0.0, b, 1.0), qBound(0.0, c, 1.0));
        colour.setAlphaF(qBound(0.0, alpha, 1.0));
        return colour;
    }
    if (value.startsWith(QLatin1Char('#'))) {
        static const QRegularExpression hex(QStringLiteral("^#([0-9a-fA-F]+)$"));
        const auto match = hex.match(value);
        if (!match.hasMatch()) return QColor();
        QString digits = match.captured(1);
        if (digits.size() == 3 || digits.size() == 4) { QString wide; for (const QChar d : digits) wide += QString(2, d); digits = wide; }
        if (digits.size() != 6 && digits.size() != 8) return QColor();
        QColor colour(QLatin1Char('#') + digits.left(6));
        if (digits.size() == 8) colour.setAlpha(digits.mid(6, 2).toInt(nullptr, 16));
        return colour;
    }
    static const QRegularExpression named(QStringLiteral("^[A-Za-z]+$"));
    return named.match(value).hasMatch() ? QColor::fromString(value) : QColor();
}

double luminance(const QColor &c) {
    const auto channel = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel(c.redF()) + 0.7152 * channel(c.greenF()) + 0.0722 * channel(c.blueF());
}
double contrast(const QColor &a, const QColor &b) {
    const double la = luminance(a), lb = luminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}
// `top` painted over the opaque `under`.
QColor over(const QColor &top, const QColor &under) {
    const double a = top.alphaF();
    return QColor::fromRgbF(top.redF() * a + under.redF() * (1 - a), top.greenF() * a + under.greenF() * (1 - a), top.blueF() * a + under.blueF() * (1 - a));
}

// Substitutes var() references in `value`; false when one cannot be resolved.
// Resolved values are memoised per variable name and capped in length, so a
// theme that fans out (each variable referencing the next many times) costs
// linear work instead of exponential; imported theme CSS is untrusted input.
constexpr qsizetype MaxResolvedLength = 4096;
bool substitute(const QString &value, const QMap<QString, QString> &raw, int depth, QSet<QString> &active, QHash<QString, QString> &memo, QString *out) {
    if (depth > 16) return false;
    static const QRegularExpression start(QStringLiteral("\\bvar\\s*\\("));
    QString result;
    qsizetype pos = 0;
    for (;;) {
        const auto match = start.match(value, pos);
        if (!match.hasMatch()) { result += value.mid(pos); break; }
        result += value.mid(pos, match.capturedStart() - pos);
        const qsizetype open = match.capturedEnd() - 1, close = matching(value, open);
        if (close < 0) return false;
        const QString inner = value.mid(open + 1, close - open - 1);
        const qsizetype comma = inner.indexOf(QLatin1Char(','));
        const QString name = (comma < 0 ? inner : inner.left(comma)).trimmed();
        QString resolved;
        bool found = false;
        if (memo.contains(name)) { resolved = memo.value(name); found = true; }
        else if (raw.contains(name) && !active.contains(name)) {
            active.insert(name);
            found = substitute(raw.value(name), raw, depth + 1, active, memo, &resolved);
            active.remove(name);
            if (found && active.isEmpty()) memo.insert(name, resolved);
        }
        if (!found && (comma < 0 || !substitute(inner.mid(comma + 1).trimmed(), raw, depth + 1, active, memo, &resolved))) return false;
        result += resolved.trimmed();
        if (result.size() > MaxResolvedLength) return false;
        pos = close + 1;
    }
    *out = result;
    return true;
}
}

namespace TyporaBase {
QString css() { return QString::fromUtf8(shim); }
QString highlightText(const QString &themeCss) { return highlightText(themeVariables(themeCss)); }
QString highlightText(const QMap<QString, QString> &variables) {
    const QString dark = QStringLiteral("#1a1a1a"), light = QStringLiteral("#ffffff");
    const QColor highlight = parseColour(variables.value(QStringLiteral("--highlight-color")));
    // No (readable) highlight colour: the ground is the editor's own pale yellow, which dark text always reads on.
    if (!highlight.isValid() || highlight.alpha() == 0) return dark;
    // The ground the text really sits on: the highlight over the page (the theme's --bg-color, else white paper).
    QColor page = parseColour(variables.value(QStringLiteral("--bg-color")));
    page = page.isValid() ? over(page, Qt::white) : QColor(Qt::white);
    const QColor ground = over(highlight, page);
    if (contrast(QColor(dark), ground) >= 4.5) return dark;
    return contrast(QColor(light), ground) > contrast(QColor(dark), ground) ? light : dark;
}
QString highlightCss(const QString &themeCss) { return highlightCss(themeVariables(themeCss)); }
QString highlightCss(const QMap<QString, QString> &variables) {
    return QStringLiteral("/* Typora base shim: text colour for ==highlight==, chosen from the theme's --highlight-color. */\n:root { --fw-highlight-fg: %1; }\n").arg(highlightText(variables));
}
QString liveCss() { return css() + QString::fromUtf8(live); }

QMap<QString, QString> themeVariables(const QString &source) {
    const QString text = stripComments(source);
    QMap<QString, QString> raw;
    qsizetype i = 0;
    while (i < text.size()) {
        // The prelude runs to the next `{` (a rule or block at-rule) or `;` (a statement at-rule such as @import).
        qsizetype end = i;
        QChar quote;
        for (; end < text.size(); ++end) {
            const QChar c = text[end];
            if (!quote.isNull()) { if (c == QLatin1Char('\\')) ++end; else if (c == quote) quote = QChar(); continue; }
            if (c == QLatin1Char('"') || c == QLatin1Char('\'')) quote = c;
            else if (c == QLatin1Char('{') || c == QLatin1Char(';')) break;
        }
        if (end >= text.size()) break;
        if (text[end] == QLatin1Char(';')) { i = end + 1; continue; }
        const QString prelude = text.mid(i, end - i).trimmed();
        const qsizetype close = matching(text, end);
        if (close < 0) break;
        if (!prelude.startsWith(QLatin1Char('@')) && selectsRoot(prelude)) {
            static const QRegularExpression important(QStringLiteral("\\s*!\\s*important\\s*$"), QRegularExpression::CaseInsensitiveOption);
            for (const QString &declaration : splitTopLevel(text.mid(end + 1, close - end - 1), QLatin1Char(';'))) {
                const qsizetype colon = declaration.indexOf(QLatin1Char(':'));
                if (colon < 0) continue;
                const QString name = declaration.left(colon).trimmed();
                if (!name.startsWith(QLatin1String("--")) || name.size() < 3) continue;
                QString value = declaration.mid(colon + 1).trimmed();
                value.remove(important);
                if (!value.isEmpty()) raw.insert(name, value);
            }
        }
        i = close + 1;
    }
    QMap<QString, QString> resolved;
    QHash<QString, QString> memo;
    for (auto it = raw.cbegin(); it != raw.cend(); ++it) {
        QSet<QString> active{it.key()};
        QString value;
        if (substitute(it.value(), raw, 0, active, memo, &value) && !value.trimmed().isEmpty()) resolved.insert(it.key(), value.trimmed());
    }
    return resolved;
}

QString themeVariable(const QString &css, const QString &name) {
    return themeVariables(css).value(name.startsWith(QLatin1String("--")) ? name : QStringLiteral("--") + name);
}
}
