#pragma once

// Native macOS helpers implemented in windowchrome_mac.mm. One declaration
// per function, so a signature change is a compile error everywhere instead
// of a link error or a silently mismatched call.

#include <QtGlobal>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantList>

class QWindow;

#ifdef Q_OS_MACOS
bool setMacRunningDockIcon(bool running);
void migrateMacPreferences();
void configureMacWindowChrome(QWindow *window);
void performMacWindowAction(QWindow *window, const QString &action, const QString &text);
QStringList macSpellingIssues(const QString &text);
QString createMacVersion(const QString &path);
QVariantList macVersions(const QString &path);
QVariantList macWordClasses(const QString &text);
int macTabInset(QWindow *window);
void restoreMacWorkspaceTabs(const QList<QWindow *> &windows);
void applyMacWindowTheme(QWindow *window, bool followSystem, bool dark);
QStringList macWritingLanguages();
QVariantList macWritingIssues(const QString &text, const QString &language, bool grammar);
void macSpeakText(const QString &text);
void macStopSpeaking();
bool shareMacFile(QWindow *window, const QString &path);
#endif
