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
    int lastSelectionStart() const { return m_lastCursor < 0 ? -1 : qMin(m_lastAnchor, m_lastCursor); }
    int lastSelectionEnd() const { return m_lastCursor < 0 ? -1 : qMax(m_lastAnchor, m_lastCursor); }

public slots:
    // Page → app (over QWebChannel).
    void ready();
    void documentChanged(const QString &changesJson, int revision);
    void cursorChanged(int anchor, int head);
    void metric(const QString &name, double ms);
    void log(const QString &message);
    void textReply(int token, const QString &text);
    // The page cannot read local files; it asks the host for a data: URL.
    void requestImage(int token, const QString &src);
    // Called by the host with the answer (emits imageReply to the page).
    void replyImage(int token, const QString &dataUrl, const QString &error);
    // Pasted/dropped image bytes the page wants saved next to the document.
    void saveImage(int token, const QString &name, const QString &mime, const QString &base64);
    void replyImageSaved(int token, const QString &relativePath, const QString &error);
    // The page reports its vertical position as a 0..1 fraction (debounced).
    void scrolled(double fraction);

signals:
    // Connected to by the page.
    void setDocument(const QString &text, int revision);
    void applyChanges(const QString &changesJson, int revision);
    void setMode(const QString &mode);
    void setTheme(const QString &css);
    void setAppearance(const QString &json);
    void focusEditor();
    void requestText(int token);
    void simulateUserChanges(const QString &changesJson);
    void undo();
    void redo();
    void imageReply(int token, const QString &dataUrl, const QString &error);
    void imageSaved(int token, const QString &relativePath, const QString &error);
    void scrollToFraction(double fraction);
    void setCursor(int position);
    void command(const QString &name);
    // For the app.
    void scrollFractionChanged(double fraction);
    void imageRequested(int token, const QString &src);
    void imageSaveRequested(int token, const QString &name, const QString &mime, const QString &base64);
    void readyChanged();
    void revisionChanged();
    void changesReceived(const QString &changesJson, int revision);
    void cursorMoved(int anchor, int head);
    void metricRecorded(const QString &name, double ms);
    void textReceived(int token, const QString &text);
    void messageLogged(const QString &message);

private:
    bool m_ready = false;
    int m_revision = 0;
    int m_nextToken = 0;
    int m_lastCursor = -1;
    int m_lastAnchor = -1;
    bool m_hasDocument = false;
    QString m_text;
    QString m_mode = QStringLiteral("live");
    QString m_theme;
    QString m_appearance;
};
