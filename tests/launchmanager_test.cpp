#include "launchmanager.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

class LaunchManagerTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void hostWrapperReceivesUmuCommandAndArguments();
};

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

QTEST_MAIN(LaunchManagerTest)
#include "launchmanager_test.moc"