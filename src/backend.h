#pragma once

#include <QObject>
#include <QSet>
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
#include <deque>
#include <functional>
#include <optional>
#include "filelibrary.h"
#include "publisher.h"
#include "editorbridge.h"
#include "spellcheck.h"

class MarkdownHighlighter;
class QTextDocument;
class QWindow;
class QPagedPaintDevice;
class QLockFile;
class QTemporaryDir;
class QIODevice;

class Backend : public QObject, public PublishingSource {
    Q_OBJECT
    Q_PROPERTY(QString applicationPath READ applicationPath CONSTANT)
    Q_PROPERTY(int outputStyle READ outputStyle NOTIFY outputStyleChanged)
    Q_PROPERTY(QString outputFont READ outputFont NOTIFY outputStyleChanged)
    Q_PROPERTY(int outputPointSize READ outputPointSize NOTIFY outputStyleChanged)
    Q_PROPERTY(QString outputTemplateName READ outputTemplateName NOTIFY outputStyleChanged)
    Q_PROPERTY(QString publishingThemeId READ publishingThemeId NOTIFY publishingThemesChanged)
    Q_PROPERTY(QString publishingThemeName READ publishingThemeName NOTIFY publishingThemesChanged)
    // The selected theme's resolved `--bg-color` (for example "#1e222a"), or
    // empty when it defines none. Lets the app chrome follow a dark theme.
    Q_PROPERTY(QString themeBackgroundColor READ themeBackgroundColor NOTIFY publishingCssChanged)
    Q_PROPERTY(QVariantList publishingThemes READ publishingThemes NOTIFY publishingThemesChanged)
    Q_PROPERTY(QString publishingThemeError READ publishingThemeError NOTIFY publishingThemesChanged)
    Q_PROPERTY(QObject *library READ library CONSTANT)
    Q_PROPERTY(QObject *publisher READ publisher CONSTANT)
    Q_PROPERTY(QObject *editorBridge READ editorBridge CONSTANT)
    Q_PROPERTY(QUrl documentBaseUrl READ documentBaseUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString publishingDocumentIdentity READ publishingDocumentIdentity NOTIFY publishingDocumentIdentityChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileUrlChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString themePreset READ themePreset WRITE setThemePreset NOTIFY themePresetChanged)
    Q_PROPERTY(QVariantMap palette READ palette NOTIFY themeColorsChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    // The app's own chrome takes the selected Output Style's page colours (Typora's
    // --bg-color / --text-color) when the theme defines them, so a dark theme darkens
    // the whole window as it does in Typora. Off: the Theme preset alone decides.
    Q_PROPERTY(bool appearanceFollowsOutputStyle READ appearanceFollowsOutputStyle WRITE setAppearanceFollowsOutputStyle NOTIFY appearanceFollowsOutputStyleChanged)
    Q_PROPERTY(SpellCheck *spellCheck READ spellCheck CONSTANT)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)
    Q_PROPERTY(QString themeBackground READ themeBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeForeground READ themeForeground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeAccent READ themeAccent NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeSelection READ themeSelection NOTIFY themeColorsChanged)

    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
public:
    explicit Backend(QObject *parent = nullptr, bool outputOnly = false);
    ~Backend() override;

    QString applicationPath() const;
    QObject *library() { return &m_library; }
    QObject *publisher() { return m_publisher.get(); }
    EditorBridge *editorBridge() { return m_editorBridge.get(); }
    SpellCheck *spellCheck() { return &m_spellCheck; }
    // Push the current document, theme and appearance to the Live editor page.
    Q_INVOKABLE void syncLiveEditor(int cursor = -1);
    // Re-style the Live editor after a theme change without reloading the document.
    Q_INVOKABLE void pushLiveTheme();
    // The Live editor's last caret position, for carrying it back to Source.
    Q_INVOKABLE int liveCursor() const { return m_editorBridge->lastCursor(); }
    // Format commands while the Live editor has focus: the existing edit
    // helpers run on the canonical document at the page's selection and the
    // host->page mirror delivers the result; the caret is then placed. While
    // the page is ready each command first asks the page for its selection as
    // it is right now (requestSelection/selectionReply), because the debounced
    // cursorChanged report can lag a selection change by 30 ms, and applies
    // the edit in the reply; the return value then means "accepted" (the
    // command is queued, one at a time, in call order). Without a ready page
    // the edit runs at once on the last reported selection and the return
    // value says whether it changed anything.
    Q_INVOKABLE bool liveWrapSelection(const QString &before, const QString &after);
    Q_INVOKABLE bool liveReplaceSelection(const QString &replacement);
    Q_INVOKABLE bool liveEditMarkdown(const QString &action);
    // Copy the Live page's selection in a clipboard format ("markdown", "html", "formatted").
    Q_INVOKABLE bool liveCopySelection(const QString &format);
    QUrl documentBaseUrl() const override;

