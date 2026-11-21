#include "pdfmarks.h"
#include "pdfsession.h"

#include <QDateTime>
#include <QSettings>
#include <QUrl>
#include <cmath>

namespace {
const QString ToolKey = QStringLiteral("pdf/markupTool");
const QString ColourKey = QStringLiteral("pdf/highlightColour");
}

PdfMarks::PdfMarks(PdfSession *session, TextSource text, EditSink apply, QObject *parent)
    : QObject(parent), m_session(session), m_text(std::move(text)), m_apply(std::move(apply)) {
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(RefreshDelay);
    connect(&m_refreshTimer, &QTimer::timeout, this, &PdfMarks::refresh);
    const auto reset = [this] { m_rects.clear(); scheduleRefresh(); };
    connect(session, &PdfSession::revisionChanged, this, reset);
    connect(session, &PdfSession::stateChanged, this, reset);
    connect(session, &PdfSession::urlChanged, this, reset);
    connect(session, &PdfSession::activeChanged, this, reset);
}

QString PdfMarks::tool() const {
    const QString tool = QSettings().value(ToolKey).toString();
    return PdfNotes::isTextMarkup(tool) ? tool : QStringLiteral("highlight");
}

void PdfMarks::setTool(const QString &tool) {
    if (!PdfNotes::isTextMarkup(tool) || tool == this->tool()) return;
    QSettings().setValue(ToolKey, tool);
    emit toolChanged();
}

QString PdfMarks::highlightColour() const {
    const QString colour = QSettings().value(ColourKey).toString();
    return PdfNotes::highlightColours().contains(colour) ? colour : QStringLiteral("yellow");
}

void PdfMarks::setHighlightColour(const QString &colour) {
    if (!PdfNotes::highlightColours().contains(colour) || colour == highlightColour()) return;
    QSettings().setValue(ColourKey, colour);
    emit toolChanged();
}

bool PdfMarks::ready() const { return m_session->active() && m_session->state() == QLatin1String("ready"); }

// `page` is a 1-based BASELINE page, as the notes number them; empty when it was removed.
QString PdfMarks::pageText(int page) const {
    const int working = m_session->workingPageForBaseline(page - 1);
    return working >= 0 && working < m_session->pageCount() ? m_session->pageText(working) : QString();
}

QVariantList PdfMarks::rects(int page, int start, int length) const {
    const QString key = QStringLiteral("%1:%2:%3").arg(page).arg(start).arg(length);
    if (auto it = m_rects.constFind(key); it != m_rects.cend()) return it.value();
    const QVariantList found = m_session->selectionAt(page, start, length).value(QStringLiteral("rects")).toList();
    m_rects.insert(key, found);
    return found;
}

bool PdfMarks::applyEdit(const PdfNotes::Edit &edit) {
    emit editStarting();
    return m_apply(edit);
}

static QString noteAuthor(const PdfNotes::Item &item) {
    return QUrl::fromPercentEncoding(item.value(QStringLiteral("author")).toLatin1());
}

// A note's text as plain text for the page's popover: its lines trimmed, blank lines kept, cut at 800 characters.
static QString noteText(const QString &text, const PdfNotes::Item &item) {
    if (!item.hasBody) return {};
    QStringList lines;
    for (const QString &line : text.mid(item.bodyStart, item.contentEnd - item.bodyStart).split(QLatin1Char('\n')))
        lines.append(line.trimmed());
    QString plain = lines.join(QLatin1Char('\n'));
    if (plain.size() > 800) plain = plain.left(800).trimmed() + QChar(0x2026);
    return plain;
}

QPoint PdfMarks::clampedPoint(int page, double x, double y) const {
    const QSizeF size = m_session->pageSize(page);
    return {int(qBound(0.0, std::round(x), std::floor(size.width()))), int(qBound(0.0, std::round(y), std::floor(size.height())))};
}

// A point on working page `page` (view points) as the notes keep it: in the baseline page's own coordinates.
QPoint PdfMarks::baselinePoint(int page, double x, double y) const {
    const QPoint clamped = clampedPoint(page, x, y);
    if (m_session->rotationOfWorkingPage(page) == 0) return clamped;
    const QPointF turned = m_session->workingPointToBaseline(page, clamped);
    const QSizeF size = m_session->baselineSizeOfWorkingPage(page);
    return {int(qBound(0.0, std::round(turned.x()), std::floor(size.width()))), int(qBound(0.0, std::round(turned.y()), std::floor(size.height())))};
}

