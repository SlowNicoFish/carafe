#include "launchmanager.h"
#include "shellargs.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

class LaunchManagerTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void parsesEmptyQuotedArguments();
    void hostWrapperReceivesUmuCommandAndArguments();
    void failedToStartReportsErrorAndClearsTracking();
    void stopTracksProcessUntilExit();
    void terminateAllClearsTrackingForMultipleProcesses();
};

void LaunchManagerTest::parsesEmptyQuotedArguments() {
    QCOMPARE(parseShellArgs(u"--title \"\" 'two words'"_s), QStringList({u"--title"_s, u""_s, u"two words"_s}));
}

void LaunchManagerTest::hostWrapperReceivesUmuCommandAndArguments() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString wrapperPath = directory.filePath(u"wrapper.sh"_s);
    const QString outputPath = directory.filePath(u"arguments.txt"_s);
    QFile wrapper(wrapperPath);
    QVERIFY(wrapper.open(QIODevice::WriteOnly));
    QVERIFY(wrapper.write("#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$CARAFE_TEST_OUTPUT\"\n") > 0);
    wrapper.close();
    QVERIFY(wrapper.setPermissions(wrapper.permissions() | QFileDevice::ExeOwner));

    qputenv("CARAFE_TEST_OUTPUT", outputPath.toLocal8Bit());
    QStringList received;
    bool terminalOk = false;
    int terminalCount = 0;

    LaunchManager::Spec spec;
    spec.exePath = u"/games/My Game.exe"_s;
    spec.wrapperCommand = u"%1 --wrapper-option"_s.arg(wrapperPath);
    spec.args = {u"--name"_s, u"Player One"_s};

    LaunchManager manager;
    manager.start(
        spec, [] {},
        [&terminalOk, &terminalCount](bool ok, const QString &) {
            terminalOk = ok;
            ++terminalCount;
        });

    QTRY_COMPARE_WITH_TIMEOUT(terminalCount, 1, 3000);
    QVERIFY(terminalOk);

    QFile output(outputPath);
    QVERIFY(output.open(QIODevice::ReadOnly));
    while (!output.atEnd())
        received << QString::fromUtf8(output.readLine()).trimmed();

    QCOMPARE(received,
             QStringList({u"--wrapper-option"_s, u"umu-run"_s, u"/games/My Game.exe"_s, u"--name"_s, u"Player One"_s}));
}

void LaunchManagerTest::failedToStartReportsErrorAndClearsTracking() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LaunchManager::Spec spec;
    spec.gameUuid = QUuid::createUuid();
    spec.wrapperCommand = directory.filePath(u"missing-wrapper"_s);

    bool terminalOk = true;
    QString terminalMessage;
    int terminalCount = 0;

    LaunchManager manager;
    manager.start(
        spec, [] {},
        [&terminalOk, &terminalMessage, &terminalCount](bool ok, const QString &message) {
            terminalOk = ok;
            terminalMessage = message;
            ++terminalCount;
        });

    QTRY_COMPARE_WITH_TIMEOUT(terminalCount, 1, 3000);
    QVERIFY(!terminalOk);
    QVERIFY(terminalMessage.contains(u"Failed to start"_s));
    QVERIFY(!manager.isRunning(spec.gameUuid));
}

void LaunchManagerTest::stopTracksProcessUntilExit() {
    LaunchManager manager;
    LaunchManager::Spec spec;
    spec.gameUuid = QUuid::createUuid();
    spec.wrapperCommand = u"/bin/sh -c 'exec /bin/sleep 30'"_s;

    bool started = false;
    int terminalCount = 0;
    QSignalSpy stoppedSpy(&manager, &LaunchManager::processStopped);
    manager.start(spec, [&started] { started = true; }, [&terminalCount](bool, const QString &) { ++terminalCount; });

    QTRY_VERIFY_WITH_TIMEOUT(started, 3000);
    manager.stop(spec.gameUuid);
    QVERIFY(manager.isRunning(spec.gameUuid));

    QTRY_VERIFY_WITH_TIMEOUT(!manager.isRunning(spec.gameUuid), 3000);
    QCOMPARE(terminalCount, 0);
    QCOMPARE(stoppedSpy.count(), 1);
}

void LaunchManagerTest::terminateAllClearsTrackingForMultipleProcesses() {
    LaunchManager manager;
    QList<QUuid> gameIds;
    int startedCount = 0;

    for (int i = 0; i < 2; ++i) {
        LaunchManager::Spec spec;
        spec.gameUuid = QUuid::createUuid();
        spec.wrapperCommand = u"/bin/sh -c 'exec /bin/sleep 30'"_s;
        gameIds.append(spec.gameUuid);
        manager.start(spec, [&startedCount] { ++startedCount; }, [](bool, const QString &) {});
    }

    QTRY_COMPARE_WITH_TIMEOUT(startedCount, 2, 3000);
    manager.terminateAll();

    for (const QUuid &gameId : gameIds)
        QVERIFY(!manager.isRunning(gameId));
}

QTEST_MAIN(LaunchManagerTest)
#include "launchmanager_test.moc"