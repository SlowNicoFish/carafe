#include "settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#ifdef HAVE_KWALLET
#include <KWallet/KWallet>
#endif

SettingsStore::SettingsStore(QObject *parent)
    : QObject(parent)
{}

QString SettingsStore::settingsPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/settings.json");
}

#ifdef HAVE_KWALLET

static const QString walletFolder()
{
    return QStringLiteral("Carafe");
}

static KWallet::Wallet *openWallet()
{
    return KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), 0);
}

static bool hasWallet()
{
    return KWallet::Wallet::isEnabled();
}

static QString readKeyFromWallet(KWallet::Wallet *wallet)
{
    if (!wallet->hasFolder(walletFolder()) && !wallet->createFolder(walletFolder()))
        return {};
    wallet->setFolder(walletFolder());
    QString value;
    if (wallet->readPassword(QStringLiteral("steamgridApiKey"), value) != 0)
        return {};
    return value;
}

static void writeKeyToWallet(KWallet::Wallet *wallet, const QString &key)
{
    if (!wallet->hasFolder(walletFolder()) && !wallet->createFolder(walletFolder()))
        return;
    wallet->setFolder(walletFolder());
    if (key.isEmpty())
        wallet->removeEntry(QStringLiteral("steamgridApiKey"));
    else
        wallet->writePassword(QStringLiteral("steamgridApiKey"), key);
}

#endif

bool SettingsStore::keyringAvailable()
{
#ifdef HAVE_KWALLET
    return hasWallet();
#else
    return false;
#endif
}

AppSettings SettingsStore::load()
{
    QFile f(settingsPath());
    QJsonObject obj;
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject())
            obj = doc.object();
    }

    AppSettings s;
    s.defaultProton     = obj[QStringLiteral("defaultProton")].toString();
    s.defaultLaunchArgs = obj[QStringLiteral("defaultLaunchArgs")].toString();
    s.extraProtonPaths  = obj[QStringLiteral("extraProtonPaths")].toString();

#ifdef HAVE_KWALLET
    KWallet::Wallet *wallet = openWallet();
    if (wallet) {
        s.steamgridApiKey = readKeyFromWallet(wallet);
        if (s.steamgridApiKey.isEmpty())
            s.steamgridApiKey = obj[QStringLiteral("steamgridApiKey")].toString();
        delete wallet;
    } else {
        s.steamgridApiKey = obj[QStringLiteral("steamgridApiKey")].toString();
    }
#else
    s.steamgridApiKey   = obj[QStringLiteral("steamgridApiKey")].toString();
#endif

    return s;
}

bool SettingsStore::save(const AppSettings &s) const
{
    const QString path = settingsPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject obj;
    obj[QStringLiteral("defaultProton")]     = s.defaultProton;
    obj[QStringLiteral("defaultLaunchArgs")] = s.defaultLaunchArgs;
    obj[QStringLiteral("extraProtonPaths")]  = s.extraProtonPaths;

#ifdef HAVE_KWALLET
    KWallet::Wallet *wallet = openWallet();
    if (wallet) {
        writeKeyToWallet(wallet, s.steamgridApiKey);
        obj[QStringLiteral("hasSteamgridApiKey")] = !s.steamgridApiKey.isEmpty();
        delete wallet;
    } else {
        obj[QStringLiteral("steamgridApiKey")] = s.steamgridApiKey;
    }
#else
    obj[QStringLiteral("steamgridApiKey")] = s.steamgridApiKey;
#endif

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    f.write(QJsonDocument(obj).toJson());
    return f.commit();
}
