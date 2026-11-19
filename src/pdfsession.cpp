#include "pdfsession.h"

#include <QBuffer>
#include <QClipboard>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfLink>
#include <QPdfLinkModel>
#include <QPdfPageRenderer>
#include <QPdfSelection>
#include <QRegularExpression>
#include <QSettings>
#include <algorithm>
#include <cmath>
#include <functional>

namespace {
// A PDF read into memory rather than held open. Qt PDF otherwise keeps the
// file open for as long as the document lives, and Windows then refuses to
// rename, move or replace it (by Fomawrite's own Rename and page saves, or by
// another program regenerating the PDF). In memory, a rewrite on disk also
// cannot tear a page that is read later. Very large files keep the old
// behaviour so they do not occupy that much memory.
class UnlockedFilePdf : public QPdfDocument {
public:
    static constexpr qint64 MemoryLimit = 512ll * 1024 * 1024;
    ~UnlockedFilePdf() override { close(); } // releases m_buffer before it is destroyed
    QPdfDocument::Error loadFile(const QString &path) {
        QFile file(path);
        if (file.size() > MemoryLimit) return load(path);
        if (!file.open(QIODevice::ReadOnly)) return QPdfDocument::Error::FileNotFound;
        m_bytes = file.readAll();
        if (file.error() != QFileDevice::NoError) return QPdfDocument::Error::DataNotYetAvailable;
        file.close();
        m_buffer.setData(m_bytes);
        m_buffer.open(QIODevice::ReadOnly);
        load(&m_buffer);
        return error();
    }
private:
    QByteArray m_bytes;
    QBuffer m_buffer;
};

QString loadErrorText(QPdfDocument::Error error, const QString &name) {
    switch (error) {
    case QPdfDocument::Error::FileNotFound: return QStringLiteral("%1 could not be found.").arg(name);
    case QPdfDocument::Error::InvalidFileFormat: return QStringLiteral("%1 is damaged or is not a PDF.").arg(name);
    case QPdfDocument::Error::IncorrectPassword: return QStringLiteral("The password for %1 is not correct.").arg(name);
    case QPdfDocument::Error::UnsupportedSecurityScheme: return QStringLiteral("%1 uses a kind of encryption Fomawrite cannot open.").arg(name);
    default: return QStringLiteral("%1 could not be opened as a PDF.").arg(name);
    }
}

const QString RememberedViewsKey = QStringLiteral("pdf/rememberedViews");
const QString ThumbnailsKey = QStringLiteral("pdf/thumbnailsVisible");
const QString SidebarViewKey = QStringLiteral("pdf/sidebarView");

QVariantList normalisedRects(const QList<QPolygonF> &polygons) {
    QVariantList rects;
    for (const QPolygonF &polygon : polygons) rects.append(polygon.boundingRect().normalized());
    return rects;
}
}

PdfSession::PdfSession(QObject *parent) : QObject(parent) {
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(300);
    connect(&m_reloadTimer, &QTimer::timeout, this, &PdfSession::reloadFromDisk);
    m_searchTimer.setSingleShot(true);
    m_searchTimer.setInterval(0);
    connect(&m_searchTimer, &QTimer::timeout, this, &PdfSession::searchSlice);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_reloadTimer.start(); });
    // Atomic replacement drops the file from the watcher; the folder notices it
    // coming back (and notes files appearing beside it).
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        refreshNotesExist();
        const QString path = m_url.toLocalFile();
        if (!path.isEmpty() && QFileInfo::exists(path) && !m_watcher.files().contains(path)) m_reloadTimer.start();
    });
}

PdfSession::~PdfSession() {
    m_renderer.reset(); // before the document it renders
}

bool PdfSession::isPdfPath(const QString &path) {
    return QFileInfo(path).suffix().compare(QLatin1String("pdf"), Qt::CaseInsensitive) == 0;
}

