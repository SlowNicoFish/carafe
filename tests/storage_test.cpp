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
    void savesAndLoadsLibrary();
    void invalidRecordsAreQuarantined();
};

void StorageTest::savesAndLoadsLibrary() {
    QStandardPaths::setTestModeEnabled(true);
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(directory).removeRecursively();

    Game original = Game::create(u"Saved game"_s, u"/tmp/game.exe"_s, u"/tmp/prefix"_s);
    original.launchArgs = u"--fullscreen"_s;
    original.wrapperCommand = u"gamescope --"_s;
    original.protonVersion = u"GE-Proton"_s;
    original.umuId = u"umu-game-id"_s;
    original.iconPath = u"/icons/game.png"_s;
    original.gridPath = u"/icons/game_grid.jpg"_s;
    original.steamgridIconPath = u"/icons/game_steamgrid.png"_s;
    Storage storage;
    QVERIFY(storage.saveLibrary({original}));

    const QList<Game> loaded = storage.loadLibrary();
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first().id, original.id);
    QCOMPARE(loaded.first().toJson(), original.toJson());
}

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