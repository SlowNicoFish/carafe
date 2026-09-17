#include "gamemodel.h"

#include <QSignalSpy>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

class GameModelTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void exposesGameRoles();
    void updatesRunningState();
    void ignoresUnchangedRunningState();
    void resetsRunningStateWithGames();
};

void GameModelTest::exposesGameRoles() {
    GameModel model;
    const Game game = Game::create(u"Test Game"_s, u"/tmp/test.exe"_s, u"/tmp/prefix"_s);
    model.setGames({game});

    const QModelIndex index = model.index(0, 0);
    const auto roles = model.roleNames();

    QCOMPARE(roles.size(), 13);
    QCOMPARE(model.data(index, GameModel::IdRole).toString(), game.id.toString(QUuid::WithoutBraces));
    QCOMPARE(model.data(index, GameModel::TitleRole).toString(), game.title);
    QCOMPARE(model.data(index, GameModel::ExePathRole).toString(), game.exePath);
    QCOMPARE(model.data(index, GameModel::PrefixPathRole).toString(), game.prefixPath);
    QCOMPARE(model.data(index, GameModel::IsRunningRole).toBool(), false);
    QCOMPARE(model.getById(game.id.toString(QUuid::WithoutBraces)).value(u"title"_s).toString(), game.title);
}

void GameModelTest::updatesRunningState() {
    GameModel model;
    const Game game = Game::create(u"Test Game"_s, u"/tmp/test.exe"_s, u"/tmp/prefix"_s);
    model.setGames({game});
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    model.setRunning(game.id, true);

    QCOMPARE(model.data(model.index(0, 0), GameModel::IsRunningRole).toBool(), true);
    QCOMPARE(changedSpy.count(), 1);
    const QList<QVariant> arguments = changedSpy.takeFirst();
    QVERIFY(arguments.at(2).value<QVector<int>>().contains(GameModel::IsRunningRole));
}

void GameModelTest::ignoresUnchangedRunningState() {
    GameModel model;
    const Game game = Game::create(u"Test Game"_s, u"/tmp/test.exe"_s, u"/tmp/prefix"_s);
    model.setGames({game});
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    model.setRunning(game.id, true);
    model.setRunning(game.id, true);

    QCOMPARE(changedSpy.count(), 1);
}

void GameModelTest::resetsRunningStateWithGames() {
    GameModel model;
    const Game game = Game::create(u"Test Game"_s, u"/tmp/test.exe"_s, u"/tmp/prefix"_s);
    model.setGames({game});
    model.setRunning(game.id, true);

    model.setGames({game});

    QCOMPARE(model.data(model.index(0, 0), GameModel::IsRunningRole).toBool(), false);
}

QTEST_MAIN(GameModelTest)
#include "gamemodel_test.moc"
