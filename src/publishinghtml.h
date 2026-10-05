#ifndef PUBLISHINGHTML_H
#define PUBLISHINGHTML_H

#include <QString>
#include <QUrl>

namespace PublishingHtml {
// Semantic contents for the publishing shell's <article id="write">.
// Input has already passed through Backend::previewMarkdown().
QString body(const QString &expandedMarkdown, QString *error = nullptr);
// Embed bounded local assets. Previews replace unavailable images with visible
// placeholders; exports fail so incomplete output cannot be mistaken for success.
QString embedImages(QString html, const QUrl &baseUrl, bool preview,
                    QString *error = nullptr, QString *warning = nullptr);
}

#endif
