#include <QScopeGuard>
#include <QCryptographicHash>
#include <QTextFragment>
#include <QJsonArray>
#include <QPdfWriter>
#include <QPageSetupDialog>
#include "markdownextensions.h"
#include <QTextBoundaryFinder>
#include "backend.h"

#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QDate>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QMimeData>
#include <QProcess>
#include <QPrintDialog>
#include <QPrinter>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QUrl>
#include <QVariantMap>
#include <QWindow>

#include <algorithm>

#include "markdownhighlighter.h"

constexpr qreal typoraLineHeightPercent = 140;
const QString lastSaveDirectorySetting = QStringLiteral("file/lastSaveDirectory");

static QString authorshipPath(const QUrl &url) {
    const QFileInfo info(url.toLocalFile());
    return info.absoluteDir().filePath("." + info.fileName() + ".omawrite-authors.json");
}

// Stage metadata before changing the Markdown path. Failure leaves the original
// pair intact; an abandoned operation removes only its exclusively created copy.
class StagedAuthorship {
public:
    ~StagedAuthorship() { if (created) QFile::remove(destination); }
    bool prepare(const QUrl &from, const QUrl &to, const QByteArray *replacement = nullptr) {
        source = authorshipPath(from);
        destination = authorshipPath(to);
        if (QFileInfo::exists(destination) || QFileInfo(destination).isSymLink()) return false;
        if (replacement) bytes = *replacement;
        else {
            const QFileInfo info(source);
            if (info.isSymLink()) return false;
            if (!info.exists()) return true;
            QFile input(source);
            if (!info.isFile() || !input.open(QIODevice::ReadOnly) || input.size() > 10 * 1024 * 1024) return false;
            bytes = input.readAll();
            if (input.error() != QFileDevice::NoError) return false;
        }
        QFile output(destination);
        if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return false;
        created = true;
        return output.write(bytes) == bytes.size() && output.flush();
    }
    bool finish(bool removeSource) {
        if (!created) return true;
        created = false;
        if (!removeSource) return true;
        // Preserve a sidecar changed by another writer during the operation.
        QFile input(source);
        if (QFileInfo(source).isSymLink() || !input.open(QIODevice::ReadOnly)) return false;
        const bool unchanged = input.readAll() == bytes && input.error() == QFileDevice::NoError;
        input.close();
        return unchanged && input.remove();
    }
private:
    QString source, destination;
    QByteArray bytes;
    bool created = false;
};

QVariantMap Backend::resolveOpenPath(const QString &input) const {
    const auto error = [](const QString &message) { return QVariantMap{{"error", message}}; };
    QString path = input.trimmed();
    if (path.size() >= 2 && ((path.startsWith('"') && path.endsWith('"'))
            || (path.startsWith('\'') && path.endsWith('\''))))
        path = path.mid(1, path.size() - 2);
    if (path.isEmpty()) return error(QStringLiteral("Enter a file or folder path."));
    if (path.contains(QChar::Null) || path.contains('\n') || path.contains('\r'))
        return error(QStringLiteral("Enter one path on a single line."));
    if (path.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        const QUrl url(path, QUrl::StrictMode);
        if (!url.isValid() || !url.isLocalFile() || !url.host().isEmpty()
                || url.hasQuery() || url.hasFragment())
            return error(QStringLiteral("Use a local file URL without a hostname, query or fragment."));
        path = url.toLocalFile();
    }
    if (path == "~") path = QDir::homePath();
    else if (path.startsWith("~/")) path = QDir::homePath() + path.mid(1);
    if (!QDir::isAbsolutePath(path))
        return error(QStringLiteral("Use an absolute path, ~/path, or a file:/// URL."));
    const QFileInfo info(path);
    if (!info.exists()) return error(QStringLiteral("That file or folder does not exist."));
    if (!info.isReadable()) return error(QStringLiteral("That file or folder is not readable."));
    if (!info.isDir() && (!info.isFile() || !FileLibrary::isTextFile(path)))
        return error(QStringLiteral("Choose a folder or a Markdown/text file (.md, .markdown, .mdown, .txt, .text)."));
    return {{"url", QUrl::fromLocalFile(info.canonicalFilePath())}, {"folder", info.isDir()}};
}

QString Backend::normalizedLinkUrl(const QString &clipboardText) {
    QString candidate = clipboardText.trimmed();
    static const QRegularExpression lineBreakRe(QStringLiteral("[\\r\\n]"));
    const int lineBreak = candidate.indexOf(lineBreakRe);
    if (lineBreak >= 0)
        candidate = candidate.left(lineBreak).trimmed();

    if (candidate.isEmpty())
        return {};

    if (candidate.startsWith(QStringLiteral("www."), Qt::CaseInsensitive))
        candidate.prepend(QStringLiteral("https://"));

    static const QRegularExpression schemeRe(
        QStringLiteral("^[A-Za-z][A-Za-z0-9+.-]*:"));
    if (!schemeRe.match(candidate).hasMatch())
        return {};

    const QUrl url(candidate);
    if (!url.isValid() || url.scheme().isEmpty())
        return {};

    const QString scheme = url.scheme().toLower();
    const bool webUrl = scheme == QStringLiteral("http")
        || scheme == QStringLiteral("https")
        || scheme == QStringLiteral("ftp");
    if (webUrl && url.host().isEmpty())
        return {};

    if (!webUrl && scheme != QStringLiteral("mailto"))
        return {};

    return url.toString();
}

Backend::Backend(QObject *parent) : QObject(parent), m_library(this) {
    connect(&m_library, &FileLibrary::rootFolderChanged, this, &Backend::fileUrlChanged);
    const QString stateDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(stateDirectory);
    // Claim an orphaned snapshot before taking an empty slot. This ensures a
    // crash in window 2 is still recovered even if window 1 exited normally.
    for (int pass = 0; pass < 2 && !m_recoveryLock; ++pass) {
        for (int slot = 0; slot < 100; ++slot) {
            const QString base = QDir(stateDirectory).filePath(
                QStringLiteral("recovery-%1").arg(slot));
            const bool snapshotExists = QFileInfo::exists(base + QStringLiteral(".json"));
            if ((pass == 0) != snapshotExists)
                continue;
            auto lock = std::make_unique<QLockFile>(base + QStringLiteral(".lock"));
            if (lock->tryLock()) {
                m_recoveryPath = base + QStringLiteral(".json");
                m_recoveryLock = std::move(lock);
                break;
            }
        }
    }
    m_wordCountTimer.setSingleShot(true);
    m_wordCountTimer.setInterval(120);
    connect(&m_wordCountTimer, &QTimer::timeout, this, &Backend::refreshWordCount);
    m_recoveryTimer.setSingleShot(true);
    m_recoveryTimer.setInterval(750);
    connect(&m_recoveryTimer, &QTimer::timeout, this, &Backend::writeRecovery);
    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString &path) {
                if (path != m_fileUrl.toLocalFile())
                    return;

                const bool deleted = !QFileInfo::exists(path);
                if (!deleted && m_hasKnownFileContents) {
                    QFile file(path);
                    if (file.open(QIODevice::ReadOnly)
                            && file.readAll() == m_lastKnownFileContents) {
                        // Atomic saves can replace the watched inode. Re-arm the
                        // watcher, but do not report our own save as an outside edit.
                        watchCurrentFile();
                        return;
                    }
                }

                emit externalChangeDetected(deleted, m_modified);
            });

    const auto preset = QSettings().value("appearance/theme", "system").toString();
    if (QStringList{"system", "light", "dark", "paper"}.contains(preset)) m_themePreset = preset;
    loadOmarchyTheme();
    watchOmarchyTheme();
    connect(&m_themeWatcher, &QFileSystemWatcher::fileChanged, this, [this]() {
        loadOmarchyTheme();
        watchOmarchyTheme();
    });
    connect(&m_themeWatcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        loadOmarchyTheme();
        watchOmarchyTheme();
    });
}

Backend::~Backend() = default;

int Backend::documentRevision() const { return m_document ? m_document->revision() : 0; }

QUrl Backend::documentBaseUrl() const {
    const QString path = m_fileUrl.isLocalFile() ? QFileInfo(m_fileUrl.toLocalFile()).absolutePath()
        : m_library.rootFolder().toLocalFile();
    return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path + QDir::separator());
}

void Backend::setParentWindow(QWindow *window) {
    m_parentWindow = window;
}

QString Backend::fileName() const {
    if (!m_fileUrl.isValid() || m_fileUrl.isEmpty())
        return QStringLiteral("Untitled.md");

    if (m_fileUrl.isLocalFile()) {
        const QFileInfo info(m_fileUrl.toLocalFile());
        if (!info.fileName().isEmpty())
            return info.fileName();
    }

    const QString name = m_fileUrl.fileName();
    return name.isEmpty() ? QStringLiteral("Untitled.md") : name;
}

void Backend::setDarkMode(bool darkMode) {
    m_systemDarkMode = darkMode;
    loadOmarchyTheme();
}

void Backend::setThemePreset(const QString &preset) {
    if (!QStringList{"system", "light", "dark", "paper"}.contains(preset) || m_themePreset == preset) return;
    m_themePreset = preset;
    QSettings().setValue("appearance/theme", preset);
    loadOmarchyTheme();
    emit themePresetChanged();
}

