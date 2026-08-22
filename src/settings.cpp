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

AppSettings SettingsStore::loadBasic()
{
    QFile f(settingsPath());
    QJsonObject obj;
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
            qWarning() << "Failed to parse settings file:" << parseError.errorString();
        else if (doc.isObject())
            obj = doc.object();
    }

    AppSettings s;
    s.defaultProton         = obj[QStringLiteral("defaultProton")].toString();
    s.defaultLaunchArgs     = obj[QStringLiteral("defaultLaunchArgs")].toString();
    s.defaultWrapperCommand = obj[QStringLiteral("defaultWrapperCommand")].toString();
    // Kept as the fallback for loadApiKey() when no keyring is available.
    s.steamgridApiKey       = obj[QStringLiteral("steamgridApiKey")].toString();

    return s;
}

QString SettingsStore::loadApiKey(const QString &jsonFallback)
{
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
    return jsonFallback;
}

bool SettingsStore::save(const AppSettings &s) const
{
    const QString path = settingsPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject obj;
    obj[QStringLiteral("defaultProton")]         = s.defaultProton;
    obj[QStringLiteral("defaultLaunchArgs")]     = s.defaultLaunchArgs;
    obj[QStringLiteral("defaultWrapperCommand")] = s.defaultWrapperCommand;

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
