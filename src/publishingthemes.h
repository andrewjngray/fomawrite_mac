#ifndef PUBLISHINGTHEMES_H
#define PUBLISHINGTHEMES_H
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <optional>

namespace PublishingThemes {
QVariantList catalog();
QString directory();
// On success, error may contain an advisory about omitted Typora export directives.
bool importTheme(QUrl cssFile, QString *importedId, QString *error);
std::optional<QString> css(QString id, QString *error);
}
#endif
