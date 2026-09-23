#include <QTemporaryDir>
#include <QDirIterator>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QPagedPaintDevice>
#include <QImageReader>
#include <QMimeDatabase>
#include <QScopeGuard>
#include <QCryptographicHash>
#include <QTextFragment>
#include <QTextTable>
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
#include <QPrintPreviewDialog>
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

// Context operations always resolve the clicked path, never the active editor by accident.
static QSet<Backend *> liveBackends;
static bool pathWithin(const QString &path, const QString &folder) {
    return path == folder || path.startsWith(folder + QDir::separator());
}

QVariantMap Backend::libraryItemInfo(const QUrl &url) const {
    QFileInfo info(url.toLocalFile());
    if (!url.isLocalFile() || !info.exists()) return {{"error", "This item is no longer available."}};
    bool favorite = false;
    for (const auto &entry : m_library.favorites())
        if (entry.toMap().value("url").toUrl() == url) favorite = true;
    return {{"name", info.fileName()}, {"url", url}, {"path", info.absoluteFilePath()},
        {"directory", info.isDir()}, {"available", true}, {"favorite", favorite},
        {"bytes", info.size()}, {"modified", info.lastModified().toString(Qt::ISODate)},
        {"created", info.birthTime().toString(Qt::ISODate)}, {"writable", info.isWritable()}};
}

bool Backend::libraryItemAction(const QUrl &url, const QString &action, const QString &argument) {
    const QFileInfo info(url.toLocalFile());
    auto fail = [&](const QString &message) { setStatus(message); return false; };
    if (!url.isLocalFile() || !info.exists() || info.isSymLink())
        return fail("The item is unavailable or is a symbolic link.");
    const QString path = info.canonicalFilePath();
    if (info.isDir() && QDir(path).isRoot() && (action == "rename" || action == "trash" || action == "duplicate"))
        return fail("A filesystem root cannot be renamed, duplicated or moved to Trash.");
    Backend *owner = nullptr;
    for (auto *candidate : liveBackends) {
        const QString openPath = QFileInfo(candidate->fileUrl().toLocalFile()).canonicalFilePath();
        if (openPath == path) owner = candidate;
        if (info.isDir() && (action == "rename" || action == "trash" || action == "duplicate")
            && !openPath.isEmpty() && pathWithin(openPath, path))
            return fail("Close documents inside this folder before renaming, duplicating or moving it to Trash.");
    }
    if (action == "favorite") { m_library.toggleFavorite(url); return true; }
    if (action == "share") {
#ifdef Q_OS_MACOS
        extern void shareMacFile(QWindow *, const QString &);
        shareMacFile(m_parentWindow, path); return true;
#else
        return fail("System sharing is available on macOS.");
#endif
    }
    if (action == "trash") {
        if (owner && owner->modified()) return fail("Save or close this document before moving it to Trash. Unsaved edits are unchanged.");
        QString trashedPath;
        if (!QFile::moveToTrash(path, &trashedPath)) return fail("Could not move this item to Trash. Nothing was permanently deleted.");
        // Keep a clean open document as an untitled draft; Save must choose a new path.
        if (owner) { owner->setFileUrl(QUrl()); owner->setModified(true); owner->writeRecovery(); }
        const QString metadata = authorshipPath(url);
        const bool metadataMoved = info.isDir() || !QFileInfo::exists(metadata) || QFile::moveToTrash(metadata);
        for (auto *candidate : liveBackends) candidate->m_library.refresh();
        setStatus(metadataMoved ? "Moved to Trash. Restore it with Finder if needed." : "Moved to Trash; the authorship sidecar could not be moved and remains in the original folder."); return true;
    }
    if (action == "newFile" || action == "newFolder" || action == "rename" || action == "duplicate") {
        if (argument.isEmpty() || argument.trimmed() != argument || argument == "." || argument == ".."
            || argument.contains('/') || argument.contains('\\') || argument.contains(QChar::Null))
            return fail("Enter a name without slashes or surrounding spaces.");
        const QString parent = (action == "newFile" || action == "newFolder") && info.isDir() ? path : QFileInfo(path).absolutePath();
        const QString destination = QDir(parent).filePath(argument);
        if (QFileInfo::exists(destination) || QFileInfo(destination).isSymLink()) return fail("That name is already in use.");
        if (action == "newFile") {
            if (!FileLibrary::isTextFile(argument)) return fail("Use a Markdown or .txt filename.");
            QFile file(destination);
            if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return fail("Could not create the file.");
        } else if (action == "newFolder") {
            if (!QDir(parent).mkdir(argument)) return fail("Could not create the folder.");
        } else if (!info.isDir()) {
            if (!FileLibrary::isTextFile(argument)) return fail("Use a Markdown or .txt filename.");
            if (owner) {
                const bool ok = action == "rename" ? owner->renameDocument(argument) : owner->duplicateDocument(argument);
                setStatus(owner->status());
                if (!ok) return false;
            } else {
                StagedAuthorship metadata;
                if (!metadata.prepare(url, QUrl::fromLocalFile(destination))) return fail("Could not preserve the file's authorship metadata.");
                bool ok = action == "rename" ? QFile::rename(path, destination) : QFile::copy(path, destination);
                if (!ok) return fail("Could not change the file. Check permissions and available space.");
                metadata.finish(action == "rename");
            }
        } else if (action == "rename") {
            if (!QDir().rename(path, destination)) return fail("Could not rename the folder.");
        } else {
            // Stage a bounded recursive copy beside the destination; publish only on success.
            QTemporaryDir staging(QDir(parent).filePath(".omawrite-copy-XXXXXX"));
            if (!staging.isValid()) return fail("Could not create the temporary copy.");
            QDirIterator scan(path, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            qint64 bytes = 0; int count = 0;
            while (scan.hasNext()) {
                scan.next(); const QFileInfo child = scan.fileInfo();
                bytes += child.isFile() ? child.size() : 0;
                if (++count > 20000 || bytes > 1024LL * 1024 * 1024 || child.isSymLink() || (!child.isFile() && !child.isDir()))
                    return fail("Use Finder to duplicate folders with links, special files, more than 20,000 items or over 1 GB.");
                const QString copy = QDir(staging.path()).filePath(QDir(path).relativeFilePath(child.filePath()));
                if (child.isDir() ? !QDir().mkpath(copy) : !QFile::copy(child.filePath(), copy))
                    return fail("Could not finish the folder copy; the original is unchanged.");
            }
            if (!QDir().rename(staging.path(), destination)) return fail("Could not publish the copied folder.");
            staging.setAutoRemove(false);
        }
        for (auto *candidate : liveBackends) {
            if (action == "rename") candidate->m_library.relocatedPath(QUrl::fromLocalFile(path), QUrl::fromLocalFile(destination));
            candidate->m_library.refresh();
        }
        setStatus((action == "rename" ? "Renamed to " : action == "duplicate" ? "Duplicated as " : "Created ") + argument); return true;
    }
    if (info.isDir()) return fail("This action requires a document.");
    // Output and clipboard operations use live unsaved text if the file is already open.
    // Otherwise create an isolated, recovery-free output backend without changing any window.
    QTextDocument document;
    std::unique_ptr<Backend> snapshot;
    if (!owner) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 32 * 1024 * 1024) return fail("Cannot read this document (32 MB limit).");
        const QByteArray bytes = file.readAll();
        if (file.error() != QFileDevice::NoError) return fail("Could not read the complete document.");
        snapshot = std::make_unique<Backend>(nullptr, true);
        document.setPlainText(QString::fromUtf8(bytes));
        snapshot->m_document = &document; snapshot->m_fileUrl = url; snapshot->m_parentWindow = m_parentWindow;
        owner = snapshot.get();
    }
    if (action == "exportHtml" || action == "exportPdf") {
        bool ok = owner->exportDocument(QUrl(argument), action == "exportHtml" ? "html" : "pdf");
        setStatus(owner->status()); return ok;
    }
    if (action == "print") { owner->printDocument(false); return true; }
    if (action == "printSource") { owner->printDocument(true); return true; }
    if (action == "printPreview") { owner->printPreview(); return true; }
    if (action == "copyMarkdown") { QGuiApplication::clipboard()->setText(owner->currentDocumentText()); return true; }
    if (action == "copyText" || action == "copyHtml") {
        QTextDocument rendered; rendered.setMarkdown(owner->previewMarkdown(owner->currentDocumentText()));
        auto *mime = new QMimeData; mime->setText(rendered.toPlainText());
        if (action == "copyHtml") mime->setHtml(rendered.toHtml());
        QGuiApplication::clipboard()->setMimeData(mime); return true;
    }
    return fail("Unknown library action.");
}

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