bool PdfMarks::refuseInserted(int page) const {
    if (m_session->baselinePageForWorking(page) >= 0) return false;
    m_session->setPageEditError(QStringLiteral("Save the PDF before marking inserted pages."));
    return true;
}

PdfMarks::Current PdfMarks::current() const {
    Current now;
    now.text = m_text();
    now.document = PdfNotes::parse(now.text);
    for (const PdfNotes::Item &item : std::as_const(now.document.items))
        now.placements.append(PdfNotes::isTextMarkup(item.kind)
            ? PdfNotes::place(item, [this](int page) { return pageText(page); }) : PdfNotes::Placement{});
    return now;
}

void PdfMarks::scheduleRefresh() {
    if (!ready() && m_items.isEmpty()) return;
    m_refreshTimer.start();
}

void PdfMarks::refresh() {
    m_refreshTimer.stop();
    QVariantList items;
    if (ready()) {
        const Current now = current();
        for (int i = 0; i < now.document.items.size(); ++i) {
            const PdfNotes::Item &item = now.document.items.at(i);
            const PdfNotes::Placement &placement = now.placements.at(i);
            // The notes number pages as the PDF was last saved; the pane shows the working pages.
            const int page = placement.placed ? m_session->workingPageForBaseline(placement.page - 1) : item.page - 1;
            const int direction = placement.placed ? m_session->textDirection(page) : 0;
            if (item.kind == QLatin1String("note")) {
                // A note whose page does not exist, or whose at= lies outside its page, is unplaced.
                const int working = m_session->workingPageForBaseline(item.page - 1);
                bool valid = item.hasAt && item.page >= 1 && working >= 0;
                if (valid) {
                    const QSizeF size = m_session->baselineSizeOfWorkingPage(working);
                    valid = item.atX >= 0 && item.atY >= 0 && item.atX <= size.width() && item.atY <= size.height();
                }
                QPoint at = valid ? QPoint(item.atX, item.atY) : QPoint();
                if (valid && m_session->rotationOfWorkingPage(working) != 0)
                    at = m_session->baselinePointToWorking(working, QPointF(item.atX, item.atY)).toPoint();
                items.append(QVariantMap{
                    {QStringLiteral("index"), i},
                    {QStringLiteral("kind"), item.kind},
                    {QStringLiteral("page"), valid ? working : item.page - 1},
                    {QStringLiteral("x"), at.x()},
                    {QStringLiteral("y"), at.y()},
                    {QStringLiteral("placed"), valid},
                    {QStringLiteral("text"), noteText(now.text, item)},
                    {QStringLiteral("author"), noteAuthor(item)},
                    {QStringLiteral("marker"), now.text.mid(item.markerStart, item.markerEnd - item.markerStart)}});
                continue;
            }
            items.append(QVariantMap{
                {QStringLiteral("index"), i},
                {QStringLiteral("kind"), item.kind},
                {QStringLiteral("colour"), item.colour.isEmpty() ? PdfNotes::defaultColour(item.kind) : item.colour},
                {QStringLiteral("page"), page},
                {QStringLiteral("start"), placement.placed ? placement.start : item.start},
                {QStringLiteral("length"), placement.placed ? placement.length : item.length},
                {QStringLiteral("placed"), placement.placed},
                {QStringLiteral("rects"), placement.placed ? rects(page, placement.start, placement.length) : QVariantList()},
                {QStringLiteral("direction"), direction},
                {QStringLiteral("vertical"), direction == 90 || direction == 270},
                {QStringLiteral("quote"), item.quote},
                {QStringLiteral("marker"), now.text.mid(item.markerStart, item.markerEnd - item.markerStart)}});
        }
    }
    if (items == m_items) return;
    m_items = items;
    emit itemsChanged();
}

