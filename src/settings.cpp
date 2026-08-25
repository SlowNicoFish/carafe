#include "settings.h"

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
    : QObject(parent)
{}

QString SettingsStore::settingsPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + u"/settings.json"_s;
}

#ifdef HAVE_KWALLET

static const QString walletFolder()
{
    return u"Carafe"_s;
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
    if (wallet->readPassword(u"steamgridApiKey"_s, value) != 0)
        return {};
    return value;
}

static void writeKeyToWallet(KWallet::Wallet *wallet, const QString &key)
{
    if (!wallet->hasFolder(walletFolder()) && !wallet->createFolder(walletFolder()))
        return;
    wallet->setFolder(walletFolder());
    if (key.isEmpty())
        wallet->removeEntry(u"steamgridApiKey"_s);
    else
        wallet->writePassword(u"steamgridApiKey"_s, key);
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
    s.defaultProton = obj[u"defaultProton"_s].toString();
    s.defaultLaunchArgs = obj[u"defaultLaunchArgs"_s].toString();
    s.defaultWrapperCommand = obj[u"defaultWrapperCommand"_s].toString();
    // Kept as the fallback for loadApiKey() when no keyring is available.
    s.steamgridApiKey = obj[u"steamgridApiKey"_s].toString();

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
    obj[u"defaultProton"_s] = s.defaultProton;
    obj[u"defaultLaunchArgs"_s] = s.defaultLaunchArgs;
    obj[u"defaultWrapperCommand"_s] = s.defaultWrapperCommand;

#ifdef HAVE_KWALLET
    KWallet::Wallet *wallet = openWallet();
    if (wallet) {
        writeKeyToWallet(wallet, s.steamgridApiKey);
        obj[u"hasSteamgridApiKey"_s] = !s.steamgridApiKey.isEmpty();
        delete wallet;
    } else {
        obj[u"steamgridApiKey"_s] = s.steamgridApiKey;
    }
#else
    obj[u"steamgridApiKey"_s] = s.steamgridApiKey;
#endif

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    f.write(QJsonDocument(obj).toJson());
    return f.commit();
}
