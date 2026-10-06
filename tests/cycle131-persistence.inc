// Cycle131 — byte-exact persistence. A document that is opened and saved
// without edits must round-trip byte-for-byte, an edited document must keep
// the file's own conventions, and bytes that could not be read exactly must
// never be written back in place.

namespace {
struct PersistenceFixture {
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> editor;

    bool create() {
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
        editor.reset(component.create());
        return !editor.isNull();
    }
    // TextEdit.text goes through toPlainText(), which rewrites no-break
    // spaces; read the canonical document text the way Backend does.
    QString text() const {
        auto *quick = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument"));
        QString raw = quick && quick->textDocument() ? quick->textDocument()->toRawText() : QString();
        return raw.replace(QChar(QChar::ParagraphSeparator), QLatin1Char('\n'));
    }
    bool insert(int position, const QString &value) {
        return QMetaObject::invokeMethod(editor.data(), "insert", Q_ARG(int, position), Q_ARG(QString, value));
    }
};

QByteArray persistenceBytes(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray("<unreadable>");
}

bool persistenceWrite(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size();
}
}

void FomawriteTest::persistenceRoundTripsBytesExactly() {
    struct Fixture { const char *name; QByteArray bytes; QByteArray eol; bool bom; QString expectedText; };
    const QList<Fixture> fixtures = {
        {"lf.md",         "# Title\n\nline one\nline two\n",                  "\n",   false, "# Title\n\nline one\nline two\n"},
        {"crlf.md",       "# Title\r\n\r\nline one\r\nline two\r\n",          "\r\n", false, "# Title\n\nline one\nline two\n"},
        {"cr.md",         "# Title\r\rline one\rline two\r",                  "\r",   false, "# Title\n\nline one\nline two\n"},
        {"bom.md",        "\xEF\xBB\xBF# Title\n\nbody\n",                    "\n",   true,  "# Title\n\nbody\n"},
        {"bom-crlf.md",   "\xEF\xBB\xBFone\r\ntwo\r\n",                       "\r\n", true,  "one\ntwo\n"},
        {"nbsp.md",       "10\xC2\xA0km and \xC2\xAB quoted \xC2\xBB\n",      "\n",   false, QString::fromUtf8("10\xC2\xA0km and \xC2\xAB quoted \xC2\xBB\n")},
        {"separator.md",  "alpha\xE2\x80\xA8" "beta\n",                       "\n",   false, QString::fromUtf8("alpha\xE2\x80\xA8" "beta\n")},
        {"no-final.md",   "no final newline",                                 "\n",   false, "no final newline"},
        {"whitespace.md", "trailing  \n\ttabbed\n   \nhard break  \nend\n",   "\n",   false, "trailing  \n\ttabbed\n   \nhard break  \nend\n"},
    };
    QTemporaryDir directory; QVERIFY(directory.isValid());
    for (const Fixture &fixture : fixtures) {
        const QString path = directory.filePath(QString::fromLatin1(fixture.name));
        QVERIFY2(persistenceWrite(path, fixture.bytes), fixture.name);
        PersistenceFixture f; QVERIFY2(f.create(), fixture.name);
        QVERIFY2(f.backend.open(QUrl::fromLocalFile(path)), fixture.name);
        QVERIFY2(!f.backend.modified(), fixture.name);
        QVERIFY2(!f.backend.status().contains("UTF-8"), qPrintable(QString("%1: %2").arg(fixture.name, f.backend.status())));
        QCOMPARE(f.text(), fixture.expectedText);

        // Saving an untouched document must not change a single byte.
        QSignalSpy failed(&f.backend, &Backend::saveFailed);
        f.backend.save();
        QCOMPARE(failed.size(), 0);
        QVERIFY2(!f.backend.modified(), fixture.name);
        QCOMPARE(persistenceBytes(path), fixture.bytes);

        // An edit keeps the file's byte-order mark and line-ending style.
        QVERIFY2(f.insert(0, "Edited\n"), fixture.name);
        QVERIFY2(f.backend.modified(), fixture.name);
        f.backend.save();
        QCOMPARE(failed.size(), 0);
        QVERIFY2(!f.backend.modified(), fixture.name);
        QByteArray expected = fixture.bytes;
        if (fixture.bom) expected.remove(0, 3);
        expected.prepend("Edited" + fixture.eol);
        if (fixture.bom) expected.prepend("\xEF\xBB\xBF");
        QCOMPARE(persistenceBytes(path), expected);
        f.backend.discardRecovery();
    }
}

