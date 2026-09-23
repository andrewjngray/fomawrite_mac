#include "markdownextensions.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QMap>
#include <functional>

static QString csvTable(const QString &text) {
    QList<QStringList> rows; QStringList row; QString cell; bool quoted=false;
    for (int i=0;i<text.size();++i) {
        const QChar c=text[i];
        if (c=='"') { if (quoted && i+1<text.size() && text[i+1]=='"') { cell+='"'; ++i; } else quoted=!quoted; }
        else if (!quoted && (c==',' || c=='\n')) {
            row.append(cell); cell.clear(); if(row.size()>50) return "> CSV block exceeds 50 columns.";
            if(c=='\n') { rows.append(row); row.clear(); if(rows.size()>100) return "> CSV block exceeds 100 rows."; }
        } else if(c!='\r') cell+=c;
    }
    if(quoted) return "> CSV block has an unfinished quoted field.";
    if(!cell.isEmpty() || !row.isEmpty()) { row.append(cell); rows.append(row); }
    if(rows.size()>100 || row.size()>50) return "> CSV block exceeds limits.";
    QString html="<table>";
    for(const auto &r:rows) { html+="<tr>"; for(const auto &c:r) html+="<td>"+c.toHtmlEscaped()+"</td>"; html+="</tr>"; }
    return html+"</table>";
}