QUrl PdfSession::notesUrlFor(const QUrl &pdfUrl) {
    if (!pdfUrl.isLocalFile()) return {};
    return QUrl::fromLocalFile(pdfUrl.toLocalFile() + QStringLiteral(".md"));
}

QString PdfSession::fileName() const {
    return m_url.isLocalFile() ? QFileInfo(m_url.toLocalFile()).fileName() : QString();
}

int PdfSession::pageCount() const { return ready() ? m_document->pageCount() : 0; }

bool PdfSession::ready() const {
    return m_document && m_state == QLatin1String("ready") && m_document->status() == QPdfDocument::Status::Ready;
}

void PdfSession::setRequestedPage(int page) {
    m_requestedPage = qBound(0, page, qMax(0, pageCount() - 1));
    emit requestedPageChanged();
}

void PdfSession::setState(const QString &state, const QString &error) {
    if (m_state == state && m_errorText == error) return;
    m_state = state;
    m_errorText = error;
    emit stateChanged();
}

bool PdfSession::load(const QUrl &url, const QString &password) {
    QUrl target = url;
    target.setFragment(QString());
    const QFileInfo file(target.toLocalFile());
    const QString name = file.fileName().isEmpty() ? QStringLiteral("The PDF") : file.fileName();
    const auto fail = [&](const QString &message) {
        m_lastError = message;
        if (!active()) setState(QStringLiteral("error"), message);
        return false;
    };
    if (!target.isLocalFile()) return fail(QStringLiteral("Only local PDF files can be opened."));
    if (!file.isFile()) return fail(QStringLiteral("%1 could not be found.").arg(name));
    target = QUrl::fromLocalFile(file.absoluteFilePath());
    auto document = std::make_unique<UnlockedFilePdf>();
    if (!password.isEmpty()) document->setPassword(password);
    const QPdfDocument::Error error = document->loadFile(file.absoluteFilePath());
    if (error == QPdfDocument::Error::None && document->status() == QPdfDocument::Status::Ready) {
        m_lastError.clear();
        adopt(std::move(document), target, password, QStringLiteral("ready"));
        return true;
    }
    if (error == QPdfDocument::Error::IncorrectPassword && password.isEmpty()) {
        m_lastError.clear();
        adopt(nullptr, target, QString(), QStringLiteral("password"));
        return true;
    }
    return fail(loadErrorText(error, name));
}

bool PdfSession::unlock(const QString &password) {
    if (m_state != QLatin1String("password") || !m_url.isLocalFile()) return false;
    auto document = std::make_unique<UnlockedFilePdf>();
    document->setPassword(password);
    const QPdfDocument::Error error = document->loadFile(m_url.toLocalFile());
    if (error != QPdfDocument::Error::None || document->status() != QPdfDocument::Status::Ready) {
        m_lastError = loadErrorText(error, fileName());
        return false;
    }
    m_lastError.clear();
    adopt(std::move(document), m_url, password, QStringLiteral("ready"));
    return true;
}

void PdfSession::adopt(std::unique_ptr<QPdfDocument> document, const QUrl &url, const QString &password, const QString &state) {
    const bool sameFile = m_url == url;
    const bool wasActive = active();
    const bool wasLocked = m_state == QLatin1String("password");
    m_reloadTimer.stop();
    resetSearch();
    m_renderer.reset(); // a pending render never touches the document it replaces
    m_document = std::move(document);
    if (m_document) {
        m_renderer = std::make_unique<QPdfPageRenderer>();
        m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
        m_renderer->setDocument(m_document.get());
    }
    m_pageText.clear();
    m_characterBoxes.clear();
    m_password = password;
    m_url = url;
    m_state = state; // announced below, after the document is in place
    m_errorText.clear();
    readInfo();
    watch();
    m_largestPageSize = QSizeF();
    for (int page = 0; page < pageCount(); ++page) {
        const QSizeF size = m_document->pagePointSize(page);
        m_largestPageSize = QSizeF(qMax(m_largestPageSize.width(), size.width()), qMax(m_largestPageSize.height(), size.height()));
    }
    const bool notesBefore = m_notesExist;
    m_notesExist = QFileInfo(notesUrl().toLocalFile()).isFile();
    // A newly opened (or newly unlocked) PDF returns to where it was last read;
    // a #page=N link sets the requested page after this.
    const int wanted = sameFile && !wasLocked ? m_requestedPage : rememberedView(url).value(QStringLiteral("page"), 0).toInt();
    m_requestedPage = qBound(0, wanted, qMax(0, pageCount() - 1));
    ++m_revision;
    if (!sameFile) emit urlChanged();
    emit stateChanged();
    if (notesBefore != m_notesExist) emit notesExistChanged();
    emit revisionChanged();
    if (!wasActive) emit activeChanged();
    if (!sameFile) emit opened();
}

