#include "settings.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

using namespace Qt::Literals::StringLiterals;

#ifdef HAVE_KWALLET
#include <KWallet/KWallet>
#endif

SettingsStore::SettingsStore(QObject *parent)
    : QObject(parent) {}

QString SettingsStore::settingsPath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + u"/settings.json"_s;
}

static void quarantineSettingsFile(const QString &path) {
    if (!QFile::exists(path))
        return;

    const QString timestamp = QDateTime::currentDateTime().toString(u"yyyyMMdd-HHmmss-zzz"_s);
    QString backup = path + u".corrupt-%1"_s.arg(timestamp);
    int suffix = 2;
    while (QFile::exists(backup))
        backup = path + u".corrupt-%1-%2"_s.arg(timestamp).arg(suffix++);

    if (!QFile::rename(path, backup))
        qWarning() << "Could not quarantine corrupt settings file:" << path;
    else
        qWarning() << "Quarantined corrupt settings file to" << backup;
}

#ifdef HAVE_KWALLET

static const QString walletFolder() {
    return u"Carafe"_s;
}

static KWallet::Wallet *openWallet() {
    return KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), 0);
}

static bool hasWallet() {
    return KWallet::Wallet::isEnabled();
}

static QString readKeyFromWallet(KWallet::Wallet *wallet) {
    if (!wallet->hasFolder(walletFolder()) || !wallet->setFolder(walletFolder()))
        return {};
    QString value;
    if (wallet->readPassword(u"steamgridApiKey"_s, value) != 0)
        return {};
    return value;
}

static bool writeKeyToWallet(KWallet::Wallet *wallet, const QString &key) {
    if (key.isEmpty()) {
        if (!wallet->hasFolder(walletFolder()))
            return true;
        if (!wallet->setFolder(walletFolder()))
            return false;
        if (!wallet->hasEntry(u"steamgridApiKey"_s))
            return true;
        return wallet->removeEntry(u"steamgridApiKey"_s) == 0;
    }

    if (!wallet->hasFolder(walletFolder()) && !wallet->createFolder(walletFolder()))
        return false;
    if (!wallet->setFolder(walletFolder()))
        return false;
    return wallet->writePassword(u"steamgridApiKey"_s, key) == 0;
}

static bool saveApiKeyToWallet(const QString &key) {
    if (!hasWallet())
        return false;
    KWallet::Wallet *wallet = openWallet();
    if (!wallet)
        return false;
    const bool saved = writeKeyToWallet(wallet, key);
    delete wallet;
    return saved;
}

#endif

bool SettingsStore::keyringAvailable() {
#ifdef HAVE_KWALLET
    return hasWallet();
#else
    return false;
#endif
}

AppSettings SettingsStore::loadBasic() {
    QFile f(settingsPath());
    QJsonObject obj;
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            if (parseError.error != QJsonParseError::NoError)
                qWarning() << "Failed to parse settings file:" << parseError.errorString();
            else
                qWarning() << "Settings file does not contain a JSON object.";
            f.close();
            quarantineSettingsFile(settingsPath());
            return AppSettings::defaults();
        }
        obj = doc.object();
    }

    const bool hasLegacyKey = obj.contains(u"steamgridApiKey"_s);
    QString legacyKey = obj.take(u"steamgridApiKey"_s).toString();
    if (hasLegacyKey) {
        if (!writeSettingsFile(obj)) {
            qWarning() << "Could not remove the legacy SteamGridDB key from settings; removing the old file.";
            QFile::remove(settingsPath());
        }
#ifdef HAVE_KWALLET
        if (!legacyKey.isEmpty() && !saveApiKeyToWallet(legacyKey))
            qWarning() << "Could not migrate the legacy SteamGridDB key to KWallet.";
#else
        if (!legacyKey.isEmpty())
            qWarning() << "Discarding a legacy SteamGridDB key because KWallet support is unavailable.";
#endif
        legacyKey.fill(QChar{});
    }

    AppSettings s;
    s.defaultProton = obj[u"defaultProton"_s].toString();
    s.defaultLaunchArgs = obj[u"defaultLaunchArgs"_s].toString();
    s.defaultWrapperCommand = obj[u"defaultWrapperCommand"_s].toString();

    return s;
}

QString SettingsStore::loadApiKey() {
#ifdef HAVE_KWALLET
    if (hasWallet()) {
        KWallet::Wallet *wallet = openWallet();
        if (wallet) {
            const QString key = readKeyFromWallet(wallet);
            delete wallet;
            if (!key.isEmpty())
                return key;
        }
    }
#endif
    return {};
}

bool SettingsStore::hasApiKey() {
#ifdef HAVE_KWALLET
    if (!hasWallet())
        return false;
    KWallet::Wallet *wallet = openWallet();
    if (!wallet)
        return false;
    const bool found = wallet->hasFolder(walletFolder()) && wallet->setFolder(walletFolder()) &&
                       wallet->hasEntry(u"steamgridApiKey"_s);
    delete wallet;
    return found;
#else
    return false;
#endif
}

bool SettingsStore::writeSettingsFile(const QJsonObject &obj) {
    const QString path = settingsPath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return false;

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    const QByteArray data = QJsonDocument(obj).toJson();
    if (f.write(data) != data.size())
        return false;
    return f.commit();
}

bool SettingsStore::save(const AppSettings &s) const {
#ifdef HAVE_KWALLET
    if (!s.steamgridApiKey.isEmpty() || s.removeSteamgridApiKey) {
        if (!hasWallet())
            return false;
        KWallet::Wallet *wallet = openWallet();
        if (!wallet)
            return false;
        const bool keySaved = writeKeyToWallet(wallet, s.removeSteamgridApiKey ? QString{} : s.steamgridApiKey);
        delete wallet;
        if (!keySaved)
            return false;
    }
#else
    if (!s.steamgridApiKey.isEmpty() || s.removeSteamgridApiKey)
        return false;
#endif

    QJsonObject obj;
    obj[u"defaultProton"_s] = s.defaultProton;
    obj[u"defaultLaunchArgs"_s] = s.defaultLaunchArgs;
    obj[u"defaultWrapperCommand"_s] = s.defaultWrapperCommand;
    return writeSettingsFile(obj);
}