QVariantMap Backend::palette() const {
    const bool paper = m_themePreset == "paper";
    return {{"page", m_themeBackground}, {"text", m_themeForeground},
        {"panel", m_darkMode ? "#222428" : paper ? "#f0e8d8" : "#fafaf9"},
        {"muted", m_darkMode ? "#afb5bf" : paper ? "#706451" : "#616975"},
        {"border", m_darkMode ? "#50545c" : paper ? "#c9bda7" : "#d5d7da"},
        {"hover", m_darkMode ? "#34383f" : paper ? "#e5dac4" : "#e8eaed"},
        {"field", m_darkMode ? "#2b2e34" : paper ? "#eae0cd" : "#eff0f2"},
        {"focus", m_darkMode ? "#9ec5ff" : "#285e9e"},
        {"folder", m_darkMode ? "#63c9f1" : "#087fa9"},
        {"selection", m_themeSelection}};
}

void Backend::setTextScale(qreal textScale) {
    if (qFuzzyCompare(m_textScale, textScale))
        return;

    m_textScale = textScale;
    emit textScaleChanged();
}

QVariantList Backend::documentOutline(const QString &markdown) const {
    QVariantList headings;
    const QStringList lines = markdown.split('\n');
    int position = 0;
    QChar fence;
    int fenceLength = 0;
    bool frontMatter = false;
    if (!lines.isEmpty() && lines.first().trimmed() == "---") {
        for (int i = 1; i < lines.size(); ++i)
            if (lines[i].trimmed() == "---" || lines[i].trimmed() == "...") { frontMatter = true; break; }
    }
    static const QRegularExpression atx("^ {0,3}(#{1,6})[ \t]+(.+)$");
    static const QRegularExpression delimiter("^ {0,3}(`{3,}|~{3,})(.*)$");
    static const QRegularExpression setext("^ {0,3}(=+|-+)[ \t]*$");
    for (int index = 0; index < lines.size(); ++index) {
        const QString &line = lines[index];
        if (frontMatter) {
            if (index > 0 && (line.trimmed() == "---" || line.trimmed() == "...")) frontMatter = false;
            position += line.size() + 1;
            continue;
        }
        const auto marker = delimiter.match(line);
        if (marker.hasMatch()) {
            const QString sequence = marker.captured(1);
            if (fence.isNull()) { fence = sequence.at(0); fenceLength = sequence.size(); }
            else if (sequence.at(0) == fence && sequence.size() >= fenceLength && marker.captured(2).trimmed().isEmpty()) fence = QChar();
        } else if (fence.isNull()) {
            const auto heading = atx.match(line);
            if (heading.hasMatch()) {
                QString title = heading.captured(2).trimmed();
                title.remove(QRegularExpression("[ \t]+#+$"));
                headings.append(QVariantMap{{"title", title}, {"level", heading.captured(1).size()}, {"position", position}});
            } else if (!line.trimmed().isEmpty() && index + 1 < lines.size() && setext.match(lines[index + 1]).hasMatch()
                       && !line.startsWith("    ") && !line.startsWith('>')) {
                headings.append(QVariantMap{{"title", line.trimmed()}, {"level", lines[index + 1].trimmed().startsWith('=') ? 1 : 2}, {"position", position}});
                position += line.size() + 1;
                ++index;
                position += lines[index].size() + 1;
                continue;
            }
        }
        position += line.size() + 1;
    }
    return headings;
}

QVariantMap Backend::documentStatistics(const QString &markdown) const {
    QTextDocument rendered;
    rendered.setMarkdown(markdown);
    const QString plain = rendered.toPlainText();
    const int words = countWords(plain);
    QString compact = plain;
    compact.remove(QRegularExpression("\\s"));
    return {{"words", words}, {"characters", plain.toUcs4().size()},
            {"charactersWithoutSpaces", compact.toUcs4().size()},
            {"readingMinutes", words == 0 ? 0 : qMax(1, (words + 199) / 200)}};
}

QVariantMap Backend::replaceText(int start, int end, const QString &replacement) {
    if (!m_document) return {};
    const int length = currentDocumentText().size();
    const int first = qBound(0, qMin(start, end), length);
    const int last = qBound(first, qMax(start, end), length);
    QString normalized = replacement;
    normalized.replace("\r\n", "\n").replace('\r', '\n');
    QTextCursor cursor(m_document);
    cursor.setPosition(first);
    cursor.setPosition(last, QTextCursor::KeepAnchor);
    cursor.beginEditBlock();
    cursor.insertText(normalized);
    cursor.endEditBlock();
    return {{"start", first}, {"end", first + normalized.size()}};
}

QVariantMap Backend::wrapSelection(int start, int end, const QString &before, const QString &after) {
    if (!m_document) return {};
    const QString text = currentDocumentText();
    int first = qBound(0, qMin(start, end), int(text.size()));
    int last = qBound(first, qMax(start, end), int(text.size()));
    QString selected = text.mid(first, last - first);
    auto escaped = [&](int position) {
        int slashes = 0;
        while (position > 0 && text.at(--position) == '\\') ++slashes;
        return slashes % 2 != 0;
    };
    if (before == "`" && after == "`") {
        int left = first, right = last;
        if (left > 0 && text.at(left - 1) == ' ' && right < text.size() && text.at(right) == ' ') { --left; ++right; }
        const int contentLeft = left, contentRight = right;
        while (left > 0 && text.at(left - 1) == '`') --left;
        while (right < text.size() && text.at(right) == '`') ++right;
        if (contentLeft - left > 0 && contentLeft - left == right - contentRight && !escaped(left))
            return replaceText(left, right, selected);
        int leading = 0, trailing = 0;
        while (leading < selected.size() && selected.at(leading) == '`') ++leading;
        while (trailing < selected.size() && selected.at(selected.size() - trailing - 1) == '`') ++trailing;
        if (leading > 0 && leading == trailing && selected.size() > leading * 2) {
            QString content = selected.mid(leading, selected.size() - leading * 2);
            if (content.startsWith(' ') && content.endsWith(' ') && !content.trimmed().isEmpty()) content = content.mid(1, content.size() - 2);
            return replaceText(first, last, content);
        }
    }
    if (before != "`" && selected.size() >= before.size() + after.size() && selected.startsWith(before)
            && selected.endsWith(after) && !escaped(first) && !escaped(last - after.size())) {
        selected = selected.mid(before.size(), selected.size() - before.size() - after.size());
        return replaceText(first, last, selected);
    }
    if (before != "`" && first >= before.size() && text.mid(first - before.size(), before.size()) == before
            && text.mid(last, after.size()) == after && !escaped(first - before.size()) && !escaped(last)) {
        return replaceText(first - before.size(), last + after.size(), selected);
    }
    QString opening = before, closing = after;
    if (before == "`" && after == "`") {
        int longest = 0, run = 0;
        for (const QChar c : selected) { run = c == '`' ? run + 1 : 0; longest = qMax(longest, run); }
        opening = closing = QString(longest + 1, '`');
        if (!selected.isEmpty() && (selected.front() == '`' || selected.back() == '`'
            || (selected.front() == ' ' && selected.back() == ' ' && !selected.trimmed().isEmpty()))) {
            opening += ' '; closing.prepend(' ');
        }
    }
    replaceText(first, last, opening + selected + closing);
    return {{"start", first + opening.size()}, {"end", first + opening.size() + selected.size()}};
}