bool PdfSession::relocate(const QUrl &url) {
    if (!active() || !url.isLocalFile()) return false;
    const QFileInfo file(url.toLocalFile());
    if (!file.isFile()) return false;
    const QUrl from = m_url;
    const QUrl target = QUrl::fromLocalFile(file.absoluteFilePath());
    if (m_state == QLatin1String("ready")) {
        auto document = std::make_unique<UnlockedFilePdf>();
        if (!m_password.isEmpty()) document->setPassword(m_password);
        const QPdfDocument::Error error = document->loadFile(file.absoluteFilePath());
        if (error != QPdfDocument::Error::None || document->status() != QPdfDocument::Status::Ready) return false;
        m_url = target; // the same file: adopt keeps the requested page and announces no new PDF
        adopt(std::move(document), target, m_password, QStringLiteral("ready"));
    } else {
        m_url = target;
        readInfo();
        watch();
        refreshNotesExist();
    }
    emit relocated(from);
    emit urlChanged();
    return true;
}

void PdfSession::close() {
    if (!active() && m_state == QLatin1String("none")) return;
    const bool wasActive = active();
    m_reloadTimer.stop();
    resetSearch();
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty()) m_watcher.removePaths(watched);
    m_renderer.reset();
    m_document.reset();
    m_pageText.clear();
    m_characterBoxes.clear();
    m_password.clear();
    m_info.clear();
    m_largestPageSize = QSizeF();
    m_url.clear();
    m_requestedPage = 0;
    const bool notesBefore = m_notesExist;
    m_notesExist = false;
    ++m_revision;
    setState(QStringLiteral("none"));
    emit urlChanged();
    if (notesBefore) emit notesExistChanged();
    emit revisionChanged();
    if (wasActive) emit activeChanged();
}

void PdfSession::readInfo() {
    m_info.clear();
    const QString path = m_url.toLocalFile();
    QFile file(path);
    QString version;
    bool encrypted = !m_password.isEmpty() || m_state == QLatin1String("password");
    if (file.open(QIODevice::ReadOnly)) {
        static const QRegularExpression header(QStringLiteral("%PDF-(\\d+\\.\\d+)"));
        const auto match = header.match(QString::fromLatin1(file.read(1024)));
        if (match.hasMatch()) version = match.captured(1);
        // A file encrypted with an empty user password opens without one; its
        // trailer (in the last few kilobytes) still names the /Encrypt dictionary.
        if (!encrypted && file.size() > 0) {
            file.seek(qMax<qint64>(0, file.size() - 65536));
            encrypted = file.read(65536).contains("/Encrypt");
        }
    }
    m_info = QVariantMap{{QStringLiteral("fileName"), fileName()}, {QStringLiteral("pdfVersion"), version},
                         {QStringLiteral("pageCount"), pageCount()}, {QStringLiteral("encrypted"), encrypted}};
    const QList<QPair<QString, QPdfDocument::MetaDataField>> fields = {
        {QStringLiteral("title"), QPdfDocument::MetaDataField::Title},
        {QStringLiteral("author"), QPdfDocument::MetaDataField::Author},
        {QStringLiteral("subject"), QPdfDocument::MetaDataField::Subject},
        {QStringLiteral("keywords"), QPdfDocument::MetaDataField::Keywords},
        {QStringLiteral("producer"), QPdfDocument::MetaDataField::Producer},
        {QStringLiteral("creator"), QPdfDocument::MetaDataField::Creator},
        {QStringLiteral("created"), QPdfDocument::MetaDataField::CreationDate},
        {QStringLiteral("modified"), QPdfDocument::MetaDataField::ModificationDate}};
    for (const auto &field : fields) {
        const QVariant value = ready() ? m_document->metaData(field.second) : QVariant();
        m_info.insert(field.first, value.metaType().id() == QMetaType::QDateTime
            ? (value.toDateTime().isValid() ? value.toDateTime().toString(Qt::ISODate) : QString())
            : value.toString());
    }
}

