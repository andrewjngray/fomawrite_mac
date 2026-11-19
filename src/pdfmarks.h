#pragma once

#include "pdfnotes.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <functional>

class PdfSession;

// The marks in a PDF's notes (backend.pdfMarks, Cycle 163). The items come
// from parsing the document's current text, saved or not, so undo in the
// editor takes a mark off the page; each is placed on the PDF by its quote
// (PdfNotes::place). Marking, removing and recolouring compute the notes
// model's minimal edit on the current text and hand it to the owner, which
// applies it as one undoable edit through the editor's own path.
class PdfMarks : public QObject {
    Q_OBJECT
    // [{index, kind, colour, page (0-based), start, length, placed, rects (one
    // per text line, page view points), direction (PdfSession::textDirection),
    // vertical, quote, marker}] in document order. Empty outside PDF mode.
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    // The highlighter's tool (app settings): "highlight", "underline" or "strike".
    Q_PROPERTY(QString tool READ tool WRITE setTool NOTIFY toolChanged)
    // The colour the highlighter uses (app settings).
    Q_PROPERTY(QString highlightColour READ highlightColour WRITE setHighlightColour NOTIFY toolChanged)
    Q_PROPERTY(QStringList highlightColours READ highlightColours CONSTANT)
public:
    using TextSource = std::function<QString()>;
    using EditSink = std::function<bool(const PdfNotes::Edit &)>;
    PdfMarks(PdfSession *session, TextSource text, EditSink apply, QObject *parent = nullptr);

    QVariantList items() const { return m_items; }
    QString tool() const;
    void setTool(const QString &tool);
    QString highlightColour() const;
    void setHighlightColour(const QString &colour);
    QStringList highlightColours() const { return PdfNotes::highlightColours(); }

    // Marks the page's text [start, start + length) (page 0-based). A mark of
    // the same kind already over exactly that text is removed instead, as
    // Preview's highlighter does. Returns "added", "removed" or "" (nothing done).
    Q_INVOKABLE QString mark(const QString &kind, const QString &colour, int page, int start, int length);
    // `index` is an items entry's index.
    Q_INVOKABLE bool removeMark(int index);
    Q_INVOKABLE bool recolourMark(int index, const QString &colour);
    // Re-reads the notes now (otherwise shortly after the text changes).
    Q_INVOKABLE void refresh();
    void scheduleRefresh();

    static constexpr int RefreshDelay = 30;

signals:
    void itemsChanged();
    void toolChanged();

private:
    struct Current {
        QString text;
        PdfNotes::Document document;
        QList<PdfNotes::Placement> placements;
    };
    Current current() const;
    int currentIndex(const Current &now, int index) const;
    QString pageText(int page) const; // 1-based; empty when there is no such page
    QVariantList rects(int page, int start, int length) const;
    bool ready() const;

    PdfSession *m_session;
    TextSource m_text;
    EditSink m_apply;
    QVariantList m_items;
    QTimer m_refreshTimer;
    mutable QHash<QString, QVariantList> m_rects;
};
