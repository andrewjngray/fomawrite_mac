// Cycle140 — fixtures shared by the Typora base shim tests (cycle140-typora-base.inc)
// and the Live capture test. tests/fixtures/typora/lapis.css + lapis-dark.css are
// copies of the Lapis theme (YiNN, MIT); lapis-dark imports lapis and overrides
// its colours with the dark palette.

#include "typorabase.h"

namespace {
// Copies the Lapis fixtures into the (test-mode) themes folder and removes them again.
struct Cycle140Lapis {
    QString folder = PublishingThemes::directory();
    QStringList files;
    bool install() {
        if (!QDir().mkpath(folder)) return false;
        for (const char *name : {"lapis.css", "lapis-dark.css"}) {
            const QString source = QFINDTESTDATA(QStringLiteral("fixtures/typora/") + QString::fromLatin1(name));
            if (source.isEmpty()) return false;
            const QString target = folder + '/' + QString::fromLatin1(name);
            QFile::remove(target);
            if (!QFile::copy(source, target)) return false;
            files.append(target);
        }
        return true;
    }
    ~Cycle140Lapis() { for (const QString &file : files) QFile::remove(file); }
};

QString cycle140Html(const QUrl &url) {
    QFile file(url.toLocalFile());
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QStringLiteral("<unreadable>");
}

int cycle140Luma(const QColor &c) { return (299 * c.red() + 587 * c.green() + 114 * c.blue()) / 1000; }
}
