#ifndef PUBLISHINGTHEMES_H
#define PUBLISHINGTHEMES_H
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <optional>

namespace PublishingThemes {
QVariantList catalog();
QString directory();
// On success, error may contain an advisory about omitted export directives or missing optional fonts.
bool importTheme(QUrl cssFile, QString *importedId, QString *error);
// Folder CSS is resolved and embedded on each use; advisory reports optional font fallbacks.
std::optional<QString> css(QString id, QString *error, QString *advisory = nullptr);
}
#endif