QVariantMap Backend::editMarkdown(const QString &action, int start, int end) {
    if (!m_document) return {};
    const QString text = currentDocumentText();
    int first = qBound(0, qMin(start, end), int(text.size()));
    int last = qBound(first, qMax(start, end), int(text.size()));
    if (action == "date") return replaceText(first, last, QDate::currentDate().toString(Qt::ISODate));
    if (action == "table") {
        QString label = text.mid(first, last - first);
        label.replace("|", "\\|").replace('\n', ' ');
        if (label.isEmpty()) label = "Column 1";
        QString table = "| " + label + " | Column 2 |\n| --- | --- |\n|  |  |\n";
        if (first > 0 && text.at(first - 1) != '\n') table.prepend("\n\n");
        if (last < text.size() && text.at(last) != '\n') table += '\n';
        return replaceText(first, last, table);
    }
    if (action == "uppercase" || action == "lowercase" || action == "titlecase" || action == "clearInline") {
        if (first == last) { setStatus(QStringLiteral("Select text first.")); return {}; }
        QString selected = text.mid(first, last - first);
        if (selected.contains('`') || selected.contains("](") || selected.contains("][") || selected.contains("://")) {
            setStatus(QStringLiteral("Select plain text without code or link destinations for this command."));
            return {};
        }
        if (action == "uppercase") selected = selected.toUpper();
        else if (action == "lowercase") selected = selected.toLower();
        else if (action == "titlecase") {
            selected = selected.toLower();
            const QRegularExpression word(QStringLiteral("\\b\\p{L}[\\p{L}\\p{M}]*"), QRegularExpression::UseUnicodePropertiesOption);
            QList<QRegularExpressionMatch> words;
            auto matches = word.globalMatch(selected);
            while (matches.hasNext()) words.append(matches.next());
            for (auto it = words.crbegin(); it != words.crend(); ++it) {
                const int index = it->capturedStart();
                const int length = selected.at(index).isHighSurrogate() ? 2 : 1;
                selected.replace(index, length, selected.mid(index, length).toUpper());
            }
        } else {
            // Deliberately limited to selected, surrounding emphasis/strike pairs.
            bool stripped;
            do {
                stripped = false;
                for (const QString &marker : {QString("**"), QString("__"), QString("~~"), QString("*"), QString("_")}) {
                    if (selected.size() > marker.size() * 2 && selected.startsWith(marker) && selected.endsWith(marker)
                            && selected.at(selected.size() - marker.size() - 1) != '\\') {
                        selected = selected.mid(marker.size(), selected.size() - marker.size() * 2);
                        stripped = true;
                        break;
                    }
                }
            } while (stripped);
        }
        return replaceText(first, last, selected);
    }
    if (action == "codeBlock" || action == "rule") {
        const QString selected = text.mid(first, last - first);
        QString block;
        if (action == "codeBlock") {
            int longest = 2, run = 0;
            for (const QChar c : selected) { run = c == '`' ? run + 1 : 0; longest = qMax(longest, run); }
            const QString fence(longest + 1, '`');
            block = fence + "\n" + selected + (selected.endsWith('\n') ? "" : "\n") + fence;
        } else block = "---";
        if (first > 0) block.prepend(text.left(first).endsWith("\n\n") ? "" : text.at(first - 1) == '\n' ? "\n" : "\n\n");
        if (last < text.size()) block += text.mid(last).startsWith("\n\n") ? "" : text.at(last) == '\n' ? "\n" : "\n\n";
        else block += "\n";
        return replaceText(first, last, block);
    }
    const int lineStart = (first == 0 ? 0 : text.lastIndexOf('\n', first - 1) + 1);
    const int probe = last > first ? last - 1 : last;
    int lineEnd = text.indexOf('\n', probe);
    if (lineEnd < 0) lineEnd = text.size();
    first = first == 0 ? 0 : lineStart;
    last = lineEnd;
    // Refuse syntax transformations on fenced code rather than corrupting it.
    QChar fence;
    int fenceLength = 0;
    int offset = 0;
    const QRegularExpression fencePattern(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    for (const QString &line : text.split('\n')) {
        const auto match = fencePattern.match(line);
        const bool wasInside = !fence.isNull();
        if (match.hasMatch()) {
            const QString marker = match.captured(1);
            if (fence.isNull()) { fence = marker.front(); fenceLength = marker.size(); }
            else if (marker.front() == fence && marker.size() >= fenceLength && match.captured(2).trimmed().isEmpty()) fence = QChar();
        }
        if (offset <= last && offset + line.size() >= first && (wasInside || match.hasMatch())) {
            setStatus(QStringLiteral("Block formatting is unavailable inside fenced code."));
            return {};
        }
        offset += line.size() + 1;
    }
    QStringList lines = text.mid(first, last - first).split('\n');
    QString replacement;
    int movedStart = -1, movedEnd = -1;
    if (action == "lineUp" || action == "lineDown") {
        const QString selected = lines.join('\n');
        if (action == "lineUp") {
            if (first == 0) return {};
            const int previous = first > 1 ? text.lastIndexOf('\n', first - 2) + 1 : 0;
            const QString adjacent = text.mid(previous, first - previous - 1);
            if (adjacent.trimmed().startsWith("```") || adjacent.trimmed().startsWith("~~~")) return {};
            replacement = selected + "\n" + adjacent;
            first = previous;
            movedStart = first;
            movedEnd = first + selected.size();
        } else {
            if (last == text.size()) return {};
            int nextEnd = text.indexOf('\n', last + 1);
            if (nextEnd < 0) nextEnd = text.size();
            const QString adjacent = text.mid(last + 1, nextEnd - last - 1);
            if (adjacent.trimmed().startsWith("```") || adjacent.trimmed().startsWith("~~~")) return {};
            replacement = adjacent + "\n" + selected;
            movedStart = first + adjacent.size() + 1;
            movedEnd = movedStart + selected.size();
            last = nextEnd;
        }
    } else {
        int number = 1;
        const QRegularExpression prefix(QStringLiteral("^( {0,3})(?:#{1,6} +|> +|[-+*] +(?:\\[[ xX]\\] +)?|[0-9]+[.)] +)"));
        for (QString &line : lines) {
            if (action == "indent") { line.prepend("    "); continue; }
            if (action == "outdent") {
                if (line.startsWith('\t')) line.remove(0, 1);
                else { int count = 0; while (count < 4 && count < line.size() && line.at(count) == ' ') ++count; line.remove(0, count); }
                continue;
            }
            if (line.trimmed().isEmpty() && lines.size() > 1) continue;
            int indentation = 0;
            while (indentation < line.size() && (line.at(indentation) == ' ' || line.at(indentation) == '\t')) ++indentation;
            const QString indent = line.left(indentation);
            QString body = line.mid(indentation);
            body.remove(prefix);
            QString marker;
            if (action.startsWith("heading")) {
                const int level = action.mid(7).toInt();
                if (level < 1 || level > 6) return {};
                marker = QString(level, '#') + " ";
            } else if (action == "bullet") marker = "- ";
            else if (action == "ordered") marker = QString::number(number++) + ". ";
            else if (action == "task") marker = "- [ ] ";
            else if (action == "quote") marker = "> ";
            else if (action == "toggleTask") {
                const QRegularExpression task(QStringLiteral("^([-+*] +)\\[([ xX])\\]"));
                const auto m = task.match(line.mid(indentation));
                if (m.hasMatch()) {
                    line[indentation + m.capturedStart(2)] = m.captured(2) == " " ? 'x' : ' ';
                    continue;
                }
                marker = "- [ ] ";
            } else if (action != "body") return {};
            line = indent + marker + body;
        }
        replacement = lines.join('\n');
    }
    if (replacement == text.mid(first, last - first)) return {{"start", start}, {"end", end}};
    QTextCursor cursor(m_document);
    cursor.setPosition(first);
    cursor.setPosition(last, QTextCursor::KeepAnchor);
    cursor.beginEditBlock();
    cursor.insertText(replacement);
    cursor.endEditBlock();
    if (start == end) {
        const int position = movedStart >= 0 ? movedStart + qMax(0, start - lineStart)
            : qBound(first, start + int(replacement.size()) - (last - first), first + int(replacement.size()));
        return {{"start", position}, {"end", position}};
    }
    return {{"start", movedStart >= 0 ? movedStart : first}, {"end", movedEnd >= 0 ? movedEnd : first + replacement.size()}};
}

QVariantList Backend::searchPositions(const QString &query) const {
    QVariantList positions;
    if (query.isEmpty()) return positions;
    const QString text = currentDocumentText();
    int position = 0;
    while ((position = text.indexOf(query, position, Qt::CaseInsensitive)) >= 0) {
        positions.append(position);
        position += query.size();
    }
    return positions;
}

int Backend::replaceMatches(const QString &query, const QString &replacement, int position) {
    if (!m_document || query.isEmpty()) return 0;
    const QVariantList matches = searchPositions(query);
    QString normalized = replacement;
    normalized.replace("\r\n", "\n").replace('\r', '\n');
    QTextCursor cursor(m_document);
    cursor.beginEditBlock();
    int count = 0;
    for (auto it = matches.crbegin(); it != matches.crend(); ++it) {
        const int start = it->toInt();
        if (position >= 0 && start != position) continue;
        cursor.setPosition(start);
        cursor.setPosition(start + query.size(), QTextCursor::KeepAnchor);
        cursor.insertText(normalized);
        ++count;
    }
    cursor.endEditBlock();
    return count;
}

static QString headingSlug(const QString &title) {
    QTextDocument plain;
    plain.setMarkdown(title);
    QString slug = plain.toPlainText().toLower().trimmed();
    slug.remove(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}_\\s-]")));
    slug.replace(QRegularExpression("\\s+"), "-");
    return slug.isEmpty() ? QStringLiteral("section") : slug;
}

QString Backend::tableOfContents(const QString &markdown) const {
    QString result;
    QHash<QString, int> occurrences;
    for (const auto &value : documentOutline(markdown)) {
        const auto heading = value.toMap();
        QString title = heading["title"].toString();
        QString slug = headingSlug(title);
        int count = occurrences[slug]++;
        if (count) slug += "-" + QString::number(count);
        title.replace("[", "\\[").replace("]", "\\]");
        result += QString((heading["level"].toInt() - 1) * 2, ' ') + "- [" + title + "](#" + slug + ")\n";
    }
    return result;
}

int Backend::previewAnchorPosition(QObject *textDocument, const QString &anchor) const {
    auto *quick = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quick) return -1;
    QHash<QString, int> occurrences;
    for (auto block = quick->textDocument()->begin(); block.isValid(); block = block.next()) {
        if (!block.blockFormat().headingLevel()) continue;
        QString slug = headingSlug(block.text());
        int count = occurrences[slug]++;
        if (count) slug += "-" + QString::number(count);
        if (slug == anchor) return block.position();
    }
    return -1;
}