void PdfSession::watch() {
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty()) m_watcher.removePaths(watched);
    const QString path = m_url.toLocalFile();
    if (path.isEmpty()) return;
    if (QFileInfo::exists(path)) m_watcher.addPath(path);
    const QString folder = QFileInfo(path).absolutePath();
    if (QFileInfo(folder).isDir()) m_watcher.addPath(folder);
}

void PdfSession::reloadFromDisk() {
    if (!active()) return;
    const QString path = m_url.toLocalFile();
    if (!QFileInfo::exists(path)) {
        setState(QStringLiteral("error"), QStringLiteral("%1 is no longer available.").arg(fileName()));
        watch();
        return;
    }
    if (m_state == QLatin1String("password")) { watch(); return; }
    auto document = std::make_unique<UnlockedFilePdf>();
    if (!m_password.isEmpty()) document->setPassword(m_password);
    const QPdfDocument::Error error = document->loadFile(path);
    if (error == QPdfDocument::Error::None && document->status() == QPdfDocument::Status::Ready) {
        adopt(std::move(document), m_url, m_password, QStringLiteral("ready"));
        return;
    }
    // Keep the open document drawable? Its file is gone or rewritten, so say so.
    resetSearch();
    m_renderer.reset();
    m_document.reset();
    m_pageText.clear();
    m_characterBoxes.clear();
    ++m_revision;
    setState(QStringLiteral("error"), loadErrorText(error, fileName()));
    emit revisionChanged();
    watch();
}

void PdfSession::refreshNotesExist() {
    const bool exists = active() && QFileInfo(notesUrl().toLocalFile()).isFile();
    if (exists == m_notesExist) return;
    m_notesExist = exists;
    emit notesExistChanged();
}

QSizeF PdfSession::pageSize(int page) const {
    if (!ready() || page < 0 || page >= m_document->pageCount()) return {};
    return m_document->pagePointSize(page);
}

QString PdfSession::pageLabel(int page) const {
    if (!ready() || page < 0 || page >= m_document->pageCount()) return {};
    const QString label = m_document->pageLabel(page);
    return label.isEmpty() ? QString::number(page + 1) : label;
}

QString PdfSession::pageText(int page) const {
    if (!ready() || page < 0 || page >= m_document->pageCount()) return {};
    auto it = m_pageText.constFind(page);
    if (it == m_pageText.cend()) it = m_pageText.insert(page, m_document->getAllText(page).text());
    return it.value();
}

QVariantMap PdfSession::selectionMap(int page, const QPdfSelection &selection) const {
    if (!selection.isValid()) return {};
    const int length = selection.endIndex() - selection.startIndex();
    return {{QStringLiteral("text"), selection.text()}, {QStringLiteral("start"), selection.startIndex()},
            {QStringLiteral("length"), length}, {QStringLiteral("rects"), lineRects(page, selection.startIndex(), length)}};
}