// The item an items entry stands for in the current text: the same marker
// line, preferring the same position in the list (the text may have changed
// since the list was made).
int PdfMarks::currentIndex(const Current &now, int index) const {
    if (index < 0 || index >= m_items.size()) return -1;
    const QString marker = m_items.at(index).toMap().value(QStringLiteral("marker")).toString();
    const auto markerOf = [&](int i) {
        const PdfNotes::Item &item = now.document.items.at(i);
        return now.text.mid(item.markerStart, item.markerEnd - item.markerStart);
    };
    if (index < now.document.items.size() && markerOf(index) == marker) return index;
    for (int i = 0; i < now.document.items.size(); ++i)
        if (markerOf(i) == marker) return i;
    return -1;
}

QString PdfMarks::mark(const QString &kind, const QString &colour, int page, int start, int length) {
    if (!ready() || !PdfNotes::isTextMarkup(kind) || page < 0 || page >= m_session->pageCount() || start < 0 || length <= 0) return {};
    const QString text = m_session->pageText(page);
    if (start + length > text.size()) return {};
    if (refuseInserted(page)) return {};
    const int baselinePage = m_session->baselinePageForWorking(page);
    const Current now = current();
    for (int i = 0; i < now.document.items.size(); ++i) {
        const PdfNotes::Placement &placement = now.placements.at(i);
        if (now.document.items.at(i).kind == kind && placement.placed && placement.page == baselinePage + 1
                && placement.start == start && placement.length == length) {
            const PdfNotes::Edit edit = PdfNotes::removeMark(now.text, now.document.items.at(i), text.mid(start, length));
            if (edit.isEmpty() || !applyEdit(edit)) return {};
            refresh();
            return QStringLiteral("removed");
        }
    }
    PdfNotes::Mark mark;
    mark.kind = kind;
    mark.colour = kind == QLatin1String("highlight") && !PdfNotes::highlightColours().contains(colour) ? QString() : colour;
    mark.page = baselinePage + 1;
    mark.start = start;
    mark.length = length;
    mark.text = text.mid(start, length);
    const PdfNotes::Edit edit = PdfNotes::insertMark(now.text, mark, m_session->fileName(), m_session->linkLabel(page));
    if (edit.isEmpty() || !applyEdit(edit)) return {};
    refresh();
    return QStringLiteral("added");
}

bool PdfMarks::removeMark(int index) {
    if (!ready()) return false;
    const Current now = current();
    const int i = currentIndex(now, index);
    if (i < 0) return false;
    const PdfNotes::Placement &placement = now.placements.at(i);
    const QString written = placement.placed ? pageText(placement.page).mid(placement.start, placement.length) : QString();
    const PdfNotes::Edit edit = PdfNotes::removeMark(now.text, now.document.items.at(i), written);
    if (edit.isEmpty() || !applyEdit(edit)) return false;
    refresh();
    return true;
}

bool PdfMarks::recolourMark(int index, const QString &colour) {
    if (!ready()) return false;
    const Current now = current();
    const int i = currentIndex(now, index);
    if (i < 0) return false;
    const PdfNotes::Item &item = now.document.items.at(i);
    if (item.kind == QLatin1String("highlight") && !PdfNotes::highlightColours().contains(colour)) return false;
    const PdfNotes::Edit edit = PdfNotes::recolourMark(now.text, item, colour);
    if (edit.isEmpty() || !applyEdit(edit)) return false;
    refresh();
    return true;
}

static QString draftBody(QString body) {
    body.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    body.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    while (body.startsWith(QLatin1Char('\n'))) body.remove(0, 1);
    while (body.endsWith(QLatin1Char('\n'))) body.chop(1);
    return body;
}

int PdfMarks::placeNote(int page, double x, double y, const QString &body, const QString &author) {
    if (!ready() || page < 0 || page >= m_session->pageCount()) return -1;
    if (refuseInserted(page)) return -1;
    const QPoint at = baselinePoint(page, x, y);
    PdfNotes::Note note;
    note.page = m_session->baselinePageForWorking(page) + 1;
    note.x = at.x();
    note.y = at.y();
    if (!body.trimmed().isEmpty()) {
        note.author = author.trimmed();
#if defined(Q_OS_WIN)
        if (note.author.isEmpty()) note.author = qEnvironmentVariable("USERNAME").trimmed();
#elif defined(Q_OS_MACOS)
        if (note.author.isEmpty()) note.author = qEnvironmentVariable("USER").trimmed();
#endif
        note.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }
    const PdfNotes::NoteInsert insert = PdfNotes::insertNote(m_text(), note, m_session->fileName(),
                                                            m_session->linkLabel(page), draftBody(body));
    if (insert.edit.isEmpty() || !applyEdit(insert.edit)) return -1;
    refresh();
    return insert.caret;
}