void Backend::stylePreview(QObject *textDocument) {
    auto *quick = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quick || !quick->textDocument() || quick->textDocument() == m_document) return;
    QTextDocument *preview = quick->textDocument();
    preview->setUndoRedoEnabled(false);
    for (QTextBlock block = preview->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat format = block.blockFormat();
        format.setLineHeight(135, QTextBlockFormat::ProportionalHeight);
        format.setTopMargin(format.headingLevel() > 0 && block.blockNumber() > 0 ? 18 : 0);
        format.setBottomMargin(block.textList() ? 4 : 12);
        QTextCursor cursor(block);
        cursor.setBlockFormat(format);
        if (format.headingLevel() > 0) {
            QTextCharFormat heading;
            // Qt Quick applies its pixel font separately from the document default.
            // Relative adjustment follows the actual preview font at every size.
            heading.setProperty(QTextFormat::FontSizeAdjustment, format.headingLevel() == 1 ? 3 : format.headingLevel() == 2 ? 2 : 0);
            heading.setFontWeight(QFont::Bold);
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.mergeCharFormat(heading);
        }
    }
}

QPair<int, int> Backend::sentenceRange(const QString &text, int position) {
    position = qBound(0, position, int(text.size()));
    QTextBoundaryFinder finder(QTextBoundaryFinder::Sentence, text);
    finder.setPosition(position);
    int start = finder.isAtBoundary() ? position : finder.toPreviousBoundary();
    if (start == text.size() && start > 0) start = finder.toPreviousBoundary();
    start = qMax(0, start);
    finder.setPosition(start);
    int end = finder.toNextBoundary();
    return {start, end < 0 ? int(text.size()) : end};
}

void Backend::setFocusPosition(int position, bool enabled, bool sentence) {
    if (!m_document || !m_highlighter) return;
    const auto block = m_document->findBlock(position);
    if (!enabled && !sentence) { m_highlighter->setFocusRange(-1, -1); return; }
    // Keep fenced code as a whole line; sentence boundaries are Unicode rules,
    // not a language-specific grammar or abbreviation model.
    if (sentence && block.userState() <= 0 && !block.text().trimmed().startsWith("```")) {
        const auto range = sentenceRange(block.text(), position - block.position());
        m_highlighter->setFocusRange(block.position() + range.first, block.position() + range.second);
    } else m_highlighter->setFocusBlock(block.blockNumber());
}

void Backend::setShowMarkup(bool show) {
    m_showMarkup = show;
    if (m_highlighter) m_highlighter->setShowMarkup(show);
}

QUrl Backend::resolveDocumentLink(const QString &link) const {
    return documentBaseUrl().resolved(QUrl(link));
}

void Backend::attachDocument(QObject *textDocument) {
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quickDocument || !quickDocument->textDocument()) {
        setStatus(QStringLiteral("Could not attach the Markdown renderer."));
        return;
    }

    if (m_highlighter)
        delete m_highlighter.data();

    m_document = quickDocument->textDocument();
    m_lastDocumentText = m_document->toPlainText();
    m_highlighter = new MarkdownHighlighter(m_document);
    m_highlighter->setDarkMode(m_darkMode);
    m_highlighter->setShowMarkup(m_showMarkup);
    m_highlighter->setColors(m_themeBackground, m_themeForeground, m_themeAccent);

    connect(m_document, &QTextDocument::contentsChange, this,
            [this](int position, int, int charsAdded) {
                if (m_formattingTypography || m_loading)
                    return;
                m_lastChangePos = position;
                m_lastChangeAdded = charsAdded;
            });

    applyDocumentTypography();
    restoreRecovery();
}

void Backend::openDialog() {
    emit openDialogRequested();
}

bool Backend::open(const QUrl &url) {
    if (focusExistingDocument && focusExistingDocument(url)) return false;
    if (!url.isLocalFile()) {
        setStatus(QStringLiteral("Only local files can be opened."));
        return false;
    }

    const QString targetName = QFileInfo(url.toLocalFile()).fileName();
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not open %1.").arg(targetName));
        return false;
    }

    const QByteArray contents = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setStatus(QStringLiteral("Could not finish reading the document."));
        return false;
    }
    loadDocumentText(QString::fromUtf8(contents));
    clearRecovery();
    m_lastKnownFileContents = contents;
    m_hasKnownFileContents = true;
    setFileUrl(url);
    loadAuthorship(url);
    m_library.recordRecentFile(url);
    if (m_library.rootFolder().isEmpty())
        m_library.setRootFolder(QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absolutePath()));
    m_library.revealFile(url);
    watchCurrentFile();
    setModified(false);
    setStatus(QStringLiteral("Opened %1").arg(fileName()));
    if (!m_navigatingHistory && (m_historyIndex < 0 || m_history[m_historyIndex].first != url)) {
        while (m_history.size() > m_historyIndex + 1) m_history.removeLast();
        m_history.append({url, 0});
        m_historyIndex = m_history.size() - 1;
        emit historyChanged();
    }
    return true;
}

void Backend::rememberCursor(int position) {
    if (m_historyIndex >= 0 && m_history[m_historyIndex].first == m_fileUrl)
        m_history[m_historyIndex].second = qMax(0, position);
}

int Backend::navigateHistory(int direction) {
    if (direction != -1 && direction != 1) return -1;
    const int target = m_historyIndex + direction;
    if (target < 0 || target >= m_history.size()) return -1;
    const auto entry = m_history.at(target);
    m_navigatingHistory = true;
    const bool loaded = open(entry.first);
    m_navigatingHistory = false;
    if (!loaded) return -1;
    m_historyIndex = target;
    emit historyChanged();
    return qMin(entry.second, int(currentDocumentText().size()));
}

QUrl Backend::sourceLinkAt(int position) const {
    const QString text = currentDocumentText();
    // Ordinary inline Markdown destinations and autolinks. Reference links and
    // destinations with nested parentheses remain explicit future work.
    const QRegularExpression link(QStringLiteral(R"link((?<!!)\[[^\]\n]*\]\(([^\s)]+)\)|<(https?://[^>]+|mailto:[^>]+)>)link"));
    auto matches = link.globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        if (position < match.capturedStart() || position >= match.capturedEnd()) continue;
        const QString destination = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        if (destination.contains('(')) return {};
        const QUrl resolved = resolveDocumentLink(destination);
        if (resolved.isLocalFile() || resolved.scheme() == "http" || resolved.scheme() == "https" || resolved.scheme() == "mailto") return resolved;
    }
    return {};
}

void Backend::save() {
    if (!m_fileUrl.isValid() || m_fileUrl.isEmpty()) {
        saveAsDialog();
        return;
    }

    saveTo(m_fileUrl);
}

void Backend::saveForClose() {
    if (!m_modified) {
        emit closeAfterSave();
        return;
    }

    m_closeAfterSave = true;
    save();
}

void Backend::saveAsDialog() {
    emit saveDialogRequested(suggestedSaveUrl());
}

void Backend::saveAs(const QUrl &url) {
    saveTo(url);
}

void Backend::fileDialogCanceled() {
    m_closeAfterSave = false;
}

void Backend::discardRecovery() {
    clearRecovery();
}

void Backend::reloadFromDisk() {
    if (m_fileUrl.isLocalFile())
        open(m_fileUrl);
}

void Backend::keepExternalVersion() {
    QFile file(m_fileUrl.toLocalFile());
    if (file.open(QIODevice::ReadOnly)) {
        m_lastKnownFileContents = file.readAll();
        m_hasKnownFileContents = true;
    } else {
        m_lastKnownFileContents.clear();
        m_hasKnownFileContents = false;
    }
    setModified(true);
    scheduleRecovery();
    watchCurrentFile();
    setStatus(QStringLiteral("Kept your version"));
}

