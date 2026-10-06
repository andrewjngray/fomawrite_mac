#include "editorbridge.h"

EditorBridge::EditorBridge(QObject *parent) : QObject(parent) {}

void EditorBridge::loadDocument(const QString &text) {
    m_text = text;
    m_hasDocument = true;
    ++m_revision;
    emit revisionChanged();
    if (m_ready) emit setDocument(m_text, m_revision);
}

void EditorBridge::selectMode(const QString &mode) {
    m_mode = mode;
    if (m_ready) emit setMode(m_mode);
}

void EditorBridge::applyTheme(const QString &css) {
    m_theme = css;
    if (m_ready) emit setTheme(m_theme);
}

void EditorBridge::applyAppearance(const QString &json) {
    m_appearance = json;
    if (m_ready) emit setAppearance(m_appearance);
}

void EditorBridge::focus() {
    if (m_ready) emit focusEditor();
}

int EditorBridge::fetchText() {
    const int token = ++m_nextToken;
    if (m_ready) emit requestText(token);
    return token;
}

void EditorBridge::injectUserChanges(const QString &changesJson) {
    if (m_ready) emit simulateUserChanges(changesJson);
}

void EditorBridge::requestUndo() { if (m_ready) emit undo(); }
void EditorBridge::requestRedo() { if (m_ready) emit redo(); }
void EditorBridge::scrollTo(double fraction) { if (m_ready) emit scrollToFraction(fraction); }
void EditorBridge::pushChanges(const QString &changesJson) {
    if (!m_ready || !m_hasDocument) return;
    ++m_revision;
    emit revisionChanged();
    emit applyChanges(changesJson, m_revision);
}
void EditorBridge::scrolled(double fraction) { emit scrollFractionChanged(fraction); }

void EditorBridge::ready() {
    m_ready = true;
    // Replay what the app pushed while the page was still loading.
    if (!m_appearance.isEmpty()) emit setAppearance(m_appearance);
    emit setTheme(m_theme);
    emit setMode(m_mode);
    if (m_hasDocument) emit setDocument(m_text, m_revision);
    emit readyChanged();
}

void EditorBridge::documentChanged(const QString &changesJson, int revision) {
    m_revision = revision;
    emit revisionChanged();
    emit changesReceived(changesJson, revision);
}

void EditorBridge::cursorChanged(int anchor, int head) { m_lastCursor = head; emit cursorMoved(anchor, head); }
void EditorBridge::placeCursor(int position) { if (m_ready && m_hasDocument) emit setCursor(position); }
void EditorBridge::metric(const QString &name, double ms) { emit metricRecorded(name, ms); }
void EditorBridge::log(const QString &message) { emit messageLogged(message); }
void EditorBridge::textReply(int token, const QString &text) { emit textReceived(token, text); }
void EditorBridge::requestImage(int token, const QString &src) { emit imageRequested(token, src); }
void EditorBridge::replyImage(int token, const QString &dataUrl, const QString &error) { emit imageReply(token, dataUrl, error); }
