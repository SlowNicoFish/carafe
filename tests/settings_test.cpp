#include "settings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

class SettingsTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void init();
    void malformedSettingsAreQuarantined();
    void nonObjectSettingsAreQuarantined();
    void legacyApiKeyIsRemovedFromSettingsJson();

private:
    QString settingsDirectory() const;
    QString settingsPath() const;
    QStringList quarantinedFiles() const;
};

QString SettingsTest::settingsDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString SettingsTest::settingsPath() const {
    return settingsDirectory() + u"/settings.json"_s;
}

QStringList SettingsTest::quarantinedFiles() const {
    return QDir(settingsDirectory()).entryList({u"settings.json.corrupt-*"_s}, QDir::Files);
}

void SettingsTest::init() {
    QStandardPaths::setTestModeEnabled(true);
    QDir(settingsDirectory()).removeRecursively();
    QVERIFY(QDir().mkpath(settingsDirectory()));
}

void SettingsTest::malformedSettingsAreQuarantined() {
    const QByteArray original = "{ invalid json";
    QFile settings(settingsPath());
    QVERIFY(settings.open(QIODevice::WriteOnly));
    QCOMPARE(settings.write(original), original.size());
    settings.close();

    SettingsStore store;
    const AppSettings loaded = SettingsStore::loadBasic();
    QVERIFY(loaded.defaultProton.isEmpty());

    const QStringList backups = quarantinedFiles();
    QCOMPARE(backups.size(), 1);
    QFile backup(settingsDirectory() + u"/"_s + backups.first());
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), original);
    backup.close();

    AppSettings updated;
    updated.defaultLaunchArgs = u"--windowed"_s;
    QVERIFY(store.save(updated));
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), original);
}

void SettingsTest::nonObjectSettingsAreQuarantined() {
    const QByteArray original = "[]";
    QFile settings(settingsPath());
    QVERIFY(settings.open(QIODevice::WriteOnly));
    QCOMPARE(settings.write(original), original.size());
    settings.close();

    SettingsStore::loadBasic();

    const QStringList backups = quarantinedFiles();
    QCOMPARE(backups.size(), 1);
    QFile backup(settingsDirectory() + u"/"_s + backups.first());
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), original);
}

void SettingsTest::legacyApiKeyIsRemovedFromSettingsJson() {
    QStandardPaths::setTestModeEnabled(true);
    QDir(settingsDirectory()).removeRecursively();
    QVERIFY(QDir().mkpath(settingsDirectory()));

    QJsonObject obj;
    obj[u"defaultProton"_s] = u"GE-Proton"_s;
    obj[u"steamgridApiKey"_s] = u"legacy-secret-key"_s;

    QFile settings(settingsPath());
    QVERIFY(settings.open(QIODevice::WriteOnly));
    settings.write(QJsonDocument(obj).toJson());
    settings.close();

    const AppSettings loaded = SettingsStore::loadBasic();
    QCOMPARE(loaded.defaultProton, u"GE-Proton"_s);
    QVERIFY(loaded.steamgridApiKey.isEmpty());

    QFile reloaded(settingsPath());
    QVERIFY(reloaded.open(QIODevice::ReadOnly));
    const QJsonObject savedObj = QJsonDocument::fromJson(reloaded.readAll()).object();
    QVERIFY(!savedObj.contains(u"steamgridApiKey"_s));
    QCOMPARE(savedObj.value(u"defaultProton"_s).toString(), u"GE-Proton"_s);
}

QTEST_MAIN(SettingsTest)
#include "settings_test.moc"