Backend::Backend(QObject *parent, bool outputOnly) : QObject(parent), m_library(this) {
    if (!outputOnly) liveBackends.insert(this);
    connect(&m_library, &FileLibrary::rootFolderChanged, this, &Backend::fileUrlChanged);
    const QString stateDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(stateDirectory);
    // Claim an orphaned snapshot before taking an empty slot. This ensures a
    // crash in window 2 is still recovered even if window 1 exited normally.
    for (int pass = 0; !outputOnly && pass < 2 && !m_recoveryLock; ++pass) {
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

    m_outputStyle=qBound(0,QSettings().value("output/style",0).toInt(),7);
    m_customOutputFont=QSettings().value("output/font","Georgia").toString();
    m_customOutputSize=qBound(8,QSettings().value("output/size",12).toInt(),32);
    m_outputHeader=QSettings().value("output/header").toString().left(200);
    m_outputFooter=QSettings().value("output/footer","{page} / {pages}").toString().left(200);
    m_outputTitlePage=QSettings().value("output/titlePage",false).toBool();
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

Backend::~Backend() { liveBackends.remove(this); }

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

static const QRegularExpression &statisticsWordPattern() {
    static const QRegularExpression pattern(
        QStringLiteral("[\\p{L}\\p{N}]+(?:['-][\\p{L}\\p{N}]+)*"));
    return pattern;
}

static const QRegularExpression &completionWordPattern() {
    static const QRegularExpression pattern(
        QStringLiteral("[\\p{L}\\p{M}\\p{N}]+(?:['_-][\\p{L}\\p{M}\\p{N}]+)*"),
        QRegularExpression::UseUnicodePropertiesOption);
    return pattern;
}

static bool completionContextAllowed(const QString &markdown, int position) {
    if (position < 0 || position > markdown.size()) return false;
    const int lineStart = position == 0 ? 0 : markdown.lastIndexOf('\n', position - 1) + 1;
    int lineEnd = markdown.indexOf('\n', position);
    if (lineEnd < 0) lineEnd = markdown.size();

    QChar fence;
    int fenceLength = 0;
    const QRegularExpression fencePattern(QStringLiteral("^ {0,3}(?:> ?)*(`{3,}|~{3,})(.*)$"));
    int offset = 0;
    while (offset <= lineStart) {
        int end = markdown.indexOf('\n', offset);
        if (end < 0) end = markdown.size();
        const QString line = markdown.mid(offset, end - offset);
        const auto marker = fencePattern.match(line);
        if (marker.hasMatch()) {
            const QString sequence = marker.captured(1);
            if (fence.isNull()) {
                fence = sequence.front();
                fenceLength = sequence.size();
            } else if (sequence.front() == fence && sequence.size() >= fenceLength
                       && marker.captured(2).trimmed().isEmpty()) {
                fence = QChar();
            }
        }
        if (offset == lineStart || end == markdown.size()) break;
        offset = end + 1;
    }
    if (!fence.isNull()) return false;

    const QString line = markdown.mid(lineStart, lineEnd - lineStart);
    if (line.startsWith(QStringLiteral("    ")) || line.startsWith('\t')) return false;
    const QString before = markdown.mid(lineStart, position - lineStart);

    int openBackticks = 0;
    auto ticks = QRegularExpression(QStringLiteral("`+")).globalMatch(before);
    while (ticks.hasNext()) {
        const int length = ticks.next().capturedLength();
        if (openBackticks == 0) openBackticks = length;
        else if (length == openBackticks) openBackticks = 0;
    }
    if (openBackticks != 0) return false;

    int tokenStart = before.size();
    while (tokenStart > 0 && !before.at(tokenStart - 1).isSpace()
           && !QStringLiteral("<>\"'").contains(before.at(tokenStart - 1))) --tokenStart;
    const QString token = before.mid(tokenStart).toCaseFolded();
    if (token.contains(QStringLiteral("://")) || token.startsWith(QStringLiteral("www."))
        || token.startsWith(QStringLiteral("mailto:"))) return false;
    if (before.lastIndexOf(QStringLiteral("](")) > before.lastIndexOf(')')) return false;
    return true;
}

QVariantMap Backend::wordCompletions(int position) const {
    if (!m_document) return {};
    const QString text = currentDocumentText();
    position = qBound(0, position, text.size());
    if (position < text.size()) {
        const QChar next = text.at(position);
        if (next.isLetterOrNumber() || next.category() == QChar::Mark_NonSpacing
            || next == '_' || next == '-' || next == '\'') return {};
    }
    if (!completionContextAllowed(text, position)) return {};

    const int lineStart = position == 0 ? 0 : text.lastIndexOf('\n', position - 1) + 1;
    const QString before = text.mid(lineStart, position - lineStart);
    const QRegularExpression suffix(
        QStringLiteral("[\\p{L}\\p{M}\\p{N}]+(?:['_-][\\p{L}\\p{M}\\p{N}]+)*$"),
        QRegularExpression::UseUnicodePropertiesOption);
    const auto prefixMatch = suffix.match(before);
    if (!prefixMatch.hasMatch() || prefixMatch.capturedLength() < 2) return {};
    const QString prefix = prefixMatch.captured();
    const QString foldedPrefix = prefix.toCaseFolded();
    const int start = lineStart + prefixMatch.capturedStart();

    // Completion is explicit and local, so keep its synchronous work bounded.
    // proseForReview preserves UTF-16 offsets while blanking fenced/inline code
    // and URL destinations; inspect at most 256 matching occurrences from its
    // existing 50,000-code-unit review window.
    const QString searchable = proseForReview(text);
    QMap<QString, QString> unique;
    int matchingOccurrences = 0;
    auto words = completionWordPattern().globalMatch(searchable);
    while (words.hasNext()) {
        const auto match = words.next();
        const QString candidate = text.mid(match.capturedStart(), match.capturedLength());
        const QString folded = candidate.toCaseFolded();
        if (candidate.size() <= prefix.size() || !folded.startsWith(foldedPrefix)
            || folded == foldedPrefix) continue;
        if (++matchingOccurrences > 256) break;
        if (!completionContextAllowed(text, match.capturedEnd())) continue;
        if (!unique.contains(folded)) unique.insert(folded, candidate);
    }

    QStringList items = unique.values();
    std::sort(items.begin(), items.end(), [](const QString &left, const QString &right) {
        const int folded = QString::compare(left, right, Qt::CaseInsensitive);
        return folded == 0 ? left < right : folded < 0;
    });
    if (items.size() > 12) items = items.mid(0, 12);
    if (items.isEmpty()) return {};
    return {{QStringLiteral("start"), start}, {QStringLiteral("end"), position},
            {QStringLiteral("prefix"), prefix}, {QStringLiteral("items"), items}};
}

QVariantMap Backend::documentStatistics(const QString &markdown) const {
    QTextDocument rendered;
    rendered.setMarkdown(markdown);
    const QString plain = rendered.toPlainText();
    const int words = countWords(plain);
    QString compact = plain;
    compact.remove(QRegularExpression("\\s"));

    // This is deliberately a visible punctuation heuristic, not a language
    // grammar: a sentence ends at .?! or their CJK forms when followed by
    // whitespace/end (optionally after closing quotes/brackets). Any remaining
    // run containing a letter or number counts as one sentence.
    int sentences = 0;
    bool sentenceHasText = false;
    const QString terminators = QStringLiteral(".?!。！？");
    const QString closers = QStringLiteral("\"')]}»”’");
    for (int i = 0; i < plain.size(); ++i) {
        if (plain.at(i).isLetterOrNumber()) sentenceHasText = true;
        if (!sentenceHasText || !terminators.contains(plain.at(i))) continue;
        int next = i + 1;
        while (next < plain.size() && closers.contains(plain.at(next))) ++next;
        if (next == plain.size() || plain.at(next).isSpace()) {
            ++sentences;
            sentenceHasText = false;
            i = next - 1;
        }
    }
    if (sentenceHasText) ++sentences;

    // Tasks are source constructs. Count bullet and numbered task markers only
    // outside backtick/tilde fences; task completion does not change the count.
    int tasks = 0;
    QChar fence;
    int fenceLength = 0;
    const QRegularExpression fenceMarker(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    const QRegularExpression taskMarker(QStringLiteral("^ {0,3}(?:[-+*]|\\d+[.)])[ \\t]+\\[[ xX]\\](?:[ \\t]|$)"));
    for (const QString &line : markdown.split('\n')) {
        const auto marker = fenceMarker.match(line);
        if (marker.hasMatch()) {
            const QString sequence = marker.captured(1);
            if (fence.isNull()) { fence = sequence.at(0); fenceLength = sequence.size(); }
            else if (sequence.at(0) == fence && sequence.size() >= fenceLength
                     && marker.captured(2).trimmed().isEmpty()) fence = QChar();
        } else if (fence.isNull() && taskMarker.match(line).hasMatch()) {
            ++tasks;
        }
    }

    QVariantMap categoryWords{{QStringLiteral("Human"), 0}, {QStringLiteral("AI"), 0},
                              {QStringLiteral("Reference"), 0}};
    // Authorship metadata is a manual assertion over UTF-16 source offsets.
    // Count a source word only when every code unit is covered by one category;
    // partially labelled and unlabelled words are excluded. Never apply the
    // live document's offsets to a different Markdown string.
    if (m_document && markdown == currentDocumentText()) {
        const QVariantList ranges = authorshipRanges();
        auto matches = statisticsWordPattern().globalMatch(markdown);
        while (matches.hasNext()) {
            const auto match = matches.next();
            int covered = match.capturedStart();
            const int end = match.capturedEnd();
            QString category;
            for (const QVariant &value : ranges) {
                const QVariantMap range = value.toMap();
                const int rangeStart = range.value(QStringLiteral("start")).toInt();
                const int rangeEnd = range.value(QStringLiteral("end")).toInt();
                if (rangeEnd <= covered) continue;
                if (rangeStart > covered) break;
                const QString nextCategory = range.value(QStringLiteral("category")).toString();
                if (!categoryWords.contains(nextCategory)
                    || (!category.isEmpty() && category != nextCategory)) break;
                category = nextCategory;
                covered = qMin(end, rangeEnd);
                if (covered == end) break;
            }
            if (covered == end && !category.isEmpty())
                categoryWords[category] = categoryWords.value(category).toInt() + 1;
        }
    }
    // Words and Unicode-scalar character counts retain the existing rendered
    // plain-text basis. Reading stays at 200 WPM; speaking uses a fixed 130 WPM.
    // Empty documents report zero minutes and non-empty estimates round up.
    return {{"words", words}, {"characters", plain.toUcs4().size()},
            {"charactersWithoutSpaces", compact.toUcs4().size()},
            {"sentences", sentences},
            {"readingMinutes", words == 0 ? 0 : qMax(1, (words + 199) / 200)},
            {"speakingMinutes", words == 0 ? 0 : qMax(1, (words + 129) / 130)},
            {"tasks", tasks},
            {"humanWords", categoryWords.value(QStringLiteral("Human"))},
            {"aiWords", categoryWords.value(QStringLiteral("AI"))},
            {"referenceWords", categoryWords.value(QStringLiteral("Reference"))}};
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
    const int selectionFirst = first;
    const int selectionLast = last;
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
    if (action == "clearStyles") {
        if (selectionFirst == selectionLast) {
            setStatus(QStringLiteral("Select styled text or complete styled lines first."));
            return {};
        }
        const QString selected = text.mid(selectionFirst, selectionLast - selectionFirst);
        if (selected.contains('`') || selected.contains(QStringLiteral("]("))
                || selected.contains(QStringLiteral("][")) || selected.contains(QStringLiteral("[["))
                || selected.contains(QStringLiteral("]]")) || selected.contains(QStringLiteral("://"))) {
            setStatus(QStringLiteral("Clear Styles does not change code or links."));
            return {};
        }

        auto stripOuterInline = [](QString value, bool &changed, bool &ambiguous) {
            changed = false;
            ambiguous = false;
            bool stripped = true;
            while (stripped) {
                stripped = false;
                for (const QString &marker : {QStringLiteral("**"), QStringLiteral("__"),
                                              QStringLiteral("~~"), QStringLiteral("=="),
                                              QStringLiteral("*"), QStringLiteral("_")}) {
                    if (value.size() <= marker.size() * 2 || !value.startsWith(marker)
                            || !value.endsWith(marker)) continue;
                    value = value.mid(marker.size(), value.size() - marker.size() * 2);
                    changed = stripped = true;
                    break;
                }
            }
            ambiguous = value.contains('*') || value.contains('_') || value.contains(QStringLiteral("~~"))
                || value.contains(QStringLiteral("=="));
            return value;
        };

        // Fully enclosing supported wrappers are deterministic, including a
        // stack such as **==text==**. Partial or interleaved markers are not.
        if (!selected.contains('\n')) {
            bool changed = false, ambiguous = false;
            const QString inner = stripOuterInline(selected, changed, ambiguous);
            if (ambiguous && (changed || selectionFirst != lineStart || selectionLast != lineEnd)) {
                setStatus(QStringLiteral("Clear Styles does not change partial or mixed inline styles."));
                return {};
            }
            if (changed) return replaceText(selectionFirst, selectionLast, inner);
        }

        // Block clearing requires a selection aligned to complete lines. Each
        // line may use one supported, single-level prefix.
        if (selectionFirst != lineStart || selectionLast != lineEnd) {
            setStatus(QStringLiteral("Select complete styled lines to clear block styles."));
            return {};
        }
        const QRegularExpression heading(QStringLiteral("^(\\s{0,3})#{1,6} +(.*)$"));
        const QRegularExpression quote(QStringLiteral("^(\\s{0,3})> +(.*)$"));
        const QRegularExpression bulletTask(QStringLiteral("^(\\s{0,3})[-+*] +\\[[ xX]\\] +(.*)$"));
        const QRegularExpression orderedTask(QStringLiteral("^(\\s{0,3})[0-9]+[.)] +\\[[ xX]\\] +(.*)$"));
        const QRegularExpression bullet(QStringLiteral("^(\\s{0,3})[-+*] +(.*)$"));
        const QRegularExpression ordered(QStringLiteral("^(\\s{0,3})[0-9]+[.)] +(.*)$"));
        const QList<QRegularExpression> prefixes{heading, quote, bulletTask, orderedTask, bullet, ordered};
        QStringList cleared;
        for (const QString &line : selected.split('\n')) {
            if (line.isEmpty()) {
                setStatus(QStringLiteral("Clear Styles requires one simple style on every selected line."));
                return {};
            }
            QRegularExpressionMatch match;
            int matchedKind = -1;
            for (int index = 0; index < prefixes.size(); ++index) {
                match = prefixes.at(index).match(line);
                if (match.hasMatch()) { matchedKind = index; break; }
            }
            if (matchedKind < 0) {
                setStatus(QStringLiteral("Clear Styles requires a supported style on every selected line."));
                return {};
            }
            bool inlineChanged = false, ambiguous = false;
            const QString body = stripOuterInline(match.captured(2), inlineChanged, ambiguous);
            bool nested = body.isEmpty() || ambiguous || body.contains('`') || body.contains(QStringLiteral("]("))
                || body.contains(QStringLiteral("][")) || body.contains(QStringLiteral("[["));
            if (!nested) {
                for (const auto &candidate : prefixes) {
                    if (candidate.match(body).hasMatch()) { nested = true; break; }
                }
            }
            if (nested) {
                setStatus(QStringLiteral("Clear Styles does not change mixed or nested block styles."));
                return {};
            }
            cleared.append(match.captured(1) + body);
        }
        return replaceText(selectionFirst, selectionLast, cleared.join('\n'));
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
        const QRegularExpression prefix(QStringLiteral("^(?:#{1,6} +|> +|[-+*] +(?:\\[[ xX]\\] +)?|[0-9]+[.)] +(?:\\[[ xX]\\] +)?)"));
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
            const QString originalBody = body;
            body.remove(prefix);
            const auto sourceTask = QRegularExpression(
                QStringLiteral("^(?:[-+*]|[0-9]+[.)]) +\\[([ xX])\\] +")).match(originalBody);
            QString marker;
            if (action.startsWith("heading")) {
                const int level = action.mid(7).toInt();
                if (level < 1 || level > 6) return {};
                marker = QString(level, '#') + " ";
            } else if (action == "bullet") marker = "- ";
            else if (action == "ordered") marker = QString::number(number++) + ". ";
            else if (action == "task") marker = "- [ ] ";
            else if (action == "orderedTask") {
                const QChar state = sourceTask.hasMatch() && sourceTask.captured(1) != QStringLiteral(" ") ? 'x' : ' ';
                marker = QString::number(number++) + ". [" + state + "] ";
            }
            else if (action == "quote") marker = "> ";
            else if (action == "toggleTask") {
                const QRegularExpression task(QStringLiteral("^((?:[-+*]|[0-9]+[.)]) +)\\[([ xX])\\]"));
                const auto m = task.match(originalBody);
                if (m.hasMatch()) {
                    line[indentation + m.capturedStart(2)] = m.captured(2) == " " ? 'x' : ' ';
                    continue;
                }
                const QRegularExpression list(QStringLiteral("^((?:[-+*]|[0-9]+[.)]) +)"));
                const auto listMatch = list.match(originalBody);
                if (listMatch.hasMatch()) {
                    marker = listMatch.captured(1) + "[ ] ";
                    body = originalBody.mid(listMatch.capturedLength(1));
                } else marker = "- [ ] ";
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
    // Qt Markdown may discard empty HTML named anchors. The generated links
    // remain, so resolve a note through its backlink and a reference by occurrence.
    const auto reference=QRegularExpression("^(ow-note-[0-9]+-.+)-ref-([1-9][0-9]*)$").match(anchor);
    int occurrence=0;
    if(anchor.startsWith("ow-note-")) {
        for(auto block=quick->textDocument()->begin();block.isValid();block=block.next())
            for(auto it=block.begin();!it.atEnd();++it) {
                const auto fragment=it.fragment(); const auto href=fragment.charFormat().anchorHref();
                if(reference.hasMatch() && href=="#"+reference.captured(1) && ++occurrence==reference.captured(2).toInt()) return fragment.position();
                if(!reference.hasMatch() && href=="#"+anchor+"-ref-1") return block.position();
            }
    }
    QHash<QString, int> occurrences;
    for (auto block = quick->textDocument()->begin(); block.isValid(); block = block.next()) {
        for (auto fragment=block.begin(); !fragment.atEnd(); ++fragment)
            if (fragment.fragment().charFormat().anchorNames().contains(anchor)) return fragment.fragment().position();
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
    applyTemplate(*preview, true);
    // Match paginated output and discard stale Markdown table layout caches.
    preview->setHtml(preview->toHtml());

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
    // Includes format-only authorship edits and their Undo/Redo paths. The QML
    // consumer debounces this signal before recomputing the displayed metrics.
    connect(m_document, &QTextDocument::contentsChanged,
            this, &Backend::documentStatisticsChanged);

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
        paintOutput(printer, rendered);
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
        QVariantList clipped;
        for (const auto &item : authorshipRanges()) {
            auto range=item.toMap();
            const int left=qMax(first, range["start"].toInt()), right=qMin(last, range["end"].toInt());
            if (left >= right) continue;
            range["start"]=left-first; range["end"]=right-first; clipped.append(range);
        }
        const QJsonObject annotation{{"version",1}, {"sha256",QString::fromLatin1(QCryptographicHash::hash(selected.toUtf8(), QCryptographicHash::Sha256).toHex())}, {"ranges",QJsonArray::fromVariantList(clipped)}};
        mime->setData("application/x-omawrite-authorship+json", QJsonDocument(annotation).toJson(QJsonDocument::Compact));
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
    emit documentStatisticsChanged();
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
    int count = 0;
    QRegularExpressionMatchIterator it = statisticsWordPattern().globalMatch(text);
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

QString Backend::outputFont() const {
    return QStringList{"Helvetica Neue", "Georgia", "iA Writer Mono S", m_customOutputFont,
                       "Helvetica Neue", "Helvetica", "Palatino", "Times New Roman"}.value(m_outputStyle);
}
int Backend::outputPointSize() const { return m_outputStyle == 3 ? m_customOutputSize : 12; }
QString Backend::outputTemplateName() const {
    return QStringList{"Modern (Sans)", "Classic (Serif)", "Manuscript (Mono)", "Custom",
                       "GitHub", "Helvetica", "Palatino", "MLA Draft"}.value(m_outputStyle);
}
void Backend::setOutputStyle(int style) {
    if (style < 0 || style > 7) return;
    m_outputStyle = style;
    QSettings().setValue("output/style", style);
    emit outputStyleChanged();
}

void Backend::applyTemplate(QTextDocument &document, bool preview) const {
    const bool manuscript = m_outputStyle == 2 || m_outputStyle == 7;
    const bool github = m_outputStyle == 4;
    const QColor muted(preview ? palette().value("muted").toString() : "#555555");
    const QColor panel(preview ? palette().value("panel").toString() : "#f3f4f6");
    const QColor border(preview ? palette().value("border").toString() : "#c8ccd0");
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        auto format = block.blockFormat();
        const int heading = format.headingLevel();
        const bool code = format.hasProperty(QTextFormat::BlockCodeFence)
            || format.hasProperty(QTextFormat::BlockCodeLanguage) || format.nonBreakableLines();
        const bool quote = format.intProperty(QTextFormat::BlockQuoteLevel) > 0;
        format.setLineHeight(manuscript && !code ? 200 : github ? 150 : 135, QTextBlockFormat::ProportionalHeight);
        format.setTopMargin(heading > 0 && block.blockNumber() > 0 ? 18 : 0);
        format.setBottomMargin(manuscript || code ? 0 : block.textList() ? 4 : 12);
        // MLA Draft deliberately does not claim full citation or submission compliance.
        const bool tableCell = QTextCursor(block).currentTable() != nullptr;
        if (tableCell) format.setBottomMargin(0);
        format.setTextIndent(m_outputStyle == 7 && !heading && !code && !quote && !block.textList() && !tableCell ? 36 : 0);
        if (m_outputStyle == 7 && heading == 1) format.setAlignment(Qt::AlignHCenter);
        if (quote) { format.setLeftMargin(24); format.setRightMargin(12); }
        if (code) { format.setBackground(panel); format.setLeftMargin(12); format.setRightMargin(12); }
        QTextCursor cursor(block);
        cursor.setBlockFormat(format);
        if (heading || quote || code) {
            QTextCharFormat ink;
            if (heading) {
                ink.setProperty(QTextFormat::FontSizeAdjustment, manuscript ? 0 : heading == 1 ? 3 : heading == 2 ? 2 : 0);
                ink.setFontWeight(m_outputStyle == 7 ? QFont::Normal : QFont::Bold);
            }
            if (quote) ink.setForeground(muted);
            if (code) ink.setFontFamilies({"iA Writer Mono S"});
            cursor.setPosition(block.position());
            cursor.setPosition(block.position() + block.length() - 1, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(ink);
        }
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const auto fragment = it.fragment();
            if (!fragment.isValid() || !fragment.charFormat().isAnchor()) continue;
            QTextCursor link(&document); link.setPosition(fragment.position());
            link.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
            QTextCharFormat ink;
            ink.setForeground(QColor(preview ? palette().value("focus").toString() : "#245da8"));
            ink.setFontUnderline(true); link.mergeCharFormat(ink);
        }
    }
    QList<QTextFrame *> frames{document.rootFrame()};
    while (!frames.isEmpty()) {
        auto *frame = frames.takeLast();
        frames.append(frame->childFrames());
        if (auto *table = qobject_cast<QTextTable *>(frame)) {
            auto format = table->format();
            format.setBorder(0.5); format.setBorderBrush(border); format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            format.setCellPadding(github ? 8 : 5); format.setCellSpacing(0);
            format.setWidth(QTextLength(QTextLength::PercentageLength, 100));
            format.setColumnWidthConstraints(QList<QTextLength>(table->columns(), QTextLength(QTextLength::PercentageLength, 100.0 / table->columns())));
            table->setFormat(format);
            for (int row = 0; row < table->rows(); ++row) for (int column = 0; column < table->columns(); ++column) {
                auto cell = table->cellAt(row, column);
                auto cellFormat = cell.format().toTableCellFormat();
                cellFormat.setVerticalAlignment(QTextCharFormat::AlignTop);
                cellFormat.setBorder(0.5); cellFormat.setBorderBrush(border);
                cellFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
                cell.setFormat(cellFormat);
                if (row == 0) {
                    auto header = cell.firstCursorPosition();
                    header.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
                    QTextCharFormat bold; bold.setFontWeight(QFont::Bold); header.mergeCharFormat(bold);
                }
            }
        }
    }
}


bool Backend::loadOutputStyle(const QUrl &url) {
    QFile file(url.toLocalFile());
    if (!url.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size() > 16384) { setStatus("Cannot read output style."); return false; }
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    const QString family = object.value("fontFamily").toString();
    const int size = object.value("pointSize").toInt();
    if (family.isEmpty() || size < 8 || size > 32) { setStatus("Style needs fontFamily and pointSize between 8 and 32."); return false; }
    m_customOutputFont = family; m_customOutputSize = size;
    m_outputHeader=object.value("header").toString().left(200);
    m_outputFooter=object.value("footer").toString("{page} / {pages}").left(200);
    m_outputTitlePage=object.value("titlePage").toBool(false);
    QSettings settings; settings.setValue("output/font",family); settings.setValue("output/size",size);
    settings.setValue("output/header",m_outputHeader); settings.setValue("output/footer",m_outputFooter); settings.setValue("output/titlePage",m_outputTitlePage);
    setOutputStyle(3);
    setStatus("Custom output style loaded."); return true;
}

void Backend::prepareOutput(QTextDocument &document, bool plain) const {
    document.setDefaultFont(QFont(outputFont(), outputPointSize()));
    document.setBaseUrl(documentBaseUrl());
    if (plain) document.setPlainText(currentDocumentText());
    else {
        QString markdown=previewMarkdown(currentDocumentText());
        markdown.replace(QRegularExpression("(?m)^\\s*<!-- pagebreak -->\\s*$"), "\n\nOMAWRITE_PAGE_BREAK_SENTINEL\n\n");
        document.setMarkdown(markdown);
        for(auto block=document.begin();block.isValid();block=block.next()) if(block.text()=="OMAWRITE_PAGE_BREAK_SENTINEL") {
            QTextCursor cursor(&document); cursor.setPosition(block.position()); cursor.setPosition(block.position()+block.length()-1,QTextCursor::KeepAnchor); cursor.removeSelectedText();
            QTextBlockFormat format=cursor.blockFormat(); format.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore); cursor.setBlockFormat(format);
        }
        applyTemplate(document, false);
    }
}

void Backend::printPreview() {
    if (!m_document) return;
    QPrinter printer(QPrinter::HighResolution);
    if (m_pageLayout.isValid()) printer.setPageLayout(m_pageLayout);
    QPrintPreviewDialog dialog(&printer);
    dialog.setWindowTitle("Omawrite — Paginated Preview");
    connect(&dialog,&QPrintPreviewDialog::paintRequested,this,[this](QPrinter *output) {
        QTextDocument rendered; prepareOutput(rendered,false); paintOutput(*output,rendered);
    });
    dialog.resize(900,700); dialog.exec();
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
        const QRegularExpression images("<img\\b[^>]*\\bsrc=\"([^\"]+)\"",QRegularExpression::CaseInsensitiveOption);
        auto matches=images.globalMatch(html); QList<QPair<QPair<int,int>,QString>> substitutions; qint64 total=0;
        while(matches.hasNext()) {
            const auto match=matches.next(); const QUrl asset=documentBaseUrl().resolved(QUrl(match.captured(1).replace("&amp;","&")));
            if(asset.scheme()=="data") continue;
            QFile image(asset.toLocalFile()); const auto format=QImageReader::imageFormat(asset.toLocalFile());
            if(!asset.isLocalFile() || !QList<QByteArray>{"png","jpeg","gif","webp"}.contains(format) || !image.open(QIODevice::ReadOnly)
                || image.size()>5*1024*1024 || total+image.size()>20*1024*1024) { setStatus("Portable HTML needs readable local PNG/JPEG/GIF/WebP images (5 MiB each, 20 MiB total)."); return false; }
            const auto data=image.read(5*1024*1024+1); if(image.error()!=QFile::NoError || data.size()>5*1024*1024 || total+data.size()>20*1024*1024) { setStatus("Could not read export image."); return false; } total+=data.size();
            const QString uri="data:image/"+QString::fromLatin1(format)+";base64,"+QString::fromLatin1(data.toBase64());
            substitutions.append({{match.capturedStart(1),match.capturedLength(1)},uri});
        }
        for(auto it=substitutions.crbegin();it!=substitutions.crend();++it) html.replace(it->first.first,it->first.second,it->second);
        const QByteArray bytes = html.toUtf8();
        if (output.write(bytes) != bytes.size()) { setStatus(output.errorString()); return false; }
    } else {
        QPdfWriter writer(&output);
        if (m_pageLayout.isValid()) writer.setPageLayout(m_pageLayout);
        else { writer.setPageSize(QPageSize(QPageSize::A4)); writer.setPageMargins(QMarginsF(18,18,18,18)); }
        writer.setTitle(fileName());
        paintOutput(writer, rendered);
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

// Replace code/markup with spaces, retaining UTF-16 offsets for safe corrections.
QString Backend::proseForReview(const QString &markdown) {
    QString result = markdown.left(50000);
    int offset = 0; QChar fence; int fenceLength = 0;
    const QRegularExpression fencePattern("^ {0,3}(`{3,}|~{3,})(.*)$");
    auto blank = [&result](int start, int length) {
        for (int i=start; i<start+length && i<result.size(); ++i)
            if (result[i] != '\n') result[i] = ' ';
    };
    for (const QString &line : result.split('\n')) {
        QString candidate = line;
        candidate.remove(QRegularExpression("^ {0,3}(?:> ?)+"));
        candidate.remove(QRegularExpression("^ {0,3}(?:[-+*]|[0-9]+[.)]) +"));
        const auto match = fencePattern.match(candidate);
        if (match.hasMatch()) {
            const auto marker=match.captured(1);
            if (fence.isNull()) { fence=marker[0]; fenceLength=marker.size(); }
            else if (marker[0]==fence && marker.size()>=fenceLength && match.captured(2).trimmed().isEmpty()) fence=QChar();
            blank(offset,line.size());
        } else if (!fence.isNull() || line.startsWith("    ") || line.startsWith('\t') || line.trimmed().startsWith("/")) blank(offset,line.size());
        offset += line.size()+1;
    }
    const QRegularExpression inlineCode("(`+)([^`]|(?!\\1)`)*?\\1");
    auto matches=inlineCode.globalMatch(result);
    while(matches.hasNext()) { const auto match=matches.next(); blank(match.capturedStart(),match.capturedLength()); }
    // URL destinations and raw tags are syntax, not prose.
    const QRegularExpression syntax("\\]\\([^\\n]*?\\)|<[^>]*>|https?://[^\\s]+");
    matches=syntax.globalMatch(result);
    while(matches.hasNext()) { const auto match=matches.next(); blank(match.capturedStart(),match.capturedLength()); }
    return result;
}
QStringList Backend::writingLanguages() const {
#ifdef Q_OS_MACOS
    extern QStringList macWritingLanguages(); return macWritingLanguages();
#else
    return {};
#endif
}
QVariantList Backend::writingIssues(const QString &text, const QString &language, bool grammar) {
#ifdef Q_OS_MACOS
    extern QVariantList macWritingIssues(const QString &, const QString &, bool);
    return macWritingIssues(proseForReview(text),language,grammar);
#else
    Q_UNUSED(text); Q_UNUSED(language); Q_UNUSED(grammar); return {};
#endif
}
bool Backend::correctWriting(int start,int end,const QString &expected,const QString &replacement) {
    if(!m_document || start<0 || end<=start || end>currentDocumentText().size() ||
       currentDocumentText().mid(start,end-start)!=expected || replacement.size()>1000) return false;
    QTextCursor cursor(m_document); cursor.setPosition(start); cursor.setPosition(end,QTextCursor::KeepAnchor);
    cursor.beginEditBlock(); cursor.insertText(replacement); cursor.endEditBlock(); return true;
}
void Backend::speakText(const QString &text) {
#ifdef Q_OS_MACOS
    extern void macSpeakText(const QString &); macSpeakText(proseForReview(text));
#else
    Q_UNUSED(text);
#endif
}
void Backend::stopSpeaking() {
#ifdef Q_OS_MACOS
    extern void macStopSpeaking(); macStopSpeaking();
#endif
}
QVariantList Backend::writingAnalysis(const QString &markdown, const QString &customWords) {
    const QString text=proseForReview(markdown);
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
    emit documentStatisticsChanged();
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
    if (data["version"].toInt() != 1 || data["sha256"].toString() != authorshipData()["sha256"].toString()) {
        emit documentStatisticsChanged();
        return;
    }
    const auto ranges = data["ranges"].toArray();
    if (ranges.size() > 100000) { emit documentStatisticsChanged(); return; }
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
    emit documentStatisticsChanged();
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

int Backend::pasteWithAuthorship(int start, int end) {
    if (!m_document) return -1;
    const auto *mime=QGuiApplication::clipboard()->mimeData();
    if (!mime || !mime->hasText()) return -1;
    QString text=mime->text();
    if (text.isEmpty()) return -1;
    const QByteArray payload=mime->data("application/x-omawrite-authorship+json");
    QJsonObject data;
    if (payload.size() <= 1024*1024) data=QJsonDocument::fromJson(payload).object();
    const bool valid=data["version"].toInt()==1 && data["sha256"].toString()==QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(),QCryptographicHash::Sha256).toHex())
        && data["ranges"].toArray().size() <= 10000 && !text.contains('\r');
    text.replace("\r\n", "\n").replace('\r','\n');
    const int first=qBound(0,qMin(start,end),currentDocumentText().size());
    const int last=qBound(first,qMax(start,end),currentDocumentText().size());
    QTextCursor cursor(m_document); cursor.beginEditBlock();
    cursor.setPosition(first); cursor.setPosition(last,QTextCursor::KeepAnchor);
    cursor.insertText(text,QTextCharFormat()); // external text must not inherit surrounding assertions
    if (valid) for (const auto &value : data["ranges"].toArray()) {
        const auto range=value.toObject(); const int left=range["start"].toInt(-1), right=range["end"].toInt(-1);
        const auto category=range["category"].toString();
        if (left<0 || right<=left || right>text.size() || !QStringList{"Human","AI","Reference"}.contains(category)) continue;
        QTextCharFormat format; format.setProperty(authorProperty,category+"\n"+range["author"].toString().left(200));
        cursor.setPosition(first+left); cursor.setPosition(first+right,QTextCursor::KeepAnchor); cursor.mergeCharFormat(format);
    }
    cursor.endEditBlock(); setModified(true); scheduleRecovery();
    return first+text.size();
}

bool Backend::exportAuthorship(const QUrl &destination) {
    const QFileInfo target(destination.toLocalFile()), source(m_fileUrl.toLocalFile());
    if (!destination.isLocalFile() || target.isSymLink() || target.absoluteFilePath()==source.absoluteFilePath()
        || (!target.canonicalFilePath().isEmpty() && target.canonicalFilePath()==source.canonicalFilePath())) {
        setStatus("Choose a metadata export path other than the document."); return false;
    }
    QSaveFile file(destination.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) { setStatus("Could not open authorship export."); return false; }
    file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
    auto data=authorshipData(); data["notice"]="Manual assertions, not verified provenance; ranges use UTF-16 offsets.";
    const auto bytes=QJsonDocument(data).toJson();
    if (file.write(bytes)!=bytes.size() || !file.commit()) { setStatus("Could not write authorship export."); return false; }
    setStatus("Authorship metadata exported; keep it with the exact Markdown text."); return true;
}

void Backend::paintOutput(QPagedPaintDevice &device, QTextDocument &document) const {
    // Normalize Markdown-import layout caches before paginating rich content.
    const QString html = document.toHtml();
    document.setHtml(html);
    // Lay out in points, then scale once for the output device. Mixing high-DPI
    // font metrics with unscaled block/table dimensions collapses table columns.
    const int dpi = device.logicalDpiX();
    const qreal scale = dpi / 72.0, margin = 24;
    const QRectF pixels = device.pageLayout().paintRectPixels(dpi);
    const QRectF page(0, 0, pixels.width() / scale, pixels.height() / scale);
    QImage metrics(1, 1, QImage::Format_ARGB32);
    metrics.setDotsPerMeterX(2835); metrics.setDotsPerMeterY(2835);
    document.documentLayout()->setPaintDevice(&metrics);
    const auto restoreDevice = qScopeGuard([&] { document.documentLayout()->setPaintDevice(nullptr); });
    const bool decorated=m_outputStyle!=0;
    const qreal inset=decorated?margin:0;
    const QSizeF content(page.width(),qMax(100.0,page.height()-2*inset));
    document.setPageSize(content);
    const int count=document.pageCount(), titlePages=(m_outputStyle==3 && m_outputTitlePage)?1:0;
    QPainter painter(&device);
    painter.scale(scale, scale);
    if(titlePages) { QFont titleFont(m_customOutputFont); titleFont.setPixelSize(24); painter.setFont(titleFont); painter.drawText(QRectF(0,0,page.width(),page.height()),Qt::AlignCenter|Qt::TextWordWrap,fileName()); device.newPage(); }
    for(int index=0;index<count;++index) {
        if(index) device.newPage();
        if(decorated) {
            auto expand=[&](QString text) { return text.replace("{title}",fileName()).replace("{page}",QString::number(index+1+titlePages)).replace("{pages}",QString::number(count+titlePages)); };
            QFont decorationFont("Helvetica Neue"); decorationFont.setPixelSize(9); painter.setFont(decorationFont); painter.setPen(Qt::black);
            painter.drawText(QRectF(0,0,page.width(),inset),Qt::AlignLeft|Qt::AlignVCenter,expand(m_outputStyle==3?m_outputHeader:"{title}"));
            painter.drawText(QRectF(0,page.height()-inset,page.width(),inset),Qt::AlignHCenter|Qt::AlignVCenter,expand(m_outputStyle==3?m_outputFooter:"{page} / {pages}"));
        }
        painter.save(); painter.translate(0,inset-index*content.height());
        const QRectF clip(0,index*content.height(),content.width(),content.height()); painter.setClipRect(clip);
        QAbstractTextDocumentLayout::PaintContext context; context.clip=clip; context.palette.setColor(QPalette::Text,Qt::black);
        document.documentLayout()->draw(&painter,context); painter.restore();
    }
}
