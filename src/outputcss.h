#pragma once

#include <QString>
#include <QUrl>
#include <optional>

// User-selected CSS is embedded only in exported HTML. It never styles the
// editor or mutates canonical Markdown.
namespace OutputCss {
std::optional<QString> load(const QUrl &file, QString *error = nullptr);
std::optional<QString> embed(const QString &html, const QString &css);
}