// One rectangle per line of the page's text. Some producers (Qt's own
// QPdfWriter among them) place every glyph on its own, and Qt PDF then returns
// a box per glyph; a line's boxes are joined when they lie on one line, across
// or (on a turned page) down it, and kept apart otherwise.
QVariantList PdfSession::lineRects(int page, int start, int length) const {
    QVariantList rects;
    const QString text = pageText(page);
    const int end = qMin(int(text.size()), start + length);
    for (int from = start; from < end;) {
        int to = from;
        while (to < end && text.at(to) != QLatin1Char('\n') && text.at(to) != QLatin1Char('\r')) ++to;
        if (to > from) {
            const QList<QPolygonF> polygons = m_document->getSelectionAtIndex(page, from, to - from).bounds();
            QRectF united;
            qreal tallest = 0, widest = 0;
            for (const QPolygonF &polygon : polygons) {
                const QRectF box = polygon.boundingRect().normalized();
                united |= box;
                tallest = qMax(tallest, box.height());
                widest = qMax(widest, box.width());
            }
            if (polygons.size() == 1 || united.height() <= 1.5 * tallest || united.width() <= 1.5 * widest) {
                if (!united.isEmpty()) rects.append(united);
            } else {
                rects += normalisedRects(polygons);
            }
        }
        from = to + 1;
    }
    return rects;
}

// Qt's getSelection(page, from, to) drops the last character on pages with
// /Rotate 90 or 270, so points are matched to characters here, by their boxes.
const QList<QRectF> &PdfSession::characterBoxes(int page) const {
    auto it = m_characterBoxes.find(page);
    if (it != m_characterBoxes.end()) return it.value();
    QList<QRectF> boxes;
    const QString text = pageText(page);
    boxes.reserve(text.size());
    for (int i = 0; i < text.size(); ++i) {
        QRectF box;
        if (!text.at(i).isSpace())
            for (const QPolygonF &polygon : m_document->getSelectionAtIndex(page, i, 1).bounds()) box |= polygon.boundingRect().normalized();
        boxes.append(box);
    }
    return m_characterBoxes.insert(page, boxes).value();
}

// The character under `point`, else the nearest one within half an inch; -1 if none.
int PdfSession::characterNear(int page, QPointF point) const {
    const QList<QRectF> &boxes = characterBoxes(page);
    int nearest = -1;
    qreal best = 36;
    for (int i = 0; i < boxes.size(); ++i) {
        const QRectF &box = boxes.at(i);
        if (box.isEmpty()) continue;
        if (box.contains(point)) return i;
        const qreal dx = qMax<qreal>(0, qMax(box.left() - point.x(), point.x() - box.right()));
        const qreal dy = qMax<qreal>(0, qMax(box.top() - point.y(), point.y() - box.bottom()));
        const qreal distance = std::hypot(dx, dy);
        if (distance < best) { best = distance; nearest = i; }
    }
    return nearest;
}

QVariantMap PdfSession::selectionBetween(int page, QPointF from, QPointF to) const {
    if (!ready() || page < 0 || page >= m_document->pageCount()) return {};
    const int first = characterNear(page, from), last = characterNear(page, to);
    if (first < 0 || last < 0) return {};
    return selectionMap(page, m_document->getSelectionAtIndex(page, qMin(first, last), qAbs(last - first) + 1));
}

QVariantMap PdfSession::selectionAt(int page, int start, int length) const {
    if (!ready() || page < 0 || page >= m_document->pageCount() || start < 0 || length <= 0) return {};
    return selectionMap(page, m_document->getSelectionAtIndex(page, start, length));
}

QVariantMap PdfSession::wordAt(int page, QPointF point) const {
    if (!ready() || page < 0 || page >= m_document->pageCount()) return {};
    const QString text = pageText(page);
    const QList<QRectF> &boxes = characterBoxes(page);
    int hit = -1;
    for (int i = 0; i < boxes.size() && hit < 0; ++i)
        if (!boxes.at(i).isEmpty() && boxes.at(i).adjusted(-1, -1, 1, 1).contains(point)) hit = i;
    const auto inWord = [&](int i) { return i >= 0 && i < text.size() && (text.at(i).isLetterOrNumber() || text.at(i) == QLatin1Char('_')); };
    if (!inWord(hit)) return {};
    int start = hit, end = hit + 1;
    while (inWord(start - 1)) --start;
    while (inWord(end)) ++end;
    return selectionMap(page, m_document->getSelectionAtIndex(page, start, end - start));
}

