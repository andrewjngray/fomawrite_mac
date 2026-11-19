#include "pdfmarks.h"
#include "pdfsession.h"

#include <QSettings>

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

QString PdfMarks::pageText(int page) const {
    return page >= 1 && page <= m_session->pageCount() ? m_session->pageText(page - 1) : QString();
}

QVariantList PdfMarks::rects(int page, int start, int length) const {
    const QString key = QStringLiteral("%1:%2:%3").arg(page).arg(start).arg(length);
    if (auto it = m_rects.constFind(key); it != m_rects.cend()) return it.value();
    const QVariantList found = m_session->selectionAt(page, start, length).value(QStringLiteral("rects")).toList();
    m_rects.insert(key, found);
    return found;
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
            const int page = (placement.placed ? placement.page : item.page) - 1;
            const int direction = placement.placed ? m_session->textDirection(page) : 0;
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
    const Current now = current();
    for (int i = 0; i < now.document.items.size(); ++i) {
        const PdfNotes::Placement &placement = now.placements.at(i);
        if (now.document.items.at(i).kind == kind && placement.placed && placement.page == page + 1
                && placement.start == start && placement.length == length) {
            const PdfNotes::Edit edit = PdfNotes::removeMark(now.text, now.document.items.at(i), text.mid(start, length));
            if (edit.isEmpty() || !m_apply(edit)) return {};
            refresh();
            return QStringLiteral("removed");
        }
    }
    PdfNotes::Mark mark;
    mark.kind = kind;
    mark.colour = kind == QLatin1String("highlight") && !PdfNotes::highlightColours().contains(colour) ? QString() : colour;
    mark.page = page + 1;
    mark.start = start;
    mark.length = length;
    mark.text = text.mid(start, length);
    const PdfNotes::Edit edit = PdfNotes::insertMark(now.text, mark, m_session->fileName(), m_session->pageLabel(page));
    if (edit.isEmpty() || !m_apply(edit)) return {};
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
    if (edit.isEmpty() || !m_apply(edit)) return false;
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
    if (edit.isEmpty() || !m_apply(edit)) return false;
    refresh();
    return true;
}
