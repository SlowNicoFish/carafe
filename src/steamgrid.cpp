#include "steamgrid.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>
using namespace Qt::Literals::StringLiterals;

static constexpr auto API_BASE = "https://www.steamgriddb.com/api/v2";

static QStringList allowedImageExtensions() {
    return {u"png"_s, u"jpg"_s, u"jpeg"_s, u"webp"_s};
}

static QString describeReplyError(QNetworkReply *reply) {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status < 400)
        return reply->errorString();

    // Prefer the API's own JSON error message (e.g. invalid API key) when present.
    QString detail;
    if (reply->rawHeader("Content-Type").contains("json")) {
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString message = obj.value(u"message"_s).toString();
        if (!message.isEmpty())
            detail = u" — %1"_s.arg(message);
    }
    return u"HTTP %1%2"_s.arg(status).arg(detail);
}

SteamGrid::SteamGrid(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this)) {}

QString SteamGrid::assetDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/icons"_s;
}

void SteamGrid::setCommonRequestAttrs(QNetworkRequest &req, const QString &apiKey) {
    req.setRawHeader("Authorization", u"Bearer %1"_s.arg(apiKey).toUtf8());
    req.setTransferTimeout(15000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
}

void SteamGrid::fetchGrid(const QString &gameName, const QUuid &gameId, const QString &apiKey) {
    searchGame(u"grids"_s, gameName, gameId, apiKey, u"grid"_s, false);
}

void SteamGrid::fetchIcon(const QString &gameName, const QUuid &gameId, const QString &apiKey) {
    searchGame(u"icons"_s, gameName, gameId, apiKey, u"icon"_s, true);
}

void SteamGrid::searchGame(const QString &endpoint, const QString &gameName, const QUuid &gameId, const QString &apiKey,
                           const QString &suffix, bool isIcon) {
    QUrl url(QString::fromLatin1(API_BASE) + u"/search/autocomplete/"_s +
             QString::fromUtf8(QUrl::toPercentEncoding(gameName)));

    QNetworkRequest req(url);
    setCommonRequestAttrs(req, apiKey);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint, gameId, apiKey, suffix, isIcon, gameName]() {
        onSearchReply(reply, endpoint, gameId, apiKey, suffix, isIcon, gameName);
    });
}

void SteamGrid::onSearchReply(QNetworkReply *reply, const QString &endpoint, const QUuid &gameId, const QString &apiKey,
                              const QString &suffix, bool isIcon, const QString &gameName) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = u"Search failed: %1"_s.arg(describeReplyError(reply));
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject() || !doc.object()[u"data"_s].isArray()) {
        const QString err = u"Search returned an invalid response."_s;
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }
    const QJsonArray data = doc.object()[u"data"_s].toArray();

    if (data.isEmpty()) {
        const QString err = u"No games found for '%1'"_s.arg(gameName);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    int steamId = -1;
    const QString nameLower = gameName.trimmed().toLower();
    for (const QJsonValue &v : data) {
        if (v[u"name"_s].toString().toLower() == nameLower) {
            steamId = v[u"id"_s].toInt();
            break;
        }
    }
    if (steamId < 0)
        steamId = data.first()[u"id"_s].toInt();

    fetchAssetList(endpoint, steamId, gameId, apiKey, suffix, isIcon, gameName);
}

void SteamGrid::fetchAssetList(const QString &endpoint, int steamId, const QUuid &gameId, const QString &apiKey,
                               const QString &suffix, bool isIcon, const QString &gameName) {
    QUrl url(QString::fromLatin1(API_BASE) + u"/%1/game/%2"_s.arg(endpoint).arg(steamId));

    QNetworkRequest req(url);
    setCommonRequestAttrs(req, apiKey);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, gameId, suffix, isIcon, gameName]() {
        onAssetListReply(reply, gameId, suffix, isIcon, gameName);
    });
}

void SteamGrid::onAssetListReply(QNetworkReply *reply, const QUuid &gameId, const QString &suffix, bool isIcon,
                                 const QString &gameName) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = u"Asset request failed: %1"_s.arg(describeReplyError(reply));
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject() || !doc.object()[u"data"_s].isArray()) {
        const QString err = u"Asset request returned an invalid response."_s;
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }
    const QJsonArray data = doc.object()[u"data"_s].toArray();

    if (data.isEmpty()) {
        const QString err = u"No %1 images found for '%2'"_s.arg(suffix, gameName);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    const QString imageUrl = data.first()[u"url"_s].toString();
    downloadAsset(imageUrl, gameId, suffix, isIcon);
}

void SteamGrid::downloadAsset(const QString &imageUrl, const QUuid &gameId, const QString &suffix, bool isIcon) {
    QNetworkRequest req{QUrl(imageUrl)};
    req.setTransferTimeout(30000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, imageUrl, gameId, suffix, isIcon]() {
        onImageReply(reply, imageUrl, gameId, suffix, isIcon);
    });
}

void SteamGrid::onImageReply(QNetworkReply *reply, const QString &imageUrl, const QUuid &gameId, const QString &suffix,
                             bool isIcon) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = u"Image download failed: %1"_s.arg(describeReplyError(reply));
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    constexpr qint64 maxImageBytes = 20 * 1024 * 1024;
    if (reply->size() > maxImageBytes) {
        const QString err = u"Image response too large (%1 bytes)"_s.arg(reply->size());
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    const QByteArray data = reply->readAll();
    const QString dir = assetDir();
    QDir().mkpath(dir);

    QString extension = QFileInfo(QUrl(imageUrl).path()).suffix().toLower();
    if (!allowedImageExtensions().contains(extension))
        extension = u"png"_s;

    const QString path = dir + u"/%1_%2.%3"_s.arg(gameId.toString(QUuid::WithoutBraces), suffix, extension);

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        const QString err = u"Failed to save image to %1"_s.arg(path);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }
    if (f.write(data) != data.size()) {
        const QString err = u"Failed to write image to disk: %1"_s.arg(path);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }
    if (!f.commit()) {
        const QString err = u"Failed to write image to disk: %1"_s.arg(path);
        isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
        return;
    }

    isIcon ? Q_EMIT iconFetched(gameId, path) : Q_EMIT gridFetched(gameId, path);
}
