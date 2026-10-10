#pragma once

#include <QObject>
#include <QString>

// The QWebChannel object ("bridge") between the app and the CodeMirror editor
// page (src/editor). Direction of each member:
//   public slots  — called BY the page over the channel;
//   JS-facing signals (setDocument … redo) — connected TO by the page;
//   app-facing signals (changesReceived …) — for Backend/QML.
// The page may not be connected yet when the app pushes state, so the latest
// document, mode, theme and appearance are kept and replayed on ready().
class EditorBridge : public QObject {
    Q_OBJECT
    // Named `connected`, not `ready`: QWebChannel defines JS properties after
    // methods, so a property called `ready` would shadow the ready() slot.
    Q_PROPERTY(bool connected READ isReady NOTIFY readyChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
public:
    explicit EditorBridge(QObject *parent = nullptr);

    bool isReady() const { return m_ready; }
    int revision() const { return m_revision; }

    // App → page.
    Q_INVOKABLE void loadDocument(const QString &text);
    Q_INVOKABLE void selectMode(const QString &mode);
    Q_INVOKABLE void applyTheme(const QString &css);
    Q_INVOKABLE void applyAppearance(const QString &json);
    Q_INVOKABLE void focus();
    Q_INVOKABLE int fetchText();
    // Ask the page for its selection as it is right now (the debounced
    // cursorChanged report can lag 30 ms). Returns the token; the answer comes
    // back through selectionReply() and the selectionReceived signal. Nothing
    // is sent (and no answer will come) while the page is not ready.
    Q_INVOKABLE int fetchSelection();
    // Forget an outstanding selection request (the host gave up waiting); a
    // late reply then no longer updates the last-known selection.
    Q_INVOKABLE void cancelSelectionRequest(int token);
    // The "check spelling while typing" setting for the page. Every call makes
    // the page check again (it clears nothing first), so the host also calls it
    // when the language or the learned/ignored words change. Remembered and
    // replayed on ready().
    Q_INVOKABLE void applySpellCheck(bool enabled);
    // Test support: edits that the page applies as ordinary user input.
    Q_INVOKABLE void injectUserChanges(const QString &changesJson);
    Q_INVOKABLE void requestUndo();
    Q_INVOKABLE void requestRedo();
    // Host-invoked editor commands: "find", "replace", "selectAll".
    Q_INVOKABLE void runCommand(const QString &name);
    // Ask the page to scroll to a 0..1 fraction (pane sync); not echoed back.
    Q_INVOKABLE void scrollTo(double fraction);
    // Host-side document changes the page must apply (not reported back).
    Q_INVOKABLE void pushChanges(const QString &changesJson);
    // Place the page's caret (UTF-16 offset); the page scrolls it into view.
    Q_INVOKABLE void placeCursor(int position);
    // The page's last reported caret (selection head), -1 before any report.
    int lastCursor() const { return m_lastCursor; }
    // True once the writer moved the page caret since the last host placement.
    Q_INVOKABLE bool caretMovedByUser() const { return m_caretMovedByUser; }
    // Outline jumps and the like: a host placement the writer asked for.
    Q_INVOKABLE void noteCaretMovedByUser() { m_caretMovedByUser = true; }
    int lastSelectionStart() const { return m_lastCursor < 0 ? -1 : qMin(m_lastAnchor, m_lastCursor); }
    int lastSelectionEnd() const { return m_lastCursor < 0 ? -1 : qMax(m_lastAnchor, m_lastCursor); }

public slots:
    // Page → app (over QWebChannel).
    void ready();
    void documentChanged(const QString &changesJson, int revision);
    void cursorChanged(int anchor, int head, bool byUser = true);
    void metric(const QString &name, double ms);
    void log(const QString &message);
    void textReply(int token, const QString &text);
    // Answer to requestSelection: the page's selection at the moment it was
    // asked. Also refreshes lastCursor()/lastSelection*() without marking the
    // caret as moved by the writer.
    void selectionReply(int token, int anchor, int head);
    // The page cannot read local files; it asks the host for a data: URL.
    void requestImage(int token, const QString &src);
    // Called by the host with the answer (emits imageReply to the page).
    void replyImage(int token, const QString &dataUrl, const QString &error);
    // Pasted/dropped image bytes the page wants saved next to the document.
    void saveImage(int token, const QString &name, const QString &mime, const QString &base64);
    void replyImageSaved(int token, const QString &relativePath, const QString &error);
    // The page reports its vertical position as a 0..1 fraction (debounced).
    void scrolled(double fraction);
    // Spelling. The page sends the prose segments it wants checked (a JSON
    // array of {from, to, text}, document offsets) under a token of its own;
    // the host answers through replySpelling with the same token. Suggestions
    // work the same way. Learn/ignore carry no answer: the host re-checks.
    void checkSpelling(int token, const QString &segmentsJson);
    void replySpelling(int token, const QString &rangesJson);
    void spellingSuggestions(int token, const QString &word);
    void replySuggestions(int token, const QString &wordsJson);
    void learnWord(const QString &word);
    void ignoreWord(const QString &word);
    void ignoreGrammar(const QString &text, const QString &message);
    // Harper (the page's grammar engine; the host decides what is prose and
    // brokers the findings). The page answers harperLint(token, text) with
    // harperReply(token, lintsJson) or harperFailed(token, error) (token -1:
    // the engine failed to load); harperReady(version) says a load finished.
    void harperReady(const QString &version);
    void harperReply(int token, const QString &lintsJson);
    void harperFailed(int token, const QString &error);

signals:
    // Connected to by the page.
    void setDocument(const QString &text, int revision);
    void applyChanges(const QString &changesJson, int revision);
    void setMode(const QString &mode);
    void setTheme(const QString &css);
    void setAppearance(const QString &json);
    void focusEditor();
    void requestText(int token);
    void requestSelection(int token);
    void simulateUserChanges(const QString &changesJson);
    void undo();
    void redo();
    void imageReply(int token, const QString &dataUrl, const QString &error);
    void imageSaved(int token, const QString &relativePath, const QString &error);
    void scrollToFraction(double fraction);
    void setCursor(int position);
    void command(const QString &name);
    void spellingReply(int token, const QString &rangesJson);
    void suggestionsReply(int token, const QString &wordsJson);
    void setSpellCheck(bool enabled);
    void harperLoad(const QString &dialect);
    void harperLint(int token, const QString &text);
    void harperImportWords(const QString &wordsJson);
    // For the app.
    void spellingRequested(int token, const QString &segmentsJson);
    void suggestionsRequested(int token, const QString &word);
    void wordLearned(const QString &word);
    void wordIgnored(const QString &word);
    void grammarIgnored(const QString &text, const QString &message);
    void harperReadied(const QString &version);
    void harperReplied(int token, const QString &lintsJson);
    void harperLintFailed(int token, const QString &error);
    void scrollFractionChanged(double fraction);
    void imageRequested(int token, const QString &src);
    void imageSaveRequested(int token, const QString &name, const QString &mime, const QString &base64);
    void readyChanged();
    void revisionChanged();
    void changesReceived(const QString &changesJson, int revision);
    void cursorMoved(int anchor, int head);
    void metricRecorded(const QString &name, double ms);
    void textReceived(int token, const QString &text);
    void selectionReceived(int token, int anchor, int head);
    void messageLogged(const QString &message);

private:
    bool m_ready = false;
    int m_revision = 0;
    int m_nextToken = 0;
    int m_lastCursor = -1;
    bool m_caretMovedByUser = false;
    int m_lastAnchor = -1;
    int m_selectionRequestInFlight = 0;
    bool m_hasDocument = false;
    bool m_spellCheck = true;
    QString m_text;
    QString m_mode = QStringLiteral("live");
    QString m_theme;
    QString m_appearance;
};
