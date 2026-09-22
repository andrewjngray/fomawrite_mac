#pragma once

#include <QObject>
#include <QPointer>
#include <QByteArray>
#include <QFileSystemWatcher>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QPageLayout>
#include <QJsonObject>
#include <memory>
#include <functional>
#include "filelibrary.h"

class MarkdownHighlighter;
class QTextDocument;
class QWindow;
class QLockFile;

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *library READ library CONSTANT)
    Q_PROPERTY(QUrl documentBaseUrl READ documentBaseUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileUrlChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY wordCountChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)
    Q_PROPERTY(QString themeBackground READ themeBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeForeground READ themeForeground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeAccent READ themeAccent NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeSelection READ themeSelection NOTIFY themeColorsChanged)

    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override;

    QObject *library() { return &m_library; }
    QUrl documentBaseUrl() const;

    void setParentWindow(QWindow *window);
    std::function<bool(const QUrl &)> focusExistingDocument;
    Q_INVOKABLE void notifyWindowClosed() { emit windowClosed(); }
    Q_INVOKABLE void requestQuit() { emit quitRequested(); }
    Q_INVOKABLE void cancelQuit() { emit quitCanceled(); }

    QUrl fileUrl() const { return m_fileUrl; }
    QString fileName() const;

    bool modified() const { return m_modified; }
    QString status() const { return m_status; }
    int wordCount() const { return m_wordCount; }
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool darkMode);
    qreal textScale() const { return m_textScale; }
    void setTextScale(qreal textScale);
    QString themeBackground() const { return m_themeBackground; }
    QString themeForeground() const { return m_themeForeground; }
    QString themeAccent() const { return m_themeAccent; }
    QString themeSelection() const { return m_themeSelection; }
    static int countWords(const QString &text);
    static QString normalizedLinkUrl(const QString &clipboardText);
    static QString suggestedFileName(const QString &text);

    Q_INVOKABLE QVariantList documentOutline(const QString &markdown) const;
    Q_INVOKABLE QVariantMap documentStatistics(const QString &markdown) const;
    Q_INVOKABLE QString previewMarkdown(const QString &source) const;
    Q_INVOKABLE int previewAnchorPosition(QObject *textDocument, const QString &anchor) const;
    Q_INVOKABLE QString tableOfContents(const QString &markdown) const;
    Q_INVOKABLE void stylePreview(QObject *textDocument);
    Q_INVOKABLE QVariantMap wrapSelection(int start, int end, const QString &before, const QString &after);
    Q_INVOKABLE QVariantMap replaceText(int start, int end, const QString &replacement);
    Q_INVOKABLE QVariantMap editMarkdown(const QString &action, int start, int end);
    Q_INVOKABLE QVariantList searchPositions(const QString &query) const;
    Q_INVOKABLE int replaceMatches(const QString &query, const QString &replacement, int position);
    Q_INVOKABLE void setFocusPosition(int position, bool enabled, bool sentence = false);
    static QPair<int, int> sentenceRange(const QString &text, int position);
    Q_INVOKABLE void setShowMarkup(bool show);
    Q_INVOKABLE QUrl resolveDocumentLink(const QString &link) const;
    Q_INVOKABLE void attachDocument(QObject *textDocument);
    Q_INVOKABLE void openDialog();
    Q_INVOKABLE bool open(const QUrl &url);
    Q_INVOKABLE void rememberCursor(int position);
    Q_INVOKABLE int navigateHistory(int direction);
    Q_INVOKABLE QUrl sourceLinkAt(int position) const;
    bool canGoBack() const { return m_historyIndex > 0; }
    bool canGoForward() const { return m_historyIndex + 1 < m_history.size(); }
    Q_INVOKABLE void save();
    Q_INVOKABLE void saveForClose();
    Q_INVOKABLE void saveAsDialog();
    Q_INVOKABLE void saveAs(const QUrl &url);
    Q_INVOKABLE void fileDialogCanceled();
    Q_INVOKABLE void discardRecovery();
    Q_INVOKABLE void reloadFromDisk();
    Q_INVOKABLE void keepExternalVersion();
    Q_INVOKABLE void printDocument(bool plain = false);
    Q_INVOKABLE void pageSetup();
    Q_INVOKABLE bool exportDocument(const QUrl &destination, const QString &format);
    Q_INVOKABLE void setOutputStyle(int style);
    Q_INVOKABLE bool loadOutputStyle(const QUrl &file);
    Q_INVOKABLE void newWindow();
    Q_INVOKABLE void markAuthorship(int start, int end, const QString &category, const QString &author);
    Q_INVOKABLE QVariantList authorshipRanges() const;
    Q_INVOKABLE bool createVersion();
    Q_INVOKABLE QVariantList versions() const;
    Q_INVOKABLE bool restoreVersion(const QUrl &version);
    Q_INVOKABLE void autosave();
    Q_INVOKABLE int nativeTabInset() const;
    Q_INVOKABLE void nativeWindowAction(const QString &action);
    Q_INVOKABLE QVariantList writingAnalysis(const QString &text, const QString &customWords);
    Q_INVOKABLE QStringList spellingIssues(const QString &text);
    Q_INVOKABLE void newDocument();
    Q_INVOKABLE bool duplicateDocument(const QString &name);
    Q_INVOKABLE bool renameDocument(const QString &name);
    Q_INVOKABLE bool moveDocument(const QUrl &folder);
    Q_INVOKABLE bool openInNewWindow(const QUrl &url);
    Q_INVOKABLE bool showInFinder();
    Q_INVOKABLE bool copySelection(int start, int end, const QString &format);
    Q_INVOKABLE QString clipboardMarkdown() const;
    Q_INVOKABLE QString clipboardUrl() const;
    Q_INVOKABLE QString clipboardText() const;
    Q_INVOKABLE bool editorTextChanged();
    Q_INVOKABLE QVariantList hiddenRangesAt(int position) const;
    Q_INVOKABLE void setSearchHighlight(const QString &query, int currentMatchStart);
    Q_INVOKABLE void openExternalUrl(const QUrl &url);
    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void newWindowRequested(const QUrl &url);
    void quitRequested();
    void quitCanceled();
    void windowClosed();
    void saveFailed();
    void historyChanged();
    void documentLoaded();
    void fileUrlChanged();
    void modifiedChanged();
    void statusChanged();
    void wordCountChanged();
    void darkModeChanged();
    void textScaleChanged();
    void themeColorsChanged();
    void closeAfterSave();
    void openDialogRequested();
    void saveDialogRequested(const QUrl &suggestedUrl);
    void saveSucceeded();
    void externalChangeDetected(bool deleted, bool locallyModified);

