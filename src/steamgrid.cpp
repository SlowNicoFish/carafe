#include "steamgrid.h"

#include <QDir>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrlQuery>

static constexpr auto API_BASE = "https://www.steamgriddb.com/api/v2";

SteamGrid::SteamGrid(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

QString SteamGrid::assetDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/icons");
}

void SteamGrid::setCommonRequestAttrs(QNetworkRequest &req, const QString &apiKey)
{
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(apiKey).toUtf8());
    req.setTransferTimeout(15000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
}

void SteamGrid::fetchGrid(const QString &gameName, const QUuid &gameId, const QString &apiKey)
{
    searchGame(QStringLiteral("grids"), gameName, gameId, apiKey,
               QStringLiteral("grid"), false);
}

void SteamGrid::fetchIcon(const QString &gameName, const QUuid &gameId, const QString &apiKey)
{
    searchGame(QStringLiteral("icons"), gameName, gameId, apiKey,
               QStringLiteral("icon"), true);
}

void SteamGrid::searchGame(const QString &endpoint,
                            const QString &gameName,
                            const QUuid   &gameId,
                            const QString &apiKey,
                            const QString &suffix,
                            bool           isIcon)
{
    QUrl url(QString::fromLatin1(API_BASE) +
             QStringLiteral("/search/autocomplete/") +
             QString::fromUtf8(QUrl::toPercentEncoding(gameName)));

    QNetworkRequest req(url);
    setCommonRequestAttrs(req, apiKey);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, endpoint, gameId, apiKey, suffix, isIcon, gameName]() {
        onSearchReply(reply, endpoint, gameId, apiKey, suffix, isIcon, gameName);
    });
}

void SteamGrid::onSearchReply(QNetworkReply   *reply,
                               const QString   &endpoint,
                               const QUuid     &gameId,
                               const QString   &apiKey,
                               const QString   &suffix,
                               bool             isIcon,
                               const QString   &gameName)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = QStringLiteral("Search failed: %1").arg(reply->errorString());
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QJsonArray data = doc[QStringLiteral("data")].toArray();

    if (data.isEmpty()) {
        const QString err = QStringLiteral("No games found for '%1'").arg(gameName);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    int steamId = -1;
    const QString nameLower = gameName.trimmed().toLower();
    for (const QJsonValue &v : data) {
        if (v[QStringLiteral("name")].toString().toLower() == nameLower) {
            steamId = v[QStringLiteral("id")].toInt();
            break;
        }
    }
    if (steamId < 0)
        steamId = data.first()[QStringLiteral("id")].toInt();

    fetchAssetList(endpoint, steamId, gameId, apiKey, suffix, isIcon, gameName);
}

void SteamGrid::fetchAssetList(const QString &endpoint,
                                int            steamId,
                                const QUuid   &gameId,
                                const QString &apiKey,
                                const QString &suffix,
                                bool           isIcon,
                                const QString &gameName)
{
    QUrl url(QString::fromLatin1(API_BASE) +
             QStringLiteral("/%1/game/%2").arg(endpoint).arg(steamId));

    QNetworkRequest req(url);
    setCommonRequestAttrs(req, apiKey);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, gameId, suffix, isIcon, gameName]() {
        onAssetListReply(reply, gameId, suffix, isIcon, gameName);
    });
}

void SteamGrid::onAssetListReply(QNetworkReply *reply,
                                  const QUuid   &gameId,
                                  const QString &suffix,
                                  bool           isIcon,
                                  const QString &gameName)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = QStringLiteral("Asset request failed: %1")
                                .arg(reply->errorString());
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    const QJsonDocument doc  = QJsonDocument::fromJson(reply->readAll());
    const QJsonArray    data = doc[QStringLiteral("data")].toArray();

    if (data.isEmpty()) {
        const QString err = QStringLiteral("No %1 images found for '%2'")
                                .arg(suffix, gameName);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    const QString imageUrl = data.first()[QStringLiteral("url")].toString();
    downloadAsset(imageUrl, gameId, suffix, isIcon);
}

void SteamGrid::downloadAsset(const QString  &imageUrl,
                               const QUuid    &gameId,
                               const QString  &suffix,
                               bool            isIcon)
{
    QNetworkRequest req{QUrl(imageUrl)};
    req.setTransferTimeout(30000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, gameId, suffix, isIcon]() {
        onImageReply(reply, gameId, suffix, isIcon);
    });
}

void SteamGrid::onImageReply(QNetworkReply *reply,
                              const QUuid   &gameId,
                              const QString &suffix,
                              bool           isIcon)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = QStringLiteral("Image download failed: %1")
                                .arg(reply->errorString());
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    constexpr qint64 maxImageBytes = 20 * 1024 * 1024;
    if (reply->size() > maxImageBytes) {
        const QString err = QStringLiteral("Image response too large (%1 bytes)")
                                .arg(reply->size());
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    const QByteArray data = reply->readAll();
    const QString dir = assetDir();
    QDir().mkpath(dir);

    const QString path = dir + QStringLiteral("/%1_%2.png")
                             .arg(gameId.toString(QUuid::WithoutBraces), suffix);

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        const QString err = QStringLiteral("Failed to save image to %1").arg(path);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }
    f.write(data);
    f.commit();

    isIcon ? Q_EMIT iconFetched(gameId, path) : Q_EMIT gridFetched(gameId, path);
}
