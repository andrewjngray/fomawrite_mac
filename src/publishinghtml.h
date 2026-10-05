#ifndef PUBLISHINGHTML_H
#define PUBLISHINGHTML_H

#include <QString>

namespace PublishingHtml {
// Semantic contents for the publishing shell's <article id="write">.
// Input has already passed through Backend::previewMarkdown().
QString body(const QString &expandedMarkdown, QString *error = nullptr);
}

#endif