QVariantList PdfSession::pageMatches(int page, const QString &query, int limit) const {
    QVariantList results;
    const QString text = pageText(page);
    for (qsizetype at = text.indexOf(query, 0, Qt::CaseInsensitive); at >= 0 && results.size() < limit;
         at = text.indexOf(query, at + query.size(), Qt::CaseInsensitive)) {
        const qsizetype from = qMax<qsizetype>(0, at - 30);
        results.append(QVariantMap{{QStringLiteral("page"), page}, {QStringLiteral("start"), int(at)},
            {QStringLiteral("length"), int(query.size())},
            {QStringLiteral("rects"), lineRects(page, int(at), int(query.size()))},
            {QStringLiteral("context"), text.mid(from, at - from + query.size() + 30).simplified()}});
    }
    return results;
}

QVariantList PdfSession::search(const QString &query) const {
    QVariantList results;
    if (!ready() || query.isEmpty()) return results;
    for (int page = 0; page < m_document->pageCount() && results.size() < SearchLimit; ++page)
        results += pageMatches(page, query, SearchLimit - int(results.size()));
    return results;
}

void PdfSession::startSearch(const QString &query) {
    const bool hadResults = !m_searchResults.isEmpty() || !m_searchQuery.isEmpty();
    m_searchTimer.stop();
    m_searchQuery = query;
    m_searchResults.clear();
    m_searchPage = 0;
    const bool searching = ready() && !query.isEmpty();
    if (hadResults || searching) emit searchResultsChanged();
    if (m_searching != searching) { m_searching = searching; emit searchingChanged(); }
    if (searching) searchSlice();
}

// Text extraction and match geometry cost about half a millisecond a page on
// dense text, so pages are taken about 12 ms at a time.
void PdfSession::searchSlice() {
    if (!m_searching) return;
    if (!ready()) { resetSearch(); return; }
    QElapsedTimer clock;
    clock.start();
    const int before = int(m_searchResults.size());
    while (m_searchPage < m_document->pageCount() && m_searchResults.size() < SearchLimit && clock.elapsed() < 12)
        m_searchResults += pageMatches(m_searchPage++, m_searchQuery, SearchLimit - int(m_searchResults.size()));
    const bool done = m_searchPage >= m_document->pageCount() || m_searchResults.size() >= SearchLimit;
    if (m_searchResults.size() != before) emit searchResultsChanged();
    if (done) { m_searching = false; emit searchingChanged(); }
    else m_searchTimer.start();
}

void PdfSession::clearSearch() { startSearch(QString()); }

void PdfSession::resetSearch() {
    m_searchTimer.stop();
    const bool hadResults = !m_searchResults.isEmpty() || !m_searchQuery.isEmpty();
    m_searchQuery.clear();
    m_searchResults.clear();
    m_searchPage = 0;
    if (hadResults) emit searchResultsChanged();
    if (m_searching) { m_searching = false; emit searchingChanged(); }
}

QVariantList PdfSession::links(int page) const {
    QVariantList result;
    if (!ready() || page < 0 || page >= m_document->pageCount()) return result;
    QPdfLinkModel model;
    model.setDocument(m_document.get());
    model.setPage(page);
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        const QPdfLink link = model.data(model.index(row), int(QPdfLinkModel::Role::Link)).value<QPdfLink>();
        // QPdfLink::isValid() is false for web links (they have no page).
        if (link.rectangles().isEmpty() || (link.page() < 0 && link.url().isEmpty())) continue;
        QRectF united;
        QVariantList rects;
        for (const QRectF &rect : link.rectangles()) { rects.append(rect.normalized()); united |= rect.normalized(); }
        result.append(QVariantMap{{QStringLiteral("rect"), united}, {QStringLiteral("rects"), rects},
            {QStringLiteral("page"), link.url().isEmpty() ? link.page() : -1},
            {QStringLiteral("location"), link.location()}, {QStringLiteral("url"), link.url()}});
    }
    return result;
}