void Backend::printDocument(bool plain) {
    if (!m_document) {
        setStatus(QStringLiteral("There is no document to print."));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    if (m_pageLayout.isValid()) printer.setPageLayout(m_pageLayout);
    QPrintDialog dialog(&printer);
    dialog.setWindowTitle(QStringLiteral("Print %1").arg(fileName()));
    dialog.winId();
    if (dialog.windowHandle() && m_parentWindow)
        dialog.windowHandle()->setTransientParent(m_parentWindow);

    if (dialog.exec() == QDialog::Accepted) {
        QTextDocument rendered;
        prepareOutput(rendered, plain);
        rendered.print(&printer);
    }
}

void Backend::newWindow() { emit newWindowRequested(QUrl()); }

void Backend::newDocument() {
    m_history.clear();
    m_historyIndex = -1;
    emit historyChanged();
    loadDocumentText(QString());
    clearRecovery();
    setFileUrl(QUrl());
    m_lastKnownFileContents.clear();
    m_hasKnownFileContents = false;
    watchCurrentFile();
    setModified(false);
    setStatus(QStringLiteral("New untitled document"));
}

static bool validDocumentName(const QString &name) {
    return !name.isEmpty() && name == name.trimmed() && name != "." && name != ".."
        && !name.contains('/') && !name.contains('\\') && !name.contains(QChar::Null)
        && !name.contains('\n') && !name.contains('\r') && FileLibrary::isTextFile(name);
}

bool Backend::duplicateDocument(const QString &name) {
    const QFileInfo original(m_fileUrl.toLocalFile());
    if (!m_document || !m_fileUrl.isLocalFile() || !original.isFile() || original.isSymLink()) {
        setStatus(QStringLiteral("Save to an available regular file before duplicating."));
        return false;
    }
    if (!validDocumentName(name)) {
        setStatus(QStringLiteral("Enter a filename ending in .md or .txt, without slashes or surrounding spaces."));
        return false;
    }
    const QUrl destinationUrl = QUrl::fromLocalFile(original.absoluteDir().filePath(name));
    StagedAuthorship metadata;
    const QByteArray annotations = QJsonDocument(authorshipData()).toJson(QJsonDocument::Compact);
    // Write an empty range list too: the duplicate describes the current buffer,
    // not potentially stale annotations belonging to the saved original.
    if (!metadata.prepare(m_fileUrl, destinationUrl, &annotations)) {
        setStatus(QStringLiteral("Could not create authorship metadata. The destination may already be in use."));
        return false;
    }
    QFile copy(destinationUrl.toLocalFile());
    if (!copy.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        setStatus(QStringLiteral("Could not create the copy. The name may already be in use."));
        return false;
    }
    const QByteArray contents = currentDocumentText().toUtf8();
    if (copy.write(contents) != contents.size() || !copy.flush()) {
        copy.close();
        copy.remove(); // Only the new file exclusively created by this operation.
        setStatus(QStringLiteral("Could not finish the copy. The original is unchanged."));
        return false;
    }
    copy.close();
    metadata.finish(false);
    m_library.recordRecentFile(QUrl::fromLocalFile(copy.fileName()));
    m_library.refresh();
    setStatus(QStringLiteral("Created %1; still editing %2").arg(name, fileName()));
    return true;
}

bool Backend::renameDocument(const QString &name) {
    const QFileInfo original(m_fileUrl.toLocalFile());
    if (!m_document || !m_fileUrl.isLocalFile() || !original.isFile() || original.isSymLink()) {
        setStatus(QStringLiteral("Save to an available regular file before renaming."));
        return false;
    }
    if (!validDocumentName(name)) {
        setStatus(QStringLiteral("Enter a filename ending in .md or .txt, without slashes or surrounding spaces."));
        return false;
    }
    if (name == original.fileName()) return true;
    const QString destination = original.absoluteDir().filePath(name);
    const QFileInfo target(destination);
    if (target.exists() || target.isSymLink()) {
        setStatus(QStringLiteral("That name is already in use. Choose another name."));
        return false;
    }
    QFile source(original.absoluteFilePath());
    if (!source.open(QIODevice::ReadOnly) || !m_hasKnownFileContents
            || source.readAll() != m_lastKnownFileContents) {
        setStatus(QStringLiteral("The file changed on disk. Resolve the external change before renaming."));
        return false;
    }
    source.close();
    const QUrl oldUrl = QUrl::fromLocalFile(original.canonicalFilePath());
    StagedAuthorship metadata;
    if (!metadata.prepare(m_fileUrl, QUrl::fromLocalFile(destination))) {
        setStatus(QStringLiteral("Could not preserve authorship metadata. Nothing was renamed."));
        return false;
    }
    if (!source.rename(destination)) {
        setStatus(QStringLiteral("Could not rename the file. Check the name and folder permissions."));
        return false;
    }
    const QUrl newUrl = QUrl::fromLocalFile(destination);
    setFileUrl(newUrl);
    m_library.renamedFile(oldUrl, newUrl);
    for (auto &entry : m_history) if (entry.first == oldUrl) entry.first = newUrl;
    if (m_modified) writeRecovery();
    else clearRecovery();
    const bool metadataCleaned = metadata.finish(true);
    setStatus(QStringLiteral("Renamed to %1%2%3").arg(name,
        m_modified ? QStringLiteral(" — unsaved edits retained") : QString(),
        metadataCleaned ? QString() : QStringLiteral(" — original metadata retained; cleanup needed")));
    return true;
}

bool Backend::moveDocument(const QUrl &folder) {
    const QFileInfo original(m_fileUrl.toLocalFile());
    if (!m_document || !m_fileUrl.isLocalFile() || !original.isFile() || original.isSymLink()) {
        setStatus(QStringLiteral("Save to an available regular file before moving."));
        return false;
    }
    const QFileInfo directory(folder.toLocalFile());
    if (!folder.isLocalFile() || !directory.isDir()) {
        setStatus(QStringLiteral("Choose an existing local destination folder."));
        return false;
    }
    const QString sourcePath = original.canonicalFilePath();
    const QString destination = QDir(directory.canonicalFilePath()).filePath(original.fileName());
    if (destination == sourcePath) {
        setStatus(QStringLiteral("The document is already in that folder."));
        return true;
    }
    const QFileInfo target(destination);
    if (target.exists() || target.isSymLink()) {
        setStatus(QStringLiteral("That folder already contains this filename. Nothing was moved."));
        return false;
    }
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly) || !m_hasKnownFileContents
            || source.readAll() != m_lastKnownFileContents || source.error() != QFileDevice::NoError) {
        setStatus(QStringLiteral("The file changed or could not be read. Resolve it before moving."));
        return false;
    }
    source.close();
    StagedAuthorship metadata;
    if (!metadata.prepare(m_fileUrl, QUrl::fromLocalFile(destination))) {
        setStatus(QStringLiteral("Could not preserve authorship metadata. Nothing was moved."));
        return false;
    }
    // QFile::copy refuses existing destinations and preserves permissions. Use
    // the same verified-copy path across volumes; never remove an unverified source.
    if (!source.copy(destination)) {
        setStatus(QStringLiteral("Could not copy to that folder. The original is unchanged."));
        return false;
    }
    QFile copy(destination);
    const bool copyVerified = !QFileInfo(destination).isSymLink() && copy.open(QIODevice::ReadOnly)
        && copy.readAll() == m_lastKnownFileContents && copy.error() == QFileDevice::NoError;
    copy.close();
    const bool sourceVerified = !QFileInfo(sourcePath).isSymLink()
        && source.open(QIODevice::ReadOnly) && source.readAll() == m_lastKnownFileContents
        && source.error() == QFileDevice::NoError;
    source.close();
    if (!copyVerified || !sourceVerified) {
        metadata.finish(false);
        setStatus(QStringLiteral("Move verification failed. The original remains active; a copy may remain in the destination folder."));
        return false;
    }
    if (!source.remove()) {
        metadata.finish(false);
        setStatus(QStringLiteral("Copied, but could not remove the original. The original remains active; both files remain."));
        return false;
    }
    const QUrl newUrl = QUrl::fromLocalFile(destination);
    setFileUrl(newUrl);
    m_library.renamedFile(QUrl::fromLocalFile(sourcePath), newUrl);
    for (auto &entry : m_history) if (entry.first.toLocalFile() == sourcePath) entry.first = newUrl;
    if (m_modified) writeRecovery();
    else clearRecovery();
    const bool metadataCleaned = metadata.finish(true);
    setStatus(QStringLiteral("Moved to %1%2%3").arg(directory.fileName(),
        m_modified ? QStringLiteral(" — unsaved edits retained") : QString(),
        metadataCleaned ? QString() : QStringLiteral(" — original metadata retained; cleanup needed")));
    return true;
}

bool Backend::openInNewWindow(const QUrl &url) {
    if (!url.isLocalFile() || !QFileInfo(url.toLocalFile()).isFile()) {
        setStatus(QStringLiteral("The file is no longer available."));
        return false;
    }
    emit newWindowRequested(url);
    return true;
}

bool Backend::showInFinder() {
    if (!m_fileUrl.isLocalFile() || !QFileInfo(m_fileUrl.toLocalFile()).isFile()) {
        setStatus(QStringLiteral("Save the document to an available local file first."));
        return false;
    }
#ifdef Q_OS_MACOS
    const bool shown = QProcess::startDetached(QStringLiteral("/usr/bin/open"),
                                              {QStringLiteral("-R"), m_fileUrl.toLocalFile()});
#else
    const bool shown = QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_fileUrl.toLocalFile()).absolutePath()));
#endif
    if (!shown) setStatus(QStringLiteral("Could not reveal the document."));
    return shown;
}

bool Backend::copySelection(int start, int end, const QString &format) {
    const QString text = currentDocumentText();
    const int first = qBound(0, qMin(start, end), int(text.size()));
    const int last = qBound(first, qMax(start, end), int(text.size()));
    if (first == last) return false;
    const QString selected = text.mid(first, last - first);
    auto *mime = new QMimeData;
    if (format == "markdown") {
        mime->setText(selected);
        mime->setData("text/markdown", selected.toUtf8());
    } else if (format == "html" || format == "formatted") {
        QTextDocument rendered;
        rendered.setMarkdown(selected);
        const QString html = rendered.toHtml();
        if (format == "html") mime->setText(html);
        else { mime->setText(rendered.toPlainText()); mime->setHtml(html); }
    } else { delete mime; return false; }
    QGuiApplication::clipboard()->setMimeData(mime);
    return true;
}

QString Backend::clipboardMarkdown() const {
    const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
    if (!mime) return {};
    if (mime->hasFormat("text/markdown")) return QString::fromUtf8(mime->data("text/markdown"));
    if (mime->hasHtml()) {
        QTextDocument document;
        document.setHtml(mime->html());
        return document.toMarkdown();
    }
    return mime->text();
}

QString Backend::clipboardUrl() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return {};

    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData)
        return {};

    if (mimeData->hasUrls()) {
        const QList<QUrl> urls = mimeData->urls();
        for (const QUrl &url : urls) {
            const QString normalized = normalizedLinkUrl(url.toString());
            if (!normalized.isEmpty())
                return normalized;
        }
    }

    if (!mimeData->hasText())
        return {};

    return normalizedLinkUrl(mimeData->text());
}