private:
    QList<QPair<QUrl, int>> m_history;
    int m_historyIndex = -1;
    bool m_navigatingHistory = false;
    void loadDocumentText(const QString &text);
    void setFileUrl(const QUrl &url);
    void setModified(bool modified);
    void setStatus(const QString &status);
    QJsonObject authorshipData() const;
    void applyAuthorshipData(const QJsonObject &data);
    bool saveAuthorship(const QUrl &url);
    void loadAuthorship(const QUrl &url);
    void prepareOutput(QTextDocument &document, bool plain = false) const;
    int m_outputStyle = 0;
    QString m_customOutputFont;
    int m_customOutputSize = 12;
    QPageLayout m_pageLayout;
    void saveTo(const QUrl &url);
    QUrl suggestedSaveUrl() const;
    QString currentDocumentText() const;
    void setWordCount(int words);
    void refreshWordCount();
    void scheduleWordCount();
    void applyDocumentTypography();
    void reapplyTypographyToChange();
    void scheduleRecovery();
    void writeRecovery();
    void restoreRecovery();
    void clearRecovery();
    QString recoveryPath() const;
    void watchCurrentFile();
    void loadOmarchyTheme();
    void watchOmarchyTheme();

    bool m_showMarkup = false;
    FileLibrary m_library;
    QUrl m_fileUrl;
    bool m_modified = false;
    QString m_status;
    int m_wordCount = 0;
    bool m_darkMode = true;
    qreal m_textScale = 1.0;
    bool m_loading = false;
    bool m_closeAfterSave = false;
    bool m_formattingTypography = false;
    int m_formattedBlockCount = 0;
    int m_lastChangePos = 0;
    int m_lastChangeAdded = 0;
    QTimer m_wordCountTimer;
    QTimer m_recoveryTimer;
    QFileSystemWatcher m_fileWatcher;
    QPointer<QTextDocument> m_document;
    QPointer<QWindow> m_parentWindow;
    QPointer<MarkdownHighlighter> m_highlighter;
    QString m_lastDocumentText;
    QByteArray m_lastKnownFileContents;
    bool m_hasKnownFileContents = false;
    QString m_recoveryPath;
    std::unique_ptr<QLockFile> m_recoveryLock;

    QString m_themeBackground;
    QString m_themeForeground;
    QString m_themeAccent;
    QString m_themeSelection;
    QFileSystemWatcher m_themeWatcher;
};
