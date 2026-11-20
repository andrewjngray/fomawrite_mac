#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// Writing a PDF safely (Cycle 165 part 2b): a temporary file beside the target,
// verified, the previous bytes kept in a recovery store, then an atomic
// replace. Pure file work; the session is never involved.
namespace PdfFiles {

struct Options {
    QString path;              // the file to replace (or create)
    QByteArray bytes;          // what it becomes
    int expectedPages = 0;     // the verified file must open with this many pages
    QString recoveryFolder;    // the recovery store's base folder; empty: no copy is kept
    int keepCopies = 3;        // the latest copies kept per file
    bool requirePrevious = false;
    QByteArray previous;       // with requirePrevious: the file must still hold exactly this
    QString failAt;            // tests: "write", "verify", "recovery" or "replace" fails there
};

// An empty string on success, otherwise a plain-English reason. On failure the
// file at `path` is exactly as it was and no temporary file is left behind.
QString writeVerified(const Options &options);
QString changedOnDiskError();
// Whether `bytes` opens in Qt PDF with `pages` pages (0: any number).
bool opens(const QByteArray &bytes, int pages, QString *why = nullptr);
// The folder holding the recovery copies of `pdfPath` and the copies, newest first.
QString recoveryFolderFor(const QString &base, const QString &pdfPath);
QStringList recoveryCopies(const QString &base, const QString &pdfPath);

}