QString Backend::clipboardText() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return {};

    const QMimeData *mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasText() ? mimeData->text() : QString();
}

bool Backend::editorTextChanged() {
    if (m_loading || m_formattingTypography)
        return false;

    const QString text = currentDocumentText();
    if (text == m_lastDocumentText)
        return false;
    m_lastDocumentText = text;

    if (m_document) {
        const int blockCount = m_document->blockCount();
        if (blockCount > m_formattedBlockCount)
            reapplyTypographyToChange();
        m_formattedBlockCount = blockCount;
    }

    scheduleWordCount();
    setModified(true);
    setStatus(QStringLiteral("Unsaved"));
    scheduleRecovery();
    return true;
}

QVariantList Backend::hiddenRangesAt(int position) const {
    QVariantList ranges;
    if (!m_document || m_showMarkup)
        return ranges;

    const QTextBlock block =
        m_document->findBlock(qBound(0, position, m_document->characterCount() - 1));
    if (!block.isValid() || block.userState() > 0 || (block.previous().isValid() && block.previous().userState() > 0))
        return ranges;

    const int lineStart = block.position();
    QList<QPair<int, int>> spans;
    const QList<MarkdownHighlighter::InlineMarkup> markup =
        MarkdownHighlighter::inlineMarkup(block.text());
    for (const MarkdownHighlighter::InlineMarkup &item : markup) {
        for (const MarkdownHighlighter::Span &marker : item.markers) {
            spans.append({lineStart + marker.start,
                          lineStart + marker.start + marker.length});
        }
    }
    std::sort(spans.begin(), spans.end());

    for (const auto &span : spans) {
        ranges.append(QVariantMap{{QStringLiteral("start"), span.first},
                                  {QStringLiteral("end"), span.second}});
    }
    return ranges;
}

void Backend::setSearchHighlight(const QString &query, int currentMatchStart) {
    if (m_highlighter)
        m_highlighter->setSearch(query, currentMatchStart);
}

void Backend::openExternalUrl(const QUrl &url) {
    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")
            || scheme == QStringLiteral("mailto"))
        QDesktopServices::openUrl(url);
}

QVariantMap Backend::windowGeometry() const {
    QSettings settings;
    return {{QStringLiteral("x"), settings.value(QStringLiteral("window/x"), -1)},
            {QStringLiteral("y"), settings.value(QStringLiteral("window/y"), -1)},
            {QStringLiteral("width"), settings.value(QStringLiteral("window/width"), 1280)},
            {QStringLiteral("height"), settings.value(QStringLiteral("window/height"), 820)},
            {QStringLiteral("maximized"), settings.value(QStringLiteral("window/maximized"), false)}};
}

void Backend::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    if (!maximized) {
        settings.setValue(QStringLiteral("window/x"), x);
        settings.setValue(QStringLiteral("window/y"), y);
        settings.setValue(QStringLiteral("window/width"), width);
        settings.setValue(QStringLiteral("window/height"), height);
    }
    settings.setValue(QStringLiteral("window/maximized"), maximized);
}

void Backend::loadDocumentText(const QString &text) {
    if (!m_document) {
        setStatus(QStringLiteral("Could not attach the Markdown renderer."));
        return;
    }

    m_loading = true;
    m_requiresExplicitSave = false;
    m_document->setPlainText(text);
    applyAuthorshipData({});
    m_lastDocumentText = text;
    m_loading = false;

    applyDocumentTypography();
    m_wordCountTimer.stop();
    setWordCount(countWords(text));
    emit documentLoaded();
}

void Backend::setFileUrl(const QUrl &url) {
    if (m_fileUrl == url)
        return;

    m_fileUrl = url;
    emit fileUrlChanged();
    watchCurrentFile();
}

void Backend::setModified(bool modified) {
    if (m_modified == modified)
        return;

    m_modified = modified;
    emit modifiedChanged();
}

void Backend::setStatus(const QString &status) {
    if (m_status == status)
        return;

    m_status = status;
    emit statusChanged();
}

void Backend::saveTo(const QUrl &url, bool protectExternalChanges) {
    if (focusExistingDocument && focusExistingDocument(url)) {
        setStatus("That file is already open in another window. Choose a different path.");
        emit saveFailed(); emit quitCanceled(); return;
    }
    if (!url.isLocalFile()) {
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Only local files can be saved."));
        emit saveFailed();
        emit quitCanceled();
        return;
    }

    const QString targetName = QFileInfo(url.toLocalFile()).fileName();
#ifdef Q_OS_MACOS
    // Keep the previous saved bytes before replacing them. Failure blocks this
    // save rather than silently violating the requested history policy.
    if (QSettings().value("safety/automaticVersions", false).toBool() && QFileInfo::exists(url.toLocalFile())) {
        QFile prior(url.toLocalFile());
        if (!prior.open(QIODevice::ReadOnly)) {
            setStatus("Cannot read the previous file for version history.");
            emit saveFailed(); emit quitCanceled(); return;
        }
        const auto bytes = prior.readAll();
        if (prior.error() != QFile::NoError) { emit saveFailed(); emit quitCanceled(); return; }
        if (bytes != currentDocumentText().toUtf8()) {
            extern QString createMacVersion(const QString &);
            const auto error = createMacVersion(url.toLocalFile());
            if (!error.isEmpty()) {
                setStatus("Save paused: could not preserve the previous version. " + error);
                emit saveFailed(); emit quitCanceled(); return;
            }
        }
    }
#endif
    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Could not save %1.").arg(targetName));
        emit saveFailed();
        emit quitCanceled();
        return;
    }

    const QByteArray contents = currentDocumentText().toUtf8();
    if (file.write(contents) != contents.size()) {
        file.cancelWriting(); m_closeAfterSave = false;
        setStatus("Could not write complete document; original file retained.");
        emit saveFailed(); emit quitCanceled(); return;
    }
    // Recheck after staging, immediately before replacement. This narrows the
    // race with non-cooperating writers; no portable atomic compare-and-swap exists.
    if (protectExternalChanges) {
        QFile current(url.toLocalFile());
        if (QFileInfo(url.toLocalFile()).isSymLink() || !current.open(QIODevice::ReadOnly)
            || current.readAll() != m_lastKnownFileContents || current.error() != QFile::NoError) {
            file.cancelWriting();
            setStatus("Autosave paused: file changed outside Omawrite."); return;
        }
    }

    // QSaveFile commits by replacing the target. Stop watching the old inode
    // before that replacement so our own write is not classified as external.
    const QStringList watched = m_fileWatcher.files();
    if (!watched.isEmpty())
        m_fileWatcher.removePaths(watched);

    // commit() flushes, fsyncs, and atomically renames the temp file into place,
    // returning false (and leaving the original untouched) on any write error.
    if (!file.commit()) {
        watchCurrentFile();
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Could not write %1.").arg(targetName));
        emit saveFailed();
        emit quitCanceled();
        return;
    }

    const bool shouldClose = m_closeAfterSave;
    m_closeAfterSave = false;
    m_lastKnownFileContents = contents;
    m_hasKnownFileContents = true;
    setFileUrl(url);
    m_library.recordRecentFile(url);
    watchCurrentFile();
    QSettings().setValue(lastSaveDirectorySetting,
                         QFileInfo(url.toLocalFile()).absolutePath());
    if (!saveAuthorship(url)) {
        setModified(true); scheduleRecovery();
        setStatus("Markdown saved; authorship sidecar could not be saved. Keep this window open and retry Save.");
        emit saveFailed(); emit quitCanceled(); return;
    }
    setModified(false);
    m_requiresExplicitSave = false;
    setStatus(QStringLiteral("Saved %1").arg(fileName()));
    clearRecovery();
    emit saveSucceeded();

    if (shouldClose)
        emit closeAfterSave();
}

void Backend::scheduleRecovery() {
    m_recoveryTimer.start();
}

QString Backend::recoveryPath() const {
    return m_recoveryPath;
}

void Backend::writeRecovery() {
    if (!m_modified)
        return;
    const QString path = recoveryPath();
    if (path.isEmpty())
        return;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    const QJsonObject recovery{{QStringLiteral("fileUrl"), m_fileUrl.toString()},
                               {QStringLiteral("text"), currentDocumentText()}, {QStringLiteral("authorship"), authorshipData()},
                               {"requiresExplicitSave", m_requiresExplicitSave},
                               {"knownDiskContents", m_hasKnownFileContents},
                               {"diskContents", QString::fromLatin1(m_lastKnownFileContents.toBase64())}};
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    file.write(QJsonDocument(recovery).toJson(QJsonDocument::Compact));
    file.commit();
}