QVariantList PdfSession::outline() const {
    QVariantList result;
    if (!ready()) return result;
    QPdfBookmarkModel model;
    model.setDocument(m_document.get());
    const std::function<void(const QModelIndex &)> walk = [&](const QModelIndex &parent) {
        for (int row = 0; row < model.rowCount(parent); ++row) {
            const QModelIndex index = model.index(row, 0, parent);
            result.append(QVariantMap{{QStringLiteral("title"), model.data(index, int(QPdfBookmarkModel::Role::Title)).toString()},
                {QStringLiteral("level"), model.data(index, int(QPdfBookmarkModel::Role::Level)).toInt()},
                {QStringLiteral("page"), model.data(index, int(QPdfBookmarkModel::Role::Page)).toInt()}});
            walk(index);
        }
    };
    walk(QModelIndex());
    return result;
}

void PdfSession::copyText(const QString &text) const {
    if (auto *clipboard = QGuiApplication::clipboard()) clipboard->setText(text);
}

QString PdfSession::linkDestination(const QString &fileName) {
    QString result;
    for (const QChar c : fileName) {
        if (c.isSpace() || QStringLiteral("%()<>#?[]\\").contains(c)) {
            for (const char byte : QString(c).toUtf8())
                result += QStringLiteral("%%1").arg(uchar(byte), 2, 16, QLatin1Char('0')).toUpper();
        } else {
            result += c;
        }
    }
    return result;
}

QString PdfSession::markdownQuote(const QString &text, const QString &fileName, int page, const QString &label) {
    QString normalised = text;
    normalised.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    for (const QChar separator : {QChar(u'\r'), QChar(QChar::LineSeparator), QChar(QChar::ParagraphSeparator)})
        normalised.replace(separator, QLatin1Char('\n'));
    static const QRegularExpression runs(QStringLiteral("[\\t\\f\\v \\x{00A0}]+"));
    QStringList lines;
    bool blank = false;
    for (QString line : normalised.split(QLatin1Char('\n'))) {
        line = line.replace(runs, QStringLiteral(" ")).trimmed();
        if (line.isEmpty()) { blank = !lines.isEmpty(); continue; }
        if (blank) lines.append(QStringLiteral(">"));
        blank = false;
        lines.append(QStringLiteral("> ") + line);
    }
    const QString number = QString::number(page + 1);
    QString name = label.isEmpty() || label == number ? number : label;
    name.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('['), QStringLiteral("\\[")).replace(QLatin1Char(']'), QStringLiteral("\\]"));
    return lines.join(QLatin1Char('\n')) + QStringLiteral("\n\n[Page %1](%2#page=%3)\n").arg(name, linkDestination(fileName), number);
}

QString PdfSession::markdownQuoteFor(int page, const QString &text) const {
    if (!ready() || page < 0 || page >= pageCount() || text.trimmed().isEmpty()) return {};
    return markdownQuote(text, fileName(), page, pageLabel(page));
}

bool PdfSession::copyMarkdownQuote(int page, const QString &text) const {
    const QString quote = markdownQuoteFor(page, text);
    if (quote.isEmpty()) return false;
    copyText(quote);
    return true;
}

QString PdfSession::pageLinkAddress(int page) const {
    if (!active() || page < 0) return {};
    return linkDestination(fileName()) + QStringLiteral("#page=%1").arg(page + 1);
}

bool PdfSession::thumbnailsVisible() const { return QSettings().value(ThumbnailsKey, false).toBool(); }

