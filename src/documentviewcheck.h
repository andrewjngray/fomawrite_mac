#pragma once

#include <QString>

class QApplication;

// Explicit diagnostic mode: run the shipped QML with disposable state, before
// ordinary startup reads preferences, recovers documents or contacts another app.
int runDocumentViewCheck(QApplication &app, const QString &outputDirectory);