void FomawriteTest::persistenceRefusesLossyDecodeAndSaveAsWritesUtf8() {
    QTemporaryDir directory; QVERIFY(directory.isValid());

    // Latin-1: not valid UTF-8. The readable text is shown, but the original
    // bytes must survive Save and autosave; Save As writes a UTF-8 copy.
    const QString latin = directory.filePath("latin1.md");
    const QByteArray latinBytes("caf\xE9 au lait\n");
    QVERIFY(persistenceWrite(latin, latinBytes));
    {
        PersistenceFixture f; QVERIFY(f.create());
        QVERIFY(f.backend.open(QUrl::fromLocalFile(latin)));
        QVERIFY2(f.backend.status().contains("UTF-8"), qPrintable(f.backend.status()));
        QCOMPARE(f.text(), QString("caf") + QChar(0xFFFD) + " au lait\n");
        QVERIFY(!f.backend.modified());
        QSignalSpy failed(&f.backend, &Backend::saveFailed);
        f.backend.save();
        QCOMPARE(failed.size(), 1);
        QCOMPARE(persistenceBytes(latin), latinBytes);
        QVERIFY2(f.backend.status().contains("Save As"), qPrintable(f.backend.status()));

        QVERIFY(f.insert(0, "Edited "));
        QVERIFY(f.backend.modified());
        f.backend.autosave(); // refused too, and reported as a failed save
        QCOMPARE(failed.size(), 2);
        QCOMPARE(persistenceBytes(latin), latinBytes);
        QVERIFY(f.backend.modified());
        f.backend.save();
        QCOMPARE(failed.size(), 3);
        QCOMPARE(persistenceBytes(latin), latinBytes);

        const QString copy = directory.filePath("latin1-utf8.md");
        f.backend.saveAs(QUrl::fromLocalFile(copy));
        QCOMPARE(failed.size(), 3);
        QVERIFY(!f.backend.modified());
        QCOMPARE(persistenceBytes(copy), QByteArray("Edited caf\xEF\xBF\xBD au lait\n"));
        QCOMPARE(persistenceBytes(latin), latinBytes);
        // The copy is exact UTF-8, so ordinary saving resumes there.
        QVERIFY(f.insert(0, "Again "));
        f.backend.save();
        QCOMPARE(failed.size(), 3);
        QCOMPARE(persistenceBytes(copy), QByteArray("Again Edited caf\xEF\xBF\xBD au lait\n"));
        f.backend.discardRecovery();
    }

    // UTF-16 with a byte-order mark: decoded faithfully for reading, never
    // rewritten in place as UTF-8.
    const QString utf16 = directory.filePath("utf16.md");
    const QByteArray utf16Bytes("\xFF\xFE" "a\0b\0\n\0", 8);
    QVERIFY(persistenceWrite(utf16, utf16Bytes));
    {
        PersistenceFixture f; QVERIFY(f.create());
        QVERIFY(f.backend.open(QUrl::fromLocalFile(utf16)));
        QCOMPARE(f.text(), QString("ab\n"));
        QVERIFY2(f.backend.status().contains("UTF-8"), qPrintable(f.backend.status()));
        QSignalSpy failed(&f.backend, &Backend::saveFailed);
        f.backend.save();
        QCOMPARE(failed.size(), 1);
        QCOMPARE(persistenceBytes(utf16), utf16Bytes);
        f.backend.discardRecovery();
    }
}

void FomawriteTest::crlfDocumentsKeepAutosaveRenameAndMove() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString path = directory.filePath("windows.md");
    QVERIFY(persistenceWrite(path, "one\r\ntwo\r\n"));
    PersistenceFixture f; QVERIFY(f.create());
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QCOMPARE(f.text(), QString("one\ntwo\n"));

    // Before Cycle131 the normalized baseline never matched the CRLF disk
    // bytes, so autosave stayed paused and rename/move were refused.
    QVERIFY(f.insert(0, "x"));
    QVERIFY(f.backend.modified());
    f.backend.autosave();
    QVERIFY2(!f.backend.modified(), qPrintable(f.backend.status()));
    QCOMPARE(persistenceBytes(path), QByteArray("xone\r\ntwo\r\n"));

    QVERIFY2(f.backend.renameDocument("renamed.md"), qPrintable(f.backend.status()));
    const QString renamed = directory.filePath("renamed.md");
    QCOMPARE(f.backend.fileUrl(), QUrl::fromLocalFile(renamed));
    QCOMPARE(persistenceBytes(renamed), QByteArray("xone\r\ntwo\r\n"));
    QVERIFY(!QFileInfo::exists(path));

    QVERIFY(QDir(directory.path()).mkdir("moved"));
    QVERIFY2(f.backend.moveDocument(QUrl::fromLocalFile(directory.filePath("moved"))), qPrintable(f.backend.status()));
    QCOMPARE(persistenceBytes(directory.filePath("moved/renamed.md")), QByteArray("xone\r\ntwo\r\n"));

    // Later edits in the moved file still use CRLF.
    QVERIFY(f.insert(0, "y"));
    f.backend.save();
    QVERIFY(!f.backend.modified());
    QCOMPARE(persistenceBytes(directory.filePath("moved/renamed.md")), QByteArray("yxone\r\ntwo\r\n"));
    f.backend.discardRecovery();
}

void FomawriteTest::manualSaveRespectsDiskBaseline() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString path = directory.filePath("shared.md");
    QVERIFY(persistenceWrite(path, "original\n"));
    PersistenceFixture f; QVERIFY(f.create());
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(f.insert(8, " mine"));
    QCOMPARE(f.text(), QString("original mine\n"));

    // Another writer replaces the file while this window is not watching.
    // A plain Save must stop and offer the external-change choice rather
    // than silently overwriting the newer version.
    QVERIFY(persistenceWrite(path, "external\n"));
    QSignalSpy external(&f.backend, &Backend::externalChangeDetected);
    QSignalSpy failed(&f.backend, &Backend::saveFailed);
    f.backend.save();
    QCOMPARE(persistenceBytes(path), QByteArray("external\n"));
    QVERIFY(f.backend.modified());
    QCOMPARE(failed.size(), 1);
    QVERIFY(external.size() >= 1);
    QCOMPARE(external.first().at(0).toBool(), false);
    QCOMPARE(external.first().at(1).toBool(), true);
    QVERIFY2(f.backend.status().contains("changed on disk"), qPrintable(f.backend.status()));
    QCOMPARE(f.text(), QString("original mine\n"));

    // Keep Mine adopts the disk version as the baseline; the next Save is an
    // explicit replacement (this is also what the Save Anyway button does).
    f.backend.keepExternalVersion();
    f.backend.save();
    QCOMPARE(failed.size(), 1);
    QVERIFY(!f.backend.modified());
    QCOMPARE(persistenceBytes(path), QByteArray("original mine\n"));
    f.backend.discardRecovery();
}
