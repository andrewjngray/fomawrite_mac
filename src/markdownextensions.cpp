#include "markdownextensions.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <functional>

QString expandedMarkdown(const QString &source, const QUrl &base) {
    QSet<QString> active;
    qint64 bytes = 0;
    std::function<QString(const QString &, const QUrl &, int)> expand;
    expand = [&](const QString &input, const QUrl &folder, int depth) {
        QStringList output;
        QStringList notes;
        QChar fence;
        int fenceLength = 0;
        const QRegularExpression fenceRe("^ {0,3}(`{3,}|~{3,})(.*)$");
        const QRegularExpression blockRe("^ {0,3}/([^\\n]+\\.(?:md|markdown|txt))\\s*$", QRegularExpression::CaseInsensitiveOption);
        const QRegularExpression noteRe("^\\[\\^([\\w-]+)\\]:\\s*(.*)$");
        const QRegularExpression inlineRe("(`+)(.*?)\\1|\\[\\[([^]\\n]+)\\]\\]|==([^=\\n]+)==|\\[\\^([\\w-]+)\\]");
        for (const QString &line : input.split('\n')) {
            const auto marker = fenceRe.match(line);
            if (marker.hasMatch()) {
                const auto run = marker.captured(1);
                if (fence.isNull()) { fence = run.at(0); fenceLength = run.size(); }
                else if (fence == run.at(0) && run.size() >= fenceLength && marker.captured(2).trimmed().isEmpty()) fence = QChar();
                output.append(line); continue;
            }
            if (!fence.isNull() || line.startsWith("    ") || line.startsWith('\t')) { output.append(line); continue; }
            const auto block = blockRe.match(line);
            if (block.hasMatch()) {
                const QFileInfo info(QDir(folder.toLocalFile()).filePath(block.captured(1)));
                const QString canonical = info.canonicalFilePath();
                const QString parent = QFileInfo(folder.toLocalFile()).canonicalFilePath();
                QFile file(canonical);
                if (depth >= 5 || canonical.isEmpty() || info.isSymLink() ||
                    !canonical.startsWith(parent + '/') || active.contains(canonical) ||
                    info.size() > 262144 || bytes + info.size() > 1048576 || !file.open(QIODevice::ReadOnly)) {
                    output.append("> Content block unavailable: " + block.captured(1)); continue;
                }
                const QByteArray content = file.read(262145);
                if (content.size() > 262144 || file.error() != QFile::NoError) { output.append("> Content block read failed."); continue; }
                bytes += content.size(); active.insert(canonical);
                output.append(expand(QString::fromUtf8(content), QUrl::fromLocalFile(info.absolutePath() + '/'), depth + 1));
                active.remove(canonical); continue;
            }
            const auto note = noteRe.match(line);
            if (note.hasMatch()) { notes.append("- **" + note.captured(1) + ":** " + note.captured(2)); continue; }
            QString transformed;
            int offset = 0;
            auto matches = inlineRe.globalMatch(line);
            while (matches.hasNext()) {
                const auto match = matches.next();
                transformed += line.mid(offset, match.capturedStart() - offset);
                if (match.capturedStart() > 0 && line.at(match.capturedStart() - 1) == '\\') transformed += match.captured();
                else if (!match.captured(1).isEmpty()) transformed += match.captured();
                else if (!match.captured(3).isEmpty()) {
                    const auto pieces = match.captured(3).split('|');
                    QString target = pieces.first().trimmed();
                    if (QFileInfo(target).suffix().isEmpty()) target += ".md";
                    const QUrl url = folder.resolved(QUrl(target));
                    QString label = pieces.size() > 1 ? pieces.mid(1).join('|') : pieces.first();
                    label.replace('[', "\\[").replace(']', "\\]");
                    transformed += url.isLocalFile() ? "[" + label + "](<" + url.toString(QUrl::FullyEncoded) + ">)" : match.captured();
                } else if (!match.captured(4).isEmpty()) transformed += "<span style=\"background-color:#fff0a3;color:#222222\">" + match.captured(4).toHtmlEscaped() + "</span>";
                else transformed += "<sup>" + match.captured(5).toHtmlEscaped() + "</sup>";
                offset = match.capturedEnd();
            }
            transformed += line.mid(offset);
            output.append(transformed);
        }
        if (!notes.isEmpty()) output.append("\n---\n\n" + notes.join('\n'));
        return output.join('\n');
    };
    return expand(source, base, 0);
}
