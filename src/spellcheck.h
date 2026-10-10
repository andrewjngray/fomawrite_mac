#pragma once

// The one spell checker both writing surfaces share.
//
// Source (the Qt Quick text item) and Live (the CodeMirror page) each decide
// which stretches of a document are prose; this object decides which words in
// a stretch are misspelled, offers suggestions, and remembers learned and
// ignored words. On macOS it is the system checker, so the dictionary, the
// language list and the learned words are the ones every other Mac app uses.
// Elsewhere it is unavailable and reports nothing.
//
// Offsets are UTF-16 code units into the string given, the same units QString
// and the page use, so a range can be mapped back without conversion.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class SpellCheck : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit SpellCheck(QObject *parent = nullptr);

    struct Range { int start; int end; };

    bool available() const;
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    // Empty means the system language; otherwise a tag from languages().
    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    Q_INVOKABLE QStringList languages() const;

    // Misspelled words in `text`, in order, as [start, end) ranges. Empty when
    // disabled or unavailable. Text is checked as given: callers blank or omit
    // code, URLs and markup first.
    QList<Range> misspellings(const QString &text) const;
    Q_INVOKABLE QVariantList misspelledRanges(const QString &text) const;
    // Up to eight replacements for one word, best first.
    Q_INVOKABLE QStringList suggestions(const QString &word) const;
    // Learned words persist in the system dictionary; ignored words last for
    // this run. Either emits wordsChanged so surfaces re-check.
    Q_INVOKABLE void learnWord(const QString &word);
    Q_INVOKABLE void unlearnWord(const QString &word);
    Q_INVOKABLE void ignoreWord(const QString &word);
    Q_INVOKABLE bool hasLearnedWord(const QString &word) const;

signals:
    void enabledChanged();
    void languageChanged();
    void wordsChanged();

private:
    bool m_enabled = true;
    QString m_language;
};