void Backend::restoreRecovery() {
    QFile file(recoveryPath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    if (!json.isObject() || !json.object().contains(QStringLiteral("text")))
        return;
    const QJsonObject recovery = json.object();
    loadDocumentText(recovery.value(QStringLiteral("text")).toString());
    applyAuthorshipData(recovery.value("authorship").toObject());
    const QUrl recoveredUrl(recovery.value(QStringLiteral("fileUrl")).toString());
    // Never bless edits made by another writer while this app was closed.
    // Older snapshots have no baseline: recover the text but require manual Save.
    m_hasKnownFileContents = recovery.value("knownDiskContents").toBool(false)
        && recovery.value("diskContents").isString();
    m_lastKnownFileContents = m_hasKnownFileContents
        ? QByteArray::fromBase64(recovery.value("diskContents").toString().toLatin1()) : QByteArray();
    m_requiresExplicitSave = recovery.value("requiresExplicitSave").toBool(false);
    setFileUrl(recoveredUrl);
    setModified(true);
    setStatus(QStringLiteral("Recovered unsaved changes"));
}

void Backend::clearRecovery() {
    m_recoveryTimer.stop();
    QFile::remove(recoveryPath());
}

void Backend::watchCurrentFile() {
    const QStringList watched = m_fileWatcher.files();
    if (!watched.isEmpty())
        m_fileWatcher.removePaths(watched);
    if (m_fileUrl.isLocalFile() && QFileInfo::exists(m_fileUrl.toLocalFile()))
        m_fileWatcher.addPath(m_fileUrl.toLocalFile());
}

void Backend::loadOmarchyTheme() {
    const bool oldDark = m_darkMode;
    m_darkMode = m_themePreset == "dark" || (m_themePreset == "system" && m_systemDarkMode);
    m_themeBackground = m_darkMode ? QStringLiteral("#101010") : QStringLiteral("#ffffff");
    m_themeForeground = m_darkMode ? QStringLiteral("#eeeeee") : QStringLiteral("#222324");
#ifdef Q_OS_MACOS
    m_themeAccent = m_darkMode ? QStringLiteral("#8eb5f0") : QStringLiteral("#244f88");
    m_themeSelection = m_darkMode ? QStringLiteral("#345783") : QStringLiteral("#345f98");
#else
    m_themeAccent = m_darkMode ? QStringLiteral("#5584aa") : QStringLiteral("#2077b2");
    m_themeSelection = m_darkMode ? QStringLiteral("#186a9a") : QStringLiteral("#2077b2");

#endif

    const QString colorsPath = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml");
    QString themeMode;
    QFile file(colorsPath);
    if (m_themePreset == "system" && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;

            const int equals = line.indexOf(QLatin1Char('='));
            if (equals < 0)
                continue;

            const QString key = line.left(equals).trimmed();
            QString value = line.mid(equals + 1).trimmed();
            if (value.size() >= 2
                    && ((value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
                        || (value.front() == QLatin1Char('\'') && value.back() == QLatin1Char('\''))))
                value = value.mid(1, value.size() - 2);

            if (key == QStringLiteral("mode"))
                themeMode = value;
            else if (key == QStringLiteral("background"))
                m_themeBackground = value;
            else if (key == QStringLiteral("foreground"))
                m_themeForeground = value;
            else if (key == QStringLiteral("accent"))
                m_themeAccent = value;
            else if (key == QStringLiteral("selection"))
                m_themeSelection = value;
        }
    }

    bool themeModeKnown = false;
    bool themeIsDark = m_darkMode;
    if (themeMode == QStringLiteral("dark")) {
        themeIsDark = true;
        themeModeKnown = true;
    } else if (themeMode == QStringLiteral("light")) {
        themeIsDark = false;
        themeModeKnown = true;
    } else {
        const QColor background(m_themeBackground);
        if (background.isValid()) {
            const double luminance = 0.299 * background.redF()
                + 0.587 * background.greenF() + 0.114 * background.blueF();
            themeIsDark = luminance < 0.5;
            themeModeKnown = true;
        }
    }
    if (themeModeKnown && themeIsDark != m_darkMode) {
        m_darkMode = themeIsDark;
        emit darkModeChanged();
    }

    if (m_themePreset == "paper") {
        m_themeBackground = "#faf4e6";
        m_themeForeground = "#342f27";
        m_themeAccent = "#285e9e";
        m_themeSelection = "#345f98";
    }
    if (oldDark != m_darkMode) emit darkModeChanged();
    if (m_highlighter) {
        m_highlighter->setDarkMode(m_darkMode);
        m_highlighter->setColors(m_themeBackground, m_themeForeground, m_themeAccent);
    }

    emit themeColorsChanged();
}

void Backend::watchOmarchyTheme() {
    const QStringList watched = m_themeWatcher.files() + m_themeWatcher.directories();
    if (!watched.isEmpty())
        m_themeWatcher.removePaths(watched);

    const QString currentDir = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current");
    const QString themeDir = currentDir + QStringLiteral("/theme");
    const QString colorsPath = themeDir + QStringLiteral("/colors.toml");

    if (QDir(currentDir).exists())
        m_themeWatcher.addPath(currentDir);
    if (QDir(themeDir).exists())
        m_themeWatcher.addPath(themeDir);
    if (QFile::exists(colorsPath))
        m_themeWatcher.addPath(colorsPath);
}

QUrl Backend::suggestedSaveUrl() const {
    if (m_fileUrl.isLocalFile())
        return m_fileUrl;

    const QString savedDirectory = QSettings().value(lastSaveDirectorySetting).toString();
    const QDir directory = savedDirectory.isEmpty() || !QDir(savedDirectory).exists()
        ? QDir::home()
        : QDir(savedDirectory);
    return QUrl::fromLocalFile(
        directory.filePath(suggestedFileName(currentDocumentText())));
}

QString Backend::currentDocumentText() const {
    return m_document ? m_document->toPlainText() : QString();
}

int Backend::countWords(const QString &text) {
    static const QRegularExpression wordRe(
        QStringLiteral("[\\p{L}\\p{N}]+(?:['-][\\p{L}\\p{N}]+)*"));
    int count = 0;
    QRegularExpressionMatchIterator it = wordRe.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

QString Backend::suggestedFileName(const QString &text) {
    QString name = text.section(QLatin1Char('\n'), 0, 0).trimmed();
    name.replace(QRegularExpression(QStringLiteral("[/\\x00-\\x1f\\x7f]")),
                 QStringLiteral("-"));
    name = name.left(120).trimmed();
    if (name.isEmpty() || name == QStringLiteral(".") || name == QStringLiteral(".."))
        name = QStringLiteral("Untitled");
    if (!name.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive))
        name += QStringLiteral(".md");
    return name;
}

void Backend::setWordCount(int words) {
    if (m_wordCount == words)
        return;

    m_wordCount = words;
    emit wordCountChanged();
}

void Backend::refreshWordCount() {
    setWordCount(countWords(currentDocumentText()));
}

void Backend::scheduleWordCount() {
    m_wordCountTimer.start();
}

void Backend::applyDocumentTypography() {
    if (!m_document)
        return;

    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(typoraLineHeightPercent, QTextBlockFormat::ProportionalHeight);

    // A full pass is only used for freshly loaded/attached documents, so it is
    // safe to drop undo history here (re-enabling clears the stack anyway).
    const bool undoEnabled = m_document->isUndoRedoEnabled();
    m_document->setUndoRedoEnabled(false);

    m_formattingTypography = true;
    QTextCursor cursor(m_document);
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(blockFormat);
    m_formattingTypography = false;

    m_document->setUndoRedoEnabled(undoEnabled);

    m_formattedBlockCount = m_document->blockCount();
}

void Backend::reapplyTypographyToChange() {
    if (!m_document)
        return;

    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(typoraLineHeightPercent, QTextBlockFormat::ProportionalHeight);

    // Format only the block(s) touched by the last edit instead of the whole
    // document, and fold the change into the preceding edit command so a single
    // undo reverts both the text and its formatting.
    const int maxPos = m_document->characterCount() - 1;
    const int start = qBound(0, m_lastChangePos, maxPos);
    const int end = qBound(start, m_lastChangePos + m_lastChangeAdded, maxPos);

    m_formattingTypography = true;
    QTextCursor cursor(m_document);
    cursor.joinPreviousEditBlock();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.mergeBlockFormat(blockFormat);
    cursor.endEditBlock();
    m_formattingTypography = false;
}

QString Backend::previewMarkdown(const QString &source) const { return expandedMarkdown(source, documentBaseUrl()); }

void Backend::setOutputStyle(int style) { m_outputStyle = qBound(0, style, 3); }

bool Backend::loadOutputStyle(const QUrl &url) {
    QFile file(url.toLocalFile());
    if (!url.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size() > 16384) { setStatus("Cannot read output style."); return false; }
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    const QString family = object.value("fontFamily").toString();
    const int size = object.value("pointSize").toInt();
    if (family.isEmpty() || size < 8 || size > 32) { setStatus("Style needs fontFamily and pointSize between 8 and 32."); return false; }
    m_customOutputFont = family; m_customOutputSize = size; m_outputStyle = 3;
    setStatus("Custom output style loaded."); return true;
}

void Backend::prepareOutput(QTextDocument &document, bool plain) const {
    const QStringList families{"Helvetica Neue", "Georgia", "iA Writer Mono S", m_customOutputFont};
    document.setDefaultFont(QFont(families.value(m_outputStyle), m_outputStyle == 3 ? m_customOutputSize : 12));
    document.setBaseUrl(documentBaseUrl());
    if (plain) document.setPlainText(currentDocumentText());
    else document.setMarkdown(previewMarkdown(currentDocumentText()));
}

void Backend::pageSetup() {
    QPrinter printer;
    if (m_pageLayout.isValid()) printer.setPageLayout(m_pageLayout);
    QPageSetupDialog dialog(&printer);
    if (dialog.exec() == QDialog::Accepted) m_pageLayout = printer.pageLayout();
}

bool Backend::exportDocument(const QUrl &destination, const QString &format) {
    if (!m_document || !destination.isLocalFile() || (format != "html" && format != "pdf")) return false;
    const QFileInfo info(destination.toLocalFile());
    if (info.isSymLink() || (!m_fileUrl.isEmpty() && (info.absoluteFilePath() == QFileInfo(m_fileUrl.toLocalFile()).absoluteFilePath() ||
        (!info.canonicalFilePath().isEmpty() && info.canonicalFilePath() == QFileInfo(m_fileUrl.toLocalFile()).canonicalFilePath())))) {
        setStatus("Choose an export destination other than the source document."); return false;
    }
    QTextDocument rendered;
    prepareOutput(rendered);
    QSaveFile output(destination.toLocalFile());
    if (!output.open(QIODevice::WriteOnly)) { setStatus(output.errorString()); return false; }
    if (format == "html") {
        QString html = rendered.toHtml();
        html.replace("<head>", "<head><base href=\"" + documentBaseUrl().toString(QUrl::FullyEncoded).toHtmlEscaped() + "\" />");
        const QByteArray bytes = html.toUtf8();
        if (output.write(bytes) != bytes.size()) { setStatus(output.errorString()); return false; }
    } else {
        QPdfWriter writer(&output);
        if (m_pageLayout.isValid()) writer.setPageLayout(m_pageLayout);
        else writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setTitle(fileName());
        rendered.print(&writer);
    }
    if (!output.commit()) { setStatus(output.errorString()); return false; }
    setStatus("Exported " + info.fileName()); return true;
}

void Backend::nativeWindowAction(const QString &action) {
#ifdef Q_OS_MACOS
    extern void performMacWindowAction(QWindow *, const QString &, const QString &);
    performMacWindowAction(m_parentWindow, action, currentDocumentText());
#else
    Q_UNUSED(action);
#endif
}
QStringList Backend::spellingIssues(const QString &text) {
#ifdef Q_OS_MACOS
    extern QStringList macSpellingIssues(const QString &);
    return macSpellingIssues(text);
#else
    Q_UNUSED(text);
    return {};
#endif
}

void Backend::setAutomaticVersions(bool enabled) { QSettings().setValue("safety/automaticVersions", enabled); }

bool Backend::createVersion() {
#ifdef Q_OS_MACOS
    if (!m_fileUrl.isLocalFile() || !QFileInfo::exists(m_fileUrl.toLocalFile())) return false;
    extern QString createMacVersion(const QString &);
    const QString error = createMacVersion(m_fileUrl.toLocalFile());
    setStatus(error.isEmpty() ? "Version of saved file created." : error);
    return error.isEmpty();
#else
    return false;
#endif
}
QVariantList Backend::versions() const {
#ifdef Q_OS_MACOS
    extern QVariantList macVersions(const QString &);
    return m_fileUrl.isLocalFile() ? macVersions(m_fileUrl.toLocalFile()) : QVariantList();
#else
    return {};
#endif
}
bool Backend::restoreVersion(const QUrl &url) {
    bool found = false;
    for (const auto &version : versions()) if (version.toMap()["url"].toUrl() == url) found = true;
    if (!found || !m_document) return false;
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) { setStatus("Version no longer available."); return false; }
    const auto bytes = file.readAll();
    if (file.error() != QFile::NoError) return false;
    // Native versions contain Markdown only. Never inherit current authorship
    // labels into historical text that has no corresponding metadata snapshot.
    QTextCursor cursor(m_document);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    QTextCharFormat unlabelled;
    cursor.insertText(QString::fromUtf8(bytes), unlabelled);
    cursor.endEditBlock();
    m_requiresExplicitSave = true;
    setModified(true);
    writeRecovery();
    setStatus("Version restored without authorship labels. Autosave paused; Undo or Save to keep it.");
    return true;
}
void Backend::autosave() {
    if (m_requiresExplicitSave) { setStatus("Autosave paused: review the restored version and Save explicitly."); return; }
    if (!m_modified || !m_fileUrl.isLocalFile() || !m_hasKnownFileContents) return;
    QFile current(m_fileUrl.toLocalFile());
    if (!current.open(QIODevice::ReadOnly) || current.readAll() != m_lastKnownFileContents || current.error() != QFile::NoError) {
        setStatus("Autosave paused: file changed outside Omawrite."); return;
    }
    saveTo(m_fileUrl, true);
}

