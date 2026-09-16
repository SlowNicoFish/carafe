#include "game.h"
#include "storage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

class StorageTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void invalidRecordsAreQuarantined();
};

void StorageTest::invalidRecordsAreQuarantined() {
    QStandardPaths::setTestModeEnabled(true);
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directory);
    QFile::remove(directory + u"/library.json"_s);
    QFile::remove(directory + u"/library.json.invalid"_s);

    const Game valid = Game::create(u"Valid"_s, u"/tmp/valid.exe"_s, u"/tmp/prefix"_s);
    QJsonArray records;
    records.append(valid.toJson());
    records.append(QJsonObject{{u"id"_s, u"missing-title"_s}, {u"exePath"_s, u"/tmp/invalid.exe"_s}});
    records.append(valid.toJson());

    QFile library(directory + u"/library.json"_s);
    QVERIFY(library.open(QIODevice::WriteOnly));
    QVERIFY(library.write(QJsonDocument(records).toJson()) > 0);
    library.close();

    Storage storage;
    const QList<Game> games = storage.loadLibrary();
    QCOMPARE(games.size(), 1);
    QCOMPARE(games.first().id, valid.id);

    QFile quarantine(directory + u"/library.json.invalid"_s);
    QVERIFY(quarantine.open(QIODevice::ReadOnly));
    QJsonParseError error;
    const QJsonDocument quarantined = QJsonDocument::fromJson(quarantine.readAll(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(quarantined.isArray());
    QCOMPARE(quarantined.array().size(), 2);
}

QTEST_MAIN(StorageTest)
#include "storage_test.moc"