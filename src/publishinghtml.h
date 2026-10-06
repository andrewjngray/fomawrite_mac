#ifndef PUBLISHINGHTML_H
#define PUBLISHINGHTML_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QUrl>

namespace PublishingHtml {
// Semantic contents for the publishing shell's <article id="write">.
// Input has already passed through Backend::previewMarkdown().
QString body(const QString &expandedMarkdown, QString *error = nullptr);
// Decoded image results keyed by path, reused while the file's size and
// modification time are unchanged, so a preview does not re-read and decode
// every image on each keystroke pause.
struct ImageEntry { qint64 size = -1; qint64 modified = -1; QString dataUrl; QString reason; };
using ImageCache = QHash<QString, ImageEntry>;
// Embed bounded local assets. Previews replace unavailable images with visible
// placeholders; exports fail so incomplete output cannot be mistaken for success.
// assetSignature receives path/size/mtime of every local image referenced, so
// callers can identify the output without hashing the embedded bytes.
QString embedImages(QString html, const QUrl &baseUrl, bool preview,
                    QString *error = nullptr, QString *warning = nullptr,
                    ImageCache *cache = nullptr, QByteArray *assetSignature = nullptr);
}

#endif
