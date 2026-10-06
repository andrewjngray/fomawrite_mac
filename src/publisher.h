#pragma once

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QPageLayout>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include <optional>
#include "publishinghtml.h"

class PublishingPdf;
class QTemporaryDir;

// Everything the publisher needs to know about the document and the output
// settings. Backend implements it; the publisher never reaches into Backend,
// so publishing state cannot corrupt editing state or vice versa.
class PublishingSource {
public:
    virtual ~PublishingSource() = default;
    virtual bool hasDocument() const = 0;
    // Expanded Markdown ready for PublishingHtml::body().
    virtual QString publishingMarkdown() const = 0;
    virtual QByteArray sourceHash() const = 0;
    virtual int sourceRevision() const = 0;
    virtual QUrl documentUrl() const = 0;
    virtual QUrl documentBaseUrl() const = 0;
    virtual QString documentTitle() const = 0;
    // Inline stylesheet for the basic presets, used when no theme is selected.
    virtual QString basicStyleCss() const = 0;
    virtual QString printCss() const = 0;
    virtual QString titlePageHtml() const = 0;
    virtual QUrl outputCssFile() const = 0;
    virtual QPageLayout outputPageLayout() const = 0;
    virtual QString outputTemplateName() const = 0;
};

// Publishing: CSS themes (catalog, folder watching, sanitized and memoized
// CSS), semantic HTML/PDF generation, and the asynchronous, cached, per-consumer
// preview protocol. Owned by Backend and exposed to QML as backend.publisher;
// Backend keeps thin forwarders for the existing QML surface.
class Publisher : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString themeId READ themeId NOTIFY themesChanged)
    Q_PROPERTY(QString themeName READ themeName NOTIFY themesChanged)
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themesChanged)
    Q_PROPERTY(QString themeError READ themeError NOTIFY themesChanged)
    Q_PROPERTY(QString documentIdentity READ documentIdentity NOTIFY documentIdentityChanged)
public:
    explicit Publisher(PublishingSource &source, QObject *parent = nullptr);
    ~Publisher() override;

    QString themeId() const { return m_themeId; }
    QString themeName() const;
    QVariantList themes() const;
    QString themeError() const { return m_themeError; }
    Q_INVOKABLE bool selectTheme(const QString &id);
    Q_INVOKABLE bool importTheme(const QUrl &file);
    Q_INVOKABLE void reloadThemes();
    Q_INVOKABLE bool openThemesFolder();
    // Basic presets replace any selected theme without announcing a theme change;
    // the caller's style signal re-renders the preview.
    void clearTheme();
    // The stylesheet published output uses right now: the selected theme's
    // sanitized CSS, or the basic style's inline CSS when no theme is selected.
    QString currentCss() const;
    PublishingHtml::ImageCache &imageCache() const { return m_imageCache; }

    QString documentIdentity() const { return QString::number(m_documentGeneration); }
    // Document text, URL or lifetime changed: retire every pending render.
    void invalidateDocument();
    // Output settings changed: retire every pending render, keep the document.
    void invalidateSettings();

    // Complete publishing HTML. fingerprint identifies the output from its
    // inputs without hashing the embedded fonts and images.
    QString html(QString *error, bool preview = false, QString *warning = nullptr,
                 QByteArray *fingerprint = nullptr) const;
    QByteArray pdfBytes(const QString &html, QString *error) const;
    // Synchronous private temporary output; never saves or changes the source.
    Q_INVOKABLE QVariantMap preview(const QString &format);
    Q_INVOKABLE QVariantMap requestPreview(const QString &format, QObject *consumer, const QString &requestIdentity = QString());
    Q_INVOKABLE void cancelPreview(QObject *consumer);

signals:
    void themesChanged();
    // Only when the CSS that reaches published output (the selected theme's
    // sanitized stylesheet, or its error) actually changed.
    void cssChanged();
    void previewReady(quint64 requestId, const QVariantMap &result);
    void documentIdentityChanged();
    void statusMessage(const QString &message);

private:
    QByteArray watchThemes();
    void refreshThemes(bool force = false);
    bool updateCss();
    std::optional<QString> selectedCss(QString *error, QByteArray *hash = nullptr) const;
    QVariantMap storeOutput(quint64 generation, const QString &format, const QByteArray &bytes, const QString &html = {});

    PublishingSource &m_source;
    QString m_themeId;
    QString m_themeError;
    QFileSystemWatcher m_watcher;
    QTimer m_refreshTimer;
    QByteArray m_snapshot;
    QHash<QString, QPair<QByteArray, QByteArray>> m_fileHashes;
    struct CssMemo { QString themeId; QByteArray snapshot; QString css; QByteArray hash; QString error; bool valid = false; };
    mutable CssMemo m_cssMemo;
    QByteArray m_cssSnapshot;
    QByteArray m_catalogSnapshot;
    mutable PublishingHtml::ImageCache m_imageCache;
    quint64 m_documentGeneration = 0;
    quint64 m_settingsGeneration = 0;
    struct PreviewCache { QByteArray key; QVariantMap result; };
    QHash<QObject *, PreviewCache> m_previewCache;
    QHash<QObject *, QByteArray> m_pendingKeys;
    QHash<QObject *, QMetaObject::Connection> m_connections;
    QSet<QObject *> m_consumers;
    QHash<QObject *, PublishingPdf *> m_renderers;
    QHash<QObject *, quint64> m_requests;
    QHash<QObject *, QString> m_pinned;
    std::unique_ptr<QTemporaryDir> m_directory;
    QStringList m_files;
    quint64 m_generation = 0;
};
