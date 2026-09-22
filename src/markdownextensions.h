#pragma once
#include <QString>
#include <QUrl>

// Preview-only extensions. Source documents remain ordinary UTF-8 Markdown.
QString expandedMarkdown(const QString &source, const QUrl &base);
