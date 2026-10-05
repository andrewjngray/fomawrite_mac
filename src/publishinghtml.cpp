#include "publishinghtml.h"
#include "vendor/md4c/md4c.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QTextDocument>
#include <QUrl>
#include <QVector>

namespace {
QString utf8(const char *text, MD_SIZE size) { return QString::fromUtf8(text, size); }

QString entity(const QString &text) {
    // Qt is used only to decode entities, never to generate the document HTML.
    QTextDocument document;
    document.setHtml(text);
    return document.toPlainText();
}

QString attribute(const MD_ATTRIBUTE &value) {
    QString result;
    for (int i = 0; value.text && value.substr_offsets[i] < value.size; ++i) {
        const auto part = utf8(value.text + value.substr_offsets[i], value.substr_offsets[i + 1] - value.substr_offsets[i]);
        result += value.substr_types[i] == MD_TEXT_ENTITY ? entity(part)
                : value.substr_types[i] == MD_TEXT_NULLCHAR ? QString(QChar::ReplacementCharacter) : part;
    }
    return result;
}

bool safeUrl(const QString &value, bool image) {
    // Decode entities before checking schemes; prohibit control-character tricks.
    for (const auto c : value) if (c.unicode() < 32 || c.unicode() == 127) return false;
    const auto scheme = QUrl(value.trimmed()).scheme().toLower();
    return scheme.isEmpty() || scheme == "file" || scheme == "http" || scheme == "https"
            || (!image && scheme == "mailto");
}

QString headingSlug(const QString &rendered) {
    // Match Backend::headingSlug(block.text()), including its Markdown-to-plain pass.
    QTextDocument renderedDocument;
    renderedDocument.setHtml(rendered);
    const QString title = renderedDocument.toPlainText();
    QTextDocument plain;
    plain.setMarkdown(title);
    QString slug = plain.toPlainText().toLower().trimmed();
    slug.remove(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}_\\s-]")));
    slug.replace(QRegularExpression("\\s+"), "-");
    return slug.isEmpty() ? QStringLiteral("section") : slug;
}

struct Renderer {
    QString html;
    QHash<QString, int> occurrences;
    QSet<QString> ids;
    QVector<QString> links;
    QVector<QString> extensionTags;
    qsizetype headingStart = 0;
    int imageDepth = 0;
    bool htmlBlock = false;
    QString rawBlock;
    QString imageAlt, imageSource, imageTitle;

    QString safeHtml(const QString &input) {
        // The extensions emit only these inert tags. Everything else stays visible
        // as escaped text, including scripts, event handlers, and arbitrary styles.
        static const QRegularExpression tags("<!--[\\s\\S]*?-->|<[^>]*>");
        static const QRegularExpression anchor("^<a name=\"(ow-note-[0-9]+-[\\w-]+(?:-ref-[1-9][0-9]*)?)\">$");
        // CSV expansion is a single inert table with escaped cell text.
        static const QRegularExpression csv("^<table>((?:<tr>(?:<td>[^<>]*</td>)+</tr>)+)</table>\\s*$");
        const auto csvMatch = csv.match(input);
        if (csvMatch.hasMatch()) return "<table><tbody>" + csvMatch.captured(1) + "</tbody></table>\n";
        QString result;
        qsizetype offset = 0;
        auto matches = tags.globalMatch(input);
        while (matches.hasNext()) {
            const auto match = matches.next();
            result += input.mid(offset, match.capturedStart() - offset).toHtmlEscaped();
            const auto tag = match.captured();
            const auto target = anchor.match(tag);
            if (tag == "<!-- pagebreak -->" && htmlBlock && input.trimmed() == tag) result += "<div class=\"pagebreak\" aria-hidden=\"true\"></div>";
            else if (target.hasMatch()) {
                result += "<a id=\"" + target.captured(1).toHtmlEscaped() + "\">";
                extensionTags.append("</a>");
            } else if (!extensionTags.isEmpty() && tag == extensionTags.last()) {
                result += tag; extensionTags.removeLast();
            } else if (tag == "<span style=\"background-color:#fff0a3;color:#222222\">") {
                result += "<span class=\"highlight\">"; extensionTags.append("</span>");
            }
            else result += tag.toHtmlEscaped();
            offset = match.capturedEnd();
        }
        result += input.mid(offset).toHtmlEscaped();
        return result;
    }
};

