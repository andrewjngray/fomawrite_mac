#include "acceptanceprofile.h"

void FomawriteTest::acceptanceProfileRejectsInvalidRoots() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QFile existingFile(temporary.filePath("file"));
    QVERIFY(existingFile.open(QIODevice::WriteOnly));
    QCOMPARE(existingFile.write("existing", 8), qint64(8));
    existingFile.close();
    for (const QString &invalid : {QString(), QStringLiteral("relative-profile"),
            temporary.filePath("missing"), existingFile.fileName()}) {
        AcceptanceProfile profile{QStringLiteral("old"), QStringLiteral("old")};
        QString error;
        QVERIFY(!prepareAcceptanceProfile(invalid, &profile, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(profile.root.isEmpty());
        QVERIFY(profile.applicationName.isEmpty());
    }
    QVERIFY(!QFileInfo::exists(temporary.filePath("missing")));
    QVERIFY(existingFile.open(QIODevice::ReadOnly));
    QCOMPARE(existingFile.readAll(), QByteArray("existing"));
}

void FomawriteTest::acceptanceProfileKeepsStableCanonicalNamespaceAndExistingFiles() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString root = temporary.filePath(QString::fromUtf8("profile café"));
    QVERIFY(QDir().mkdir(root));
    // A prior receipt must not prevent a restored launch or be erased by setup.
    QFile receipt(QDir(root).filePath("profile.json"));
    QVERIFY(receipt.open(QIODevice::WriteOnly));
    QCOMPARE(receipt.write("previous receipt", 16), qint64(16));
    receipt.close();
    const QString appNameBefore = QCoreApplication::applicationName();
    AcceptanceProfile first, repeated, distinct;
    QString error;
    QVERIFY2(prepareAcceptanceProfile(root, &first, &error), qPrintable(error));
    const QString canonical = QFileInfo(root).canonicalFilePath();
    QCOMPARE(first.root, canonical);
    QCOMPARE(first.applicationName, QStringLiteral("FomawriteAcceptance-")
        + QString::fromLatin1(QCryptographicHash::hash(canonical.toUtf8(),
            QCryptographicHash::Sha256).toHex().left(20)));
    QVERIFY2(prepareAcceptanceProfile(QDir(root).filePath("."), &repeated, &error), qPrintable(error));
    QCOMPARE(repeated.root, first.root);
    QCOMPARE(repeated.applicationName, first.applicationName);
    QVERIFY2(prepareAcceptanceProfile(temporary.path(), &distinct, &error), qPrintable(error));
    QVERIFY(distinct.applicationName != first.applicationName);
    QCOMPARE(QCoreApplication::applicationName(), appNameBefore);
    QCOMPARE(QDir(root).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot),
             QStringList{QStringLiteral("profile.json")});
    QVERIFY(receipt.open(QIODevice::ReadOnly));
    QCOMPARE(receipt.readAll(), QByteArray("previous receipt"));
}
