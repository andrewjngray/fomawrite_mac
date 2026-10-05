#ifndef PUBLISHINGPDF_H
#define PUBLISHINGPDF_H

#include <QByteArray>
#include <QObject>
#include <QPageLayout>
#include <QString>
#include <QUrl>
#include <memory>

// Browser PDF output for the same portable HTML used by publishing preview.
// Call on the GUI thread. Assets must be embedded as data URLs; network access
// and local resources other than the private HTML document are blocked.
class PublishingPdf final : public QObject
{
    Q_OBJECT
public:
    explicit PublishingPdf(QObject *parent = nullptr);
    ~PublishingPdf() override;

    // One request renders at a time. While busy, only the latest pending request
    // is retained. Superseded/cancelled requests finish with an error and no PDF.
    void render(quint64 requestId, const QString &html, const QUrl &baseUrl,
                const QPageLayout &layout);
    void cancel();

    // Compatibility bridge for synchronous exports. Processes Qt events while
    // waiting, excludes user input, and refuses nested blocking renders.
    static QByteArray renderBlocking(const QString &html, const QUrl &baseUrl,
                                    const QPageLayout &layout, QString *error = nullptr,
                                    int timeoutMs = 30000);

signals:
    void finished(quint64 requestId, const QByteArray &pdf, const QString &error);

private:
    struct Private;
    std::unique_ptr<Private> d;
    void startPending();
    void pollReady(quint64 generation);
    void finish(quint64 generation, const QByteArray &pdf, const QString &error);
};

#endif