int enterBlock(MD_BLOCKTYPE type, void *detail, void *data) {
    auto &r = *static_cast<Renderer *>(data);
    switch (type) {
    case MD_BLOCK_DOC: break;
    case MD_BLOCK_HTML: r.htmlBlock = true; r.rawBlock.clear(); break;
    case MD_BLOCK_QUOTE: r.html += "<blockquote>\n"; break;
    case MD_BLOCK_UL: r.html += "<ul>\n"; break;
    case MD_BLOCK_OL: {
        const auto start = static_cast<MD_BLOCK_OL_DETAIL *>(detail)->start;
        r.html += start == 1 ? "<ol>\n" : "<ol start=\"" + QString::number(start) + "\">\n";
        break;
    }
    case MD_BLOCK_LI: {
        const auto *item = static_cast<MD_BLOCK_LI_DETAIL *>(detail);
        r.html += "<li>";
        if (item->is_task) r.html += QString("<input type=\"checkbox\" disabled%1> ").arg(item->task_mark == ' ' ? "" : " checked");
        break;
    }
    case MD_BLOCK_H:
        r.html += "<h" + QString::number(static_cast<MD_BLOCK_H_DETAIL *>(detail)->level) + ">";
        r.headingStart = r.html.size();
        break;
    case MD_BLOCK_HR: r.html += "<hr>\n"; break;
    case MD_BLOCK_CODE: {
        const auto lang = attribute(static_cast<MD_BLOCK_CODE_DETAIL *>(detail)->lang);
        r.html += "<pre><code";
        if (!lang.isEmpty()) r.html += " class=\"language-" + lang.toHtmlEscaped() + "\"";
        r.html += ">";
        break;
    }
    case MD_BLOCK_P: r.html += "<p>"; break;
    case MD_BLOCK_TABLE: r.html += "<table>\n"; break;
    case MD_BLOCK_THEAD: r.html += "<thead>\n"; break;
    case MD_BLOCK_TBODY: r.html += "<tbody>\n"; break;
    case MD_BLOCK_TR: r.html += "<tr>"; break;
    case MD_BLOCK_TH: case MD_BLOCK_TD: {
        r.html += type == MD_BLOCK_TH ? "<th" : "<td";
        const auto align = static_cast<MD_BLOCK_TD_DETAIL *>(detail)->align;
        if (align != MD_ALIGN_DEFAULT) r.html += QString(" style=\"text-align:%1\"").arg(align == MD_ALIGN_LEFT ? "left" : align == MD_ALIGN_RIGHT ? "right" : "center");
        r.html += ">";
        break;
    }
    }
    return 0;
}

int leaveBlock(MD_BLOCKTYPE type, void *detail, void *data) {
    auto &r = *static_cast<Renderer *>(data);
    switch (type) {
    case MD_BLOCK_DOC: case MD_BLOCK_HR: break;
    case MD_BLOCK_HTML: r.html += r.safeHtml(r.rawBlock); r.htmlBlock = false; r.rawBlock.clear(); break;
    case MD_BLOCK_QUOTE: r.html += "</blockquote>\n"; break;
    case MD_BLOCK_UL: r.html += "</ul>\n"; break;
    case MD_BLOCK_OL: r.html += "</ol>\n"; break;
    case MD_BLOCK_LI: r.html += "</li>\n"; break;
    case MD_BLOCK_H: {
        const auto base = headingSlug(r.html.mid(r.headingStart));
        int count = r.occurrences[base]++;
        QString id = count ? base + "-" + QString::number(count) : base;
        while (r.ids.contains(id)) id = base + "-" + QString::number(r.occurrences[base]++);
        r.ids.insert(id);
        r.html.insert(r.headingStart - 1, " id=\"" + id.toHtmlEscaped() + "\"");
        r.html += "</h" + QString::number(static_cast<MD_BLOCK_H_DETAIL *>(detail)->level) + ">\n";
        break;
    }
    case MD_BLOCK_CODE: r.html += "</code></pre>\n"; break;
    case MD_BLOCK_P: r.html += "</p>\n"; break;
    case MD_BLOCK_TABLE: r.html += "</table>\n"; break;
    case MD_BLOCK_THEAD: r.html += "</thead>\n"; break;
    case MD_BLOCK_TBODY: r.html += "</tbody>\n"; break;
    case MD_BLOCK_TR: r.html += "</tr>\n"; break;
    case MD_BLOCK_TH: r.html += "</th>"; break;
    case MD_BLOCK_TD: r.html += "</td>"; break;
    }
    return 0;
}