QVariantList Backend::writingAnalysis(const QString &text, const QString &customWords) {
    QVariantList result;
#ifdef Q_OS_MACOS
    extern QVariantList macWordClasses(const QString &);
    result = macWordClasses(text.left(50000));
#endif
    QStringList watchWords = customWords.split(',', Qt::SkipEmptyParts);
    watchWords << "very" << "really" << "quite" << "just";
    for (QString word : watchWords) {
        word = word.trimmed();
        if (word.isEmpty()) continue;
        const QRegularExpression pattern("\\b" + QRegularExpression::escape(word) + "\\b", QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
        auto matches = pattern.globalMatch(text.left(50000));
        while (matches.hasNext() && result.size() < 1000) {
            const auto match = matches.next();
            result.append(QVariantMap{{"start", match.capturedStart()}, {"end", match.capturedEnd()}, {"label", "Review word"}, {"word", match.captured()}});
        }
    }
    return result;
}

static constexpr int authorProperty = QTextFormat::UserProperty + 701;

void Backend::markAuthorship(int start, int end, const QString &category, const QString &author) {
    if (!m_document || (category != "Human" && category != "AI" && category != "Reference" && category != "Unknown")) return;
    const int length = currentDocumentText().size();
    int first = qBound(0, qMin(start, end), length), last = qBound(0, qMax(start, end), length);
    if (first == last) return;
    QTextCursor cursor(m_document);
    cursor.setPosition(first); cursor.setPosition(last, QTextCursor::KeepAnchor);
    QTextCharFormat format;
    format.setProperty(authorProperty, category == "Unknown" ? QString() : category + "\n" + author.left(200));
    cursor.beginEditBlock(); cursor.mergeCharFormat(format); cursor.endEditBlock();
    setModified(true); scheduleRecovery();
    setStatus("Authorship annotation applied; Save writes a separate metadata file.");
}
QVariantList Backend::authorshipRanges() const {
    QVariantList ranges;
    if (!m_document) return ranges;
    for (auto block = m_document->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const auto fragment = it.fragment();
            const QString value = fragment.charFormat().stringProperty(authorProperty);
            if (!value.isEmpty()) ranges.append(QVariantMap{{"start", fragment.position()}, {"end", fragment.position() + fragment.length()},
                {"category", value.section('\n', 0, 0)}, {"author", value.section('\n', 1)}});
        }
    }
    return ranges;
}
QJsonObject Backend::authorshipData() const {
    return QJsonObject{{"version", 1}, {"sha256", QString::fromLatin1(QCryptographicHash::hash(currentDocumentText().toUtf8(), QCryptographicHash::Sha256).toHex())},
        {"ranges", QJsonArray::fromVariantList(authorshipRanges())}};
}
void Backend::applyAuthorshipData(const QJsonObject &data) {
    if (!m_document) return;
    m_document->setUndoRedoEnabled(false);
    const auto undoGuard = qScopeGuard([this] { m_document->setUndoRedoEnabled(true); });
    QTextCursor clear(m_document); clear.select(QTextCursor::Document);
    QTextCharFormat blank; blank.setProperty(authorProperty, QString()); clear.mergeCharFormat(blank);
    if (data["version"].toInt() != 1 || data["sha256"].toString() != authorshipData()["sha256"].toString()) return;
    const auto ranges = data["ranges"].toArray();
    if (ranges.size() > 100000) return;
    for (const auto &item : ranges) {
        const auto range = item.toObject();
        const int start = range["start"].toInt(-1), end = range["end"].toInt(-1);
        const QString category = range["category"].toString();
        if (start < 0 || end <= start || end > currentDocumentText().size() ||
            (category != "Human" && category != "AI" && category != "Reference")) continue;
        QTextCursor cursor(m_document); cursor.setPosition(start); cursor.setPosition(end, QTextCursor::KeepAnchor);
        QTextCharFormat format; format.setProperty(authorProperty, category + "\n" + range["author"].toString().left(200));
        cursor.mergeCharFormat(format);
    }
    m_document->clearUndoRedoStacks();
}
bool Backend::saveAuthorship(const QUrl &url) {
    const QString path = authorshipPath(url);
    if (authorshipRanges().isEmpty() && !QFileInfo::exists(path)) return true;
    if (QFileInfo(path).isSymLink()) return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const auto bytes = QJsonDocument(authorshipData()).toJson(QJsonDocument::Compact);
    return file.write(bytes) == bytes.size() && file.commit();
}
void Backend::loadAuthorship(const QUrl &url) {
    QFile file(authorshipPath(url));
    if (!file.open(QIODevice::ReadOnly) || file.size() > 10 * 1024 * 1024) { applyAuthorshipData({}); return; }
    applyAuthorshipData(QJsonDocument::fromJson(file.readAll()).object());
}

int Backend::nativeTabInset() const {
#ifdef Q_OS_MACOS
    extern int macTabInset(QWindow *);
    return macTabInset(m_parentWindow);
#else
    return 0;
#endif
}