QVariantMap PdfMarks::noteDraft(int index) {
    if (!ready()) return {};
    const Current now = current();
    const int i = currentIndex(now, index);
    if (i < 0) return {};
    const PdfNotes::Item &item = now.document.items.at(i);
    if (item.kind != QLatin1String("note")) return {};
    return {
        {QStringLiteral("marker"), now.text.mid(item.markerStart, item.markerEnd - item.markerStart)},
        {QStringLiteral("body"), now.text.mid(item.bodyStart, item.contentEnd - item.bodyStart)},
        {QStringLiteral("author"), noteAuthor(item)},
        {QStringLiteral("created"), item.value(QStringLiteral("created"))}
    };
}

bool PdfMarks::validNoteDraft(const QString &body) const {
    return PdfNotes::canUseNoteBody(draftBody(body));
}

int PdfMarks::matchingNoteIndex(const Current &now, int index, const QString &marker, const QString &previous) const {
    const int i = currentIndex(now, index);
    if (i < 0) return -1;
    const PdfNotes::Item &item = now.document.items.at(i);
    if (item.kind != QLatin1String("note")
        || now.text.mid(item.markerStart, item.markerEnd - item.markerStart) != marker
        || now.text.mid(item.bodyStart, item.contentEnd - item.bodyStart) != previous) return -1;
    return i;
}

bool PdfMarks::editNote(int index, const QString &marker, const QString &previous, const QString &body) {
    if (!ready()) return false;
    const Current now = current();
    const int i = matchingNoteIndex(now, index, marker, previous);
    if (i < 0) return false;
    if (body == previous) return true;
    const PdfNotes::Edit edit = PdfNotes::replaceNoteBody(now.text, now.document.items.at(i), draftBody(body));
    if (edit.isEmpty() || !applyEdit(edit)) return false;
    refresh();
    return true;
}

bool PdfMarks::deleteNote(int index, const QString &marker, const QString &previous) {
    if (!ready()) return false;
    const Current now = current();
    const int i = matchingNoteIndex(now, index, marker, previous);
    if (i < 0) return false;
    const PdfNotes::Edit edit = PdfNotes::removeMark(now.text, now.document.items.at(i), {}, true);
    if (edit.isEmpty() || !applyEdit(edit)) return false;
    refresh();
    return true;
}

bool PdfMarks::moveNote(int index, double x, double y) {
    if (!ready()) return false;
    const Current now = current();
    const int i = currentIndex(now, index);
    if (i < 0) return false;
    const PdfNotes::Item &item = now.document.items.at(i);
    const int working = m_session->workingPageForBaseline(item.page - 1);
    if (item.kind != QLatin1String("note") || item.page < 1 || working < 0) return false;
    const QPoint at = baselinePoint(working, x, y);
    const PdfNotes::Edit edit = PdfNotes::moveNote(now.text, item, at.x(), at.y());
    if (edit.isEmpty() || !applyEdit(edit)) return false;
    refresh();
    return true;
}

int PdfMarks::bodyPosition(int index) {
    if (!ready()) return -1;
    const Current now = current();
    const int i = currentIndex(now, index);
    return i < 0 ? -1 : now.document.items.at(i).bodyStart;
}

QVariantMap PdfMarks::itemAt(int position) {
    if (!ready() || position < 0) return {};
    const Current now = current();
    for (int i = 0; i < now.document.items.size(); ++i) {
        const PdfNotes::Item &item = now.document.items.at(i);
        if (position < item.markerStart || position > item.contentEnd) continue;
        return {{QStringLiteral("index"), i}, {QStringLiteral("marker"), now.text.mid(item.markerStart, item.markerEnd - item.markerStart)}};
    }
    return {};
}