int enterSpan(MD_SPANTYPE type, void *detail, void *data) {
    auto &r = *static_cast<Renderer *>(data);
    if (type == MD_SPAN_IMG) {
        if (++r.imageDepth == 1) {
            const auto *image = static_cast<MD_SPAN_IMG_DETAIL *>(detail);
            r.imageSource = attribute(image->src); r.imageTitle = attribute(image->title); r.imageAlt.clear();
        }
        return 0;
    }
    if (r.imageDepth) return 0;
    switch (type) {
    case MD_SPAN_EM: r.html += "<em>"; break;
    case MD_SPAN_STRONG: r.html += "<strong>"; break;
    case MD_SPAN_CODE: r.html += "<code>"; break;
    case MD_SPAN_DEL: r.html += "<del>"; break;
    case MD_SPAN_A: {
        const auto *link = static_cast<MD_SPAN_A_DETAIL *>(detail);
        const auto href = attribute(link->href), title = attribute(link->title);
        const bool safe = safeUrl(href, false);
        r.links.append(safe ? "</a>" : "</span>");
        r.html += safe ? "<a href=\"" + href.toHtmlEscaped() + "\"" : "<span";
        if (!title.isEmpty()) r.html += " title=\"" + title.toHtmlEscaped() + "\"";
        r.html += ">";
        break;
    }
    default: break;
    }
    return 0;
}

int leaveSpan(MD_SPANTYPE type, void *, void *data) {
    auto &r = *static_cast<Renderer *>(data);
    if (type == MD_SPAN_IMG) {
        if (--r.imageDepth == 0) {
            if (safeUrl(r.imageSource, true)) {
                r.html += "<img src=\"" + r.imageSource.toHtmlEscaped() + "\" alt=\"" + r.imageAlt.toHtmlEscaped() + "\"";
                if (!r.imageTitle.isEmpty()) r.html += " title=\"" + r.imageTitle.toHtmlEscaped() + "\"";
                r.html += ">";
            } else r.html += r.imageAlt.toHtmlEscaped();
        }
        return 0;
    }
    if (r.imageDepth) return 0;
    switch (type) {
    case MD_SPAN_EM: r.html += "</em>"; break;
    case MD_SPAN_STRONG: r.html += "</strong>"; break;
    case MD_SPAN_CODE: r.html += "</code>"; break;
    case MD_SPAN_DEL: r.html += "</del>"; break;
    case MD_SPAN_A: r.html += r.links.takeLast(); break;
    default: break;
    }
    return 0;
}

int text(MD_TEXTTYPE type, const MD_CHAR *value, MD_SIZE size, void *data) {
    auto &r = *static_cast<Renderer *>(data);
    const auto content = utf8(value, size);
    if (r.imageDepth) {
        r.imageAlt += type == MD_TEXT_ENTITY ? entity(content) : type == MD_TEXT_NULLCHAR ? QString(QChar::ReplacementCharacter)
                    : type == MD_TEXT_BR || type == MD_TEXT_SOFTBR ? " " : content;
    } else if (type == MD_TEXT_HTML) {
        if (r.htmlBlock) r.rawBlock += content;
        else r.html += r.safeHtml(content);
    }
    else if (type == MD_TEXT_ENTITY) r.html += entity(content).toHtmlEscaped();
    else if (type == MD_TEXT_NULLCHAR) r.html += QChar::ReplacementCharacter;
    else if (type == MD_TEXT_BR) r.html += "<br>\n";
    else if (type == MD_TEXT_SOFTBR) r.html += '\n';
    else r.html += content.toHtmlEscaped();
    return 0;
}
}

QString PublishingHtml::body(const QString &expandedMarkdown, QString *error) {
    if (error) error->clear();
    Renderer renderer;
    // Reserve generated footnote targets against colliding heading names.
    auto notes = QRegularExpression("<a name=\"(ow-note-[0-9]+-[\\w-]+)\"></a>").globalMatch(expandedMarkdown);
    while (notes.hasNext()) renderer.ids.insert(notes.next().captured(1));
    const auto input = expandedMarkdown.toUtf8();
    MD_PARSER parser = {};
    parser.flags = MD_DIALECT_GITHUB;
    parser.enter_block = enterBlock; parser.leave_block = leaveBlock;
    parser.enter_span = enterSpan; parser.leave_span = leaveSpan; parser.text = text;
    if (md_parse(input.constData(), MD_SIZE(input.size()), &parser, &renderer) != 0) {
        if (error) *error = QStringLiteral("The Markdown document could not be rendered for publishing.");
        return {};
    }
    return renderer.html;
}