QString expandedMarkdown(const QString &source, const QUrl &base) {
    QSet<QString> active; qint64 bytes=0; int namespaceId=0;
    std::function<QString(const QString &, const QUrl &, int)> expand;
    expand = [&](const QString &input, const QUrl &folder, int depth) {
        QStringList output, noteOrder; QMap<QString,QString> notes; QMap<QString,int> refs;
        const QString prefix="ow-note-"+QString::number(namespaceId++)+"-";
        QChar fence; int fenceLength=0;
        const QRegularExpression fenceRe("^ {0,3}(`{3,}|~{3,})(.*)$");
        const QRegularExpression blockRe("^ {0,3}/([^\\n]+\\.(?:md|markdown|txt|csv|png|jpe?g|gif|webp|cpp|h|py|js|json|css|sh))\\s*$",QRegularExpression::CaseInsensitiveOption);
        const QRegularExpression noteRe("^\\[\\^([\\w-]+)\\]:\\s*(.*)$");
        const QRegularExpression inlineRe(R"ow((`+)(.*?)\1|\[\[([^]\n]+)\]\]|==([^=\n]+)==|\[\^([\w-]+)\]|(!?\[[^]\n]*\]\()(<[^>\n]+>|[^)\s]+)(\s+"[^"]*")?\))ow");
        const auto lines=input.split('\n');
        for(int lineIndex=0;lineIndex<lines.size();++lineIndex) {
            const auto &line=lines[lineIndex]; const auto marker=fenceRe.match(line);
            if(marker.hasMatch()) { const auto run=marker.captured(1); if(fence.isNull()) { fence=run.at(0); fenceLength=run.size(); } else if(fence==run.at(0) && run.size()>=fenceLength && marker.captured(2).trimmed().isEmpty()) fence=QChar(); output.append(line); continue; }
            if(!fence.isNull() || line.startsWith("    ") || line.startsWith('\t')) { output.append(line); continue; }
            const auto note=noteRe.match(line);
            if(note.hasMatch()) {
                const QString key=note.captured(1); QString body=note.captured(2);
                while(lineIndex+1<lines.size()) {
                    const auto next=lines[lineIndex+1];
                    if(next.startsWith("    ") || next.startsWith('\t')) { body+='\n'+next.mid(next.startsWith('\t')?1:4); ++lineIndex; }
                    else if(next.isEmpty() && lineIndex+2<lines.size() && (lines[lineIndex+2].startsWith("    ") || lines[lineIndex+2].startsWith('\t'))) { body+='\n'; ++lineIndex; }
                    else break;
                }
                if(!notes.contains(key)) noteOrder.append(key); notes[key]=body; continue;
            }
            const auto block=blockRe.match(line);
            if(block.hasMatch()) {
                const QFileInfo info(QDir(folder.toLocalFile()).filePath(block.captured(1))); const auto canonical=info.canonicalFilePath();
                const auto parent=QFileInfo(folder.toLocalFile()).canonicalFilePath(); QFile file(canonical);
                if(depth>=5 || canonical.isEmpty() || info.isSymLink() || !canonical.startsWith(parent+'/') || active.contains(canonical)
                    || info.size()>262144 || bytes+info.size()>1048576 || !file.open(QIODevice::ReadOnly)) { output.append("> Content block unavailable: "+block.captured(1)); continue; }
                const auto content=file.read(262145); if(content.size()>262144 || file.error()!=QFile::NoError) { output.append("> Content block read failed."); continue; }
                bytes+=content.size(); const auto suffix=info.suffix().toLower();
                if(QStringList{"png","jpg","jpeg","gif","webp"}.contains(suffix)) output.append("!["+info.fileName()+"](<"+QUrl::fromLocalFile(canonical).toString(QUrl::FullyEncoded)+">)");
                else if(suffix=="csv") output.append(csvTable(QString::fromUtf8(content)));
                else if(QStringList{"md","markdown","txt"}.contains(suffix)) { active.insert(canonical); output.append(expand(QString::fromUtf8(content),QUrl::fromLocalFile(info.absolutePath()+'/'),depth+1)); active.remove(canonical); }
                else { QString code=QString::fromUtf8(content); int run=3; while(code.contains(QString(run,'`'))) ++run; const QString ticks(run,'`'); output.append(ticks+suffix+'\n'+code+'\n'+ticks); }
                continue;
            }
            QString transformed; int offset=0; auto matches=inlineRe.globalMatch(line);
            while(matches.hasNext()) {
                const auto match=matches.next(); transformed+=line.mid(offset,match.capturedStart()-offset);
                if(match.capturedStart()>0 && line.at(match.capturedStart()-1)=='\\') transformed+=match.captured();
                else if(!match.captured(1).isEmpty()) transformed+=match.captured();
                else if(!match.captured(3).isEmpty()) {
                    const auto pieces=match.captured(3).split('|'); QUrl target(pieces.first().trimmed());
                    QString path=target.path(); if(!path.isEmpty() && QFileInfo(path).suffix().isEmpty()) { path+=".md"; target.setPath(path); }
                    const auto url=folder.resolved(target); QString label=pieces.size()>1?pieces.mid(1).join('|'):pieces.first(); label.replace('[',"\\[").replace(']',"\\]");
                    transformed+=url.isLocalFile()?"["+label+"](<"+url.toString(QUrl::FullyEncoded)+">)":match.captured();
                } else if(!match.captured(4).isEmpty()) transformed+="<span style=\"background-color:#fff0a3;color:#222222\">"+match.captured(4).toHtmlEscaped()+"</span>";
                else if(!match.captured(5).isEmpty()) {
                    const auto key=match.captured(5); const int number=++refs[key]; const auto ref=prefix+key+"-ref-"+QString::number(number);
                    transformed+="<a name=\""+ref+"\"></a>["+key+"](#"+prefix+key+")";
                } else {
                    QString target=match.captured(7); if(target.startsWith('<')) target=target.mid(1,target.size()-2);
                    const QUrl url(target);
                    // Top-level links retain their source spelling. Included documents
                    // resolve their relative links/images against their own folder.
                    if(depth>0 && url.isRelative() && !target.startsWith('#')) transformed+=match.captured(6)+"<"+folder.resolved(url).toString(QUrl::FullyEncoded)+">"+match.captured(8)+")";
                    else transformed+=match.captured();
                }
                offset=match.capturedEnd();
            }
            transformed+=line.mid(offset); output.append(transformed);
        }
        for(const auto &key:noteOrder) {
            QString back; for(int n=1;n<=refs[key];++n) back+=" [↩"+QString::number(n)+"](#"+prefix+key+"-ref-"+QString::number(n)+")";
            output.append("\n<a name=\""+prefix+key+"\"></a>\n\n**"+key+":** "+notes[key]+back+"\n");
        }
        return output.join('\n');
    };
    return expand(source,base,0);
}
