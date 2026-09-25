#include "outputcss.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace OutputCss {

std::optional<QString> load(const QUrl &file, QString *error) {
    const auto fail = [error](const QString &message) -> std::optional<QString> {
        if (error) *error = message;
        return std::nullopt;
    };
    if (!file.isLocalFile()) return fail(QStringLiteral("Choose a local CSS file."));
    const QFileInfo info(file.toLocalFile());
    if (!info.isFile() || info.isSymLink() || info.size() > 64 * 1024)
        return fail(QStringLiteral("CSS must be a regular local file no larger than 64 KiB."));
    QFile input(info.absoluteFilePath());
    if (!input.open(QIODevice::ReadOnly)) return fail(QStringLiteral("Could not read CSS file."));
    const QByteArray bytes = input.read(64 * 1024 + 1);
    if (input.error() != QFileDevice::NoError || bytes.size() > 64 * 1024)
        return fail(QStringLiteral("Could not read bounded CSS file."));
    const QString css = QString::fromUtf8(bytes);
    if (css.toUtf8() != bytes || css.contains(QChar(0)))
        return fail(QStringLiteral("CSS must be valid UTF-8 text."));
    static const QRegularExpression disallowed(
        QStringLiteral("@(?:import|font-face|namespace)\\b|url\\s*\\(|</|(?:[A-Za-z][A-Za-z0-9+.-]*:)?\\/\\/|[\\x00-\\x08\\x0b\\x0c\\x0e-\\x1f]"),
        QRegularExpression::CaseInsensitiveOption);
    if (disallowed.match(css).hasMatch())
        return fail(QStringLiteral("CSS cannot load external assets or contain embedded markup."));
    if (error) error->clear();
    return css;
}

std::optional<QString> embed(const QString &html, const QString &css) {
    static const QRegularExpression headClose(QStringLiteral("</head\\s*>"),
                                               QRegularExpression::CaseInsensitiveOption);
    const auto match = headClose.match(html);
    if (!match.hasMatch()) return std::nullopt;
    QString result = html;
    result.insert(match.capturedStart(),
                  QStringLiteral("<style data-fomawrite-user-style>\n") + css
                      + QStringLiteral("\n</style>\n"));
    return result;
}

}