    void setParentWindow(QWindow *window);
    std::function<bool(const QUrl &)> focusExistingDocument;
    int documentRevision() const;
    Q_INVOKABLE void notifyQuitReady() { emit quitReady(); }
    Q_INVOKABLE void notifyWindowClosed() { emit windowClosed(); }
    Q_INVOKABLE void requestQuit() { emit quitRequested(); }
    Q_INVOKABLE void cancelQuit() { emit quitCanceled(); }

    QUrl fileUrl() const { return m_fileUrl; }
    QString fileName() const;

    bool modified() const { return m_modified; }
    QString status() const { return m_status; }
    QString themePreset() const { return m_themePreset; }
    void setThemePreset(const QString &preset);
    QVariantMap palette() const;
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool darkMode);
    bool appearanceFollowsOutputStyle() const { return m_appearanceFollowsOutputStyle; }
    void setAppearanceFollowsOutputStyle(bool follow);
    // True while the selected Output Style's page colour is driving the chrome.
    Q_INVOKABLE bool appearanceOverridden() const { return m_appearanceOverridden; }
    qreal textScale() const { return m_textScale; }
    void setTextScale(qreal textScale);
    QString themeBackground() const { return m_themeBackground; }
    QString themeForeground() const { return m_themeForeground; }
    QString themeAccent() const { return m_themeAccent; }
    QString themeSelection() const { return m_themeSelection; }
    static int countWords(const QString &text);
    static QString normalizedLinkUrl(const QString &clipboardText);
    static QString suggestedFileName(const QString &text);
    Q_INVOKABLE QVariantMap resolveOpenPath(const QString &input) const;

    Q_INVOKABLE QVariantList documentOutline(const QString &markdown) const;
    Q_INVOKABLE QVariantMap documentStatistics(const QString &markdown) const;
    Q_INVOKABLE QVariantMap wordCompletions(int position) const;
    Q_INVOKABLE QString smartQuoteAt(int position) const;
    Q_INVOKABLE bool smartDashAt(int position) const;
    Q_INVOKABLE QString bundledHelp(const QString &page) const;
    Q_INVOKABLE QString previewMarkdown(const QString &source) const;
    Q_INVOKABLE int previewAnchorPosition(QObject *textDocument, const QString &anchor) const;
    Q_INVOKABLE int markdownAnchorPosition(const QString &markdown, const QString &anchor) const;
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
    Q_INVOKABLE bool refreshSourceTypography();
    Q_INVOKABLE void setSourceAppearance(const QString &appearance);
    Q_INVOKABLE QVariantList sourceLineDecorations(qreal top, qreal bottom) const;
    Q_INVOKABLE QVariantMap editCode(const QString &action, int start, int end);
    Q_INVOKABLE int undoSource(int position);
    Q_INVOKABLE int redoSource(int position);
    Q_INVOKABLE void openDialog();
    Q_INVOKABLE bool open(const QUrl &url);
    Q_INVOKABLE void rememberCursor(int position);
    Q_INVOKABLE int navigateHistory(int direction);
    Q_INVOKABLE QUrl sourceLinkAt(int position) const;
    bool canGoBack() const { return m_historyIndex > 0; }
    bool canGoForward() const { return m_historyIndex + 1 < m_history.size(); }
    Q_INVOKABLE void save();
    Q_INVOKABLE void saveAsDialog();
    Q_INVOKABLE void saveAs(const QUrl &url);
    Q_INVOKABLE void fileDialogCanceled();
    Q_INVOKABLE void discardRecovery();
    Q_INVOKABLE void reloadFromDisk();
    Q_INVOKABLE void keepExternalVersion();
    Q_INVOKABLE void printDocument(bool plain = false);
    Q_INVOKABLE void pageSetup();
    Q_INVOKABLE void printPreview();
    Q_INVOKABLE bool exportDocument(const QUrl &destination, const QString &format);
    // Private temporary publishing output; never saves or changes the source.
    Q_INVOKABLE QVariantMap publishingPreview(const QString &format);
    Q_INVOKABLE QVariantMap requestPublishingPreview(const QString &format, QObject *consumer, const QString &requestIdentity = QString());
    Q_INVOKABLE void cancelPublishingPreview(QObject *consumer);
    QString publishingDocumentIdentity() const { return m_publisher->documentIdentity(); }
    QString publishingThemeId() const { return m_publisher->themeId(); }
    QString publishingThemeName() const;
    QString themeBackgroundColor() const;
    // The Live page's theme text: the Typora base shim, then the current theme CSS.
    QString liveThemeCss() const;
    QVariantList publishingThemes() const;
    QString publishingThemeError() const { return m_publisher->themeError(); }
    Q_INVOKABLE bool selectPublishingTheme(const QString &id);
    Q_INVOKABLE bool importPublishingTheme(const QUrl &file);
    Q_INVOKABLE void reloadPublishingThemes();
    Q_INVOKABLE bool openPublishingThemesFolder();
    int outputStyle() const { return m_outputStyle; }
    QString outputFont() const;
    int outputPointSize() const;
    QString outputTemplateName() const override;
    // Stable, lowercase identifiers for the export hub. Invalid requests leave
    // the active layout unchanged, so QML can safely pass untrusted selections.
    Q_INVOKABLE QString exportPaperSize() const;
    Q_INVOKABLE bool setExportPaperSize(const QString &paperSize);
    Q_INVOKABLE QString exportOrientation() const;
    Q_INVOKABLE bool setExportOrientation(const QString &orientation);
    // Maps contain id, name, and font. Custom is deliberately excluded because
    // it is loaded from a user file rather than bundled with Fomawrite.
    Q_INVOKABLE QVariantList builtInOutputStyles() const;
    // Optional additional CSS applies to Web and PDF publishing output.
    Q_INVOKABLE QString outputCssName() const;
    Q_INVOKABLE bool loadOutputCss(const QUrl &file);
    Q_INVOKABLE void clearOutputCss();
    // Local, Fomawrite-owned export styles. Selecting one copies it into the
    // existing Custom output slot (ID 3); it never changes document source.
    Q_INVOKABLE QVariantList userOutputStyles() const;
    Q_INVOKABLE QString selectedUserOutputStyleId() const;
    Q_INVOKABLE QVariantMap createUserOutputStyleFromCurrent(const QString &name);
    Q_INVOKABLE bool updateUserOutputStyle(const QString &id, const QVariantMap &changes);
    Q_INVOKABLE bool selectUserOutputStyle(const QString &id);
    Q_INVOKABLE bool deleteUserOutputStyle(const QString &id);
    Q_INVOKABLE void setOutputStyle(int style);
    Q_INVOKABLE bool loadOutputStyle(const QUrl &file);
    Q_INVOKABLE void newWindow();
    Q_INVOKABLE void markAuthorship(int start, int end, const QString &category, const QString &author);
    Q_INVOKABLE QVariantList authorshipRanges() const;
    Q_INVOKABLE bool createVersion();
    Q_INVOKABLE void setAutomaticVersions(bool enabled);
    Q_INVOKABLE QVariantList versions() const;
    Q_INVOKABLE bool restoreVersion(const QUrl &version);
    Q_INVOKABLE void autosave();
    Q_INVOKABLE int nativeTabInset() const;
    Q_INVOKABLE void nativeWindowAction(const QString &action);
    Q_INVOKABLE QVariantList writingAnalysis(const QString &text, const QString &customWords);
    // Comma-separated current-document terms: first 32 unique entries, each at
    // most 64 UTF-16 units; returns up to 1000 ordered, non-overlapping spans.
    Q_INVOKABLE QVariantList customReviewSpans(const QString &customWords) const;
    Q_INVOKABLE QVariantList styleReviewSpans(const QString &customWords,
                                               bool customEnabled,
                                               bool fillersEnabled) const;
    Q_INVOKABLE void setStyleReviewWords(const QString &customWords,
                                         bool customEnabled,
                                         bool fillersEnabled);
    Q_INVOKABLE QStringList writingLanguages() const;
    Q_INVOKABLE QVariantList writingIssues(const QString &text, const QString &language, bool grammar);
    Q_INVOKABLE bool correctWriting(int start, int end, const QString &expected, const QString &replacement);
    Q_INVOKABLE void speakText(const QString &text);
    Q_INVOKABLE void stopSpeaking();
    static QString proseForReview(const QString &markdown);
    Q_INVOKABLE void newDocument();
    Q_INVOKABLE bool duplicateDocument(const QString &name);
    Q_INVOKABLE bool renameDocument(const QString &name);
    Q_INVOKABLE bool moveDocument(const QUrl &folder);
    Q_INVOKABLE QVariantMap libraryItemInfo(const QUrl &url) const;
    Q_INVOKABLE bool libraryItemAction(const QUrl &url, const QString &action, const QString &argument = QString());
    Q_INVOKABLE bool openInNewTab(const QUrl &url);
    Q_INVOKABLE bool openInNewWindow(const QUrl &url);
    Q_INVOKABLE bool showInFinder();
    Q_INVOKABLE int pasteWithAuthorship(int start, int end);
    Q_INVOKABLE bool exportAuthorship(const QUrl &destination);
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
    void newTabRequested(const QUrl &url);
    void newWindowRequested(const QUrl &url);
    void quitRequested();
    void quitReady();
    void quitCanceled();
    void windowClosed();
    void saveFailed();
    void historyChanged();
    void documentLoaded();
    void fileUrlChanged();
    void modifiedChanged();
    void statusChanged();
    void documentStatisticsChanged();
    void outputStyleChanged();
    void outputPageLayoutChanged();
    void outputCssChanged();
    void publishingThemesChanged();
    // Emitted only when the CSS that reaches published output (the selected
    // theme's sanitized stylesheet, or its error) actually changed. Catalog
    // changes and unrelated files in the themes folder do not emit it.
    void publishingCssChanged();
    void publishingPreviewReady(quint64 requestId, const QVariantMap &result);
    void publishingDocumentIdentityChanged();
    void themePresetChanged();
    void darkModeChanged();
    void appearanceFollowsOutputStyleChanged();
    void textScaleChanged();
    void themeColorsChanged();
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
    QString outputHtml(QTextDocument &document, QString *error) const;
    // PublishingSource: what Publisher may know about this document and the
    // output settings. Publishing state itself lives in Publisher.
    bool hasDocument() const override;
    QString publishingMarkdown() const override;
    QByteArray sourceHash() const override;
    int sourceRevision() const override;
    QUrl documentUrl() const override;
    QString documentTitle() const override;
    QString basicStyleCss() const override;
    QString printCss() const override;
    QString titlePageHtml() const override;
    QUrl outputCssFile() const override;
    QPageLayout outputPageLayout() const override;
    void invalidatePublishingDocument();
    std::unique_ptr<Publisher> m_publisher;
    // Mirrors the Live editor's change lists into the canonical document.
    bool applyLiveChanges(const QString &changesJson, int revision);
    void resolveLiveImage(int token, const QString &src);
    void saveLiveImage(int token, const QString &name, const QString &mime, const QString &base64);
    // The other direction: document edits made on the C++ side (format commands,
    // version restore, replace) reach the page while Live is active.
    void forwardLiveChange(int position, int charsRemoved, int charsAdded);
    bool m_applyingLiveChanges = false;
    // Live commands waiting for the page's selection (FIFO, one request in flight).
    void runAtLiveSelection(std::function<void(int start, int end)> operation);
    void pumpLiveSelection();
    void finishLiveSelection(int start, int end);
    // Each queued operation remembers the document generation it was issued
    // for; a reply that arrives after the document was swapped drops it.
    struct LiveSelectionOperation { int generation; std::function<void(int, int)> run; };
    std::deque<LiveSelectionOperation> m_liveSelectionQueue;
    int m_liveSelectionToken = 0;
    int m_liveDocumentGeneration = 0;
    QString m_liveMirror;
    bool m_liveMirrorValid = false;
    std::unique_ptr<EditorBridge> m_editorBridge;
    SpellCheck m_spellCheck{this};
    void paintOutput(QPagedPaintDevice &device, QTextDocument &document) const;
    void paintPublishingOutput(QPagedPaintDevice &device);
    void applyTemplate(QTextDocument &document, bool preview) const;
    void prepareOutput(QTextDocument &document, bool plain = false) const;
    QPageLayout effectiveOutputPageLayout() const;
    void restoreOutputPageLayout();
    void persistOutputPageLayout();
    void loadUserOutputStyles();
    bool saveUserOutputStyles();
    bool applyUserOutputStyle(const QVariantMap &style);
    int m_outputStyle = 0;
    QString m_customOutputFont;
    int m_customOutputSize = 12;
    QString m_outputHeader, m_outputFooter;
    bool m_outputTitlePage = false;
    bool m_customPageFurniture = true;
    QVariantList m_userOutputStyles;
    QString m_selectedUserOutputStyleId;
    bool m_userOutputStylesLoadFailed = false;
    QPageLayout m_pageLayout;
    QUrl m_outputCssFile;
    void saveTo(const QUrl &url, bool protectExternalChanges = false);
    QUrl suggestedSaveUrl() const;
    QString currentDocumentText() const;
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

    bool m_showMarkup = true;
    QString m_sourceAppearance = "manuscript";
    int m_codeIndentWidth = 0;
    bool m_codeIndentTabs = false;
    FileLibrary m_library;
    QUrl m_fileUrl;
    bool m_modified = false;
    QString m_status;
    bool m_systemDarkMode = true;
    QString m_themePreset = "studio";
    bool m_appearanceFollowsOutputStyle = true;
    bool m_appearanceOverridden = false;
    bool m_themeColorsAnnounced = false;
    bool m_darkMode = true;
    qreal m_textScale = 1.0;
    bool m_loading = false;
    bool m_formattingTypography = false;
    QMap<int, int> m_typographyUndoRanges;
    int m_observedUndoSteps = 0;
    bool m_sourceHistoryTraversal = false;
    QTimer m_recoveryTimer;
    QFileSystemWatcher m_fileWatcher;
    QPointer<QTextDocument> m_document;
    QPointer<QWindow> m_parentWindow;
    QPointer<MarkdownHighlighter> m_highlighter;
    QString m_lastDocumentText;
    QByteArray m_lastKnownFileContents;
    bool m_requiresExplicitSave = false;
    bool m_hasKnownFileContents = false;
    bool m_requiresViewConflictCheck = false;
    // Cycle131 byte-exact persistence. The document is edited as LF text, but
    // the bytes written back reproduce the file's own byte-order mark and
    // line-ending convention, so an untouched document round-trips exactly.
    enum class LineEnding { Lf, CrLf, Cr };
    LineEnding m_lineEnding = LineEnding::Lf;
    bool m_hadByteOrderMark = false;
    bool m_mixedLineEndings = false;
    // Set when the opened bytes were not valid UTF-8 (or were UTF-16) and so
    // could not be read losslessly. Saving over the original is refused;
    // Save As writes a UTF-8 copy and clears the flag.
    bool m_lossyDecode = false;
    bool decodeDocumentBytes(const QByteArray &bytes, QString *text);
    QByteArray encodeDocumentText(const QString &text) const;
    void resetPersistenceState();
    QString m_recoveryPath;
    std::unique_ptr<QLockFile> m_recoveryLock;

    QString m_themeBackground;
    QString m_themeForeground;
    QString m_themeAccent;
    QString m_themeSelection;
    QFileSystemWatcher m_themeWatcher;
};