void PdfSession::setThumbnailsVisible(bool visible) {
    if (visible == thumbnailsVisible()) return;
    QSettings().setValue(ThumbnailsKey, visible);
    emit thumbnailsVisibleChanged();
}

QString PdfSession::sidebarView() const {
    return QSettings().value(SidebarViewKey).toString() == QLatin1String("contents") ? QStringLiteral("contents") : QStringLiteral("thumbnails");
}

void PdfSession::setSidebarView(const QString &view) {
    const QString value = view == QLatin1String("contents") ? QStringLiteral("contents") : QStringLiteral("thumbnails");
    if (value == sidebarView()) return;
    QSettings().setValue(SidebarViewKey, value);
    emit sidebarViewChanged();
}

void PdfSession::noteRenderRequest() {
    ++m_renderRequestCount;
    emit renderRequestCountChanged();
}

QString PdfSession::viewKey(const QUrl &url) {
    QUrl target = url;
    target.setFragment(QString());
    if (!target.isLocalFile()) return {};
    const QFileInfo file(target.toLocalFile());
    const QString canonical = file.canonicalFilePath();
    return canonical.isEmpty() ? file.absoluteFilePath() : canonical;
}

QVariantMap PdfSession::rememberedView(const QUrl &url) const {
    const QString key = viewKey(url.isEmpty() ? m_url : url);
    if (key.isEmpty()) return {};
    for (const QVariant &value : QSettings().value(RememberedViewsKey).toList()) {
        const QVariantMap view = value.toMap();
        if (view.value(QStringLiteral("path")).toString() == key) return view;
    }
    return {};
}

void PdfSession::moveRememberedView(const QString &fromKey, const QUrl &to) {
    const QString toKey = viewKey(to);
    if (fromKey.isEmpty() || toKey.isEmpty() || fromKey == toKey) return;
    QSettings settings;
    QVariantList views = settings.value(RememberedViewsKey).toList();
    bool changed = false;
    // A view already written under the new path (the open reader) is the newer one.
    const auto hasKey = [&](const QString &key) {
        return std::any_of(views.cbegin(), views.cend(), [&](const QVariant &value) { return value.toMap().value(QStringLiteral("path")).toString() == key; });
    };
    if (hasKey(toKey)) {
        if (views.removeIf([&](const QVariant &value) { return value.toMap().value(QStringLiteral("path")).toString() == fromKey; }))
            settings.setValue(RememberedViewsKey, views);
        return;
    }
    for (QVariant &value : views) {
        QVariantMap view = value.toMap();
        if (view.value(QStringLiteral("path")).toString() != fromKey) continue;
        view.insert(QStringLiteral("path"), toKey);
        value = view;
        changed = true;
    }
    if (changed) settings.setValue(RememberedViewsKey, views);
}

void PdfSession::forgetRememberedView(const QString &key) {
    if (key.isEmpty()) return;
    QSettings settings;
    QVariantList views = settings.value(RememberedViewsKey).toList();
    if (views.removeIf([&](const QVariant &value) { return value.toMap().value(QStringLiteral("path")).toString() == key; }))
        settings.setValue(RememberedViewsKey, views);
}

void PdfSession::rememberView(const QUrl &url, int page, const QString &mode, qreal zoom) {
    const QString key = viewKey(url);
    if (key.isEmpty() || page < 0) return;
    QSettings settings;
    QVariantList views = settings.value(RememberedViewsKey).toList();
    const QVariantMap view{{QStringLiteral("path"), key}, {QStringLiteral("page"), page},
                           {QStringLiteral("mode"), mode}, {QStringLiteral("zoom"), zoom}};
    if (!views.isEmpty() && views.constFirst().toMap() == view) return;
    views.removeIf([&](const QVariant &value) { return value.toMap().value(QStringLiteral("path")).toString() == key; });
    views.prepend(view);
    while (views.size() > RememberedViewLimit) views.removeLast();
    settings.setValue(RememberedViewsKey, views);
}
