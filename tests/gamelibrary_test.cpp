#include "gamelibrary.h"
#include "storage.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

class GameLibraryTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void init();
    void updatesAndPersistsGame();
    void rejectsInvalidUpdate();
    void rejectsUnsafeImportedImagePath();

private:
    QString libraryPath() const;
};

QString GameLibraryTest::libraryPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/library.json"_s;
}

void GameLibraryTest::init() {
    QStandardPaths::setTestModeEnabled(true);
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(directory).removeRecursively();
    QVERIFY(QDir().mkpath(directory));
}

void GameLibraryTest::updatesAndPersistsGame() {
    GameLibrary library;
    const Game original = Game::create(u"Original"_s, u"/tmp/original.exe"_s, u"/tmp/prefix"_s);
    QVERIFY(library.addGame(original));

    Game updated = original;
    updated.title = u"Updated"_s;
    updated.launchArgs = u"--fullscreen"_s;
    QVERIFY(library.updateGame(updated));

    QCOMPARE(library.model()->gameById(original.id).title, u"Updated"_s);
    QCOMPARE(library.model()->gameById(original.id).launchArgs, u"--fullscreen"_s);

    Storage storage;
    const QList<Game> savedGames = storage.loadLibrary();
    QCOMPARE(savedGames.size(), 1);
    QCOMPARE(savedGames.first().id, original.id);
    QCOMPARE(savedGames.first().title, u"Updated"_s);
    QCOMPARE(savedGames.first().launchArgs, u"--fullscreen"_s);
    QVERIFY(QFileInfo::exists(libraryPath()));
}

void GameLibraryTest::rejectsInvalidUpdate() {
    GameLibrary library;
    const Game original = Game::create(u"Original"_s, u"/tmp/original.exe"_s, u"/tmp/prefix"_s);
    QVERIFY(library.addGame(original));

    Game invalid = original;
    invalid.title.clear();

    QVERIFY(!library.updateGame(invalid));
    QCOMPARE(library.model()->gameById(original.id).title, original.title);
}

void GameLibraryTest::rejectsUnsafeImportedImagePath() {
    QTemporaryDir sourceDir;
    QVERIFY(sourceDir.isValid());
    const QString sourcePath = sourceDir.filePath(u"cover.png"_s);
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QVERIFY(source.write("image") > 0);
    source.close();

    GameLibrary library;
    QVERIFY(library.importImage(sourcePath, u"../outside"_s, u"grid"_s).isEmpty());
}

QTEST_MAIN(GameLibraryTest)
#include "gamelibrary_test.moc"
