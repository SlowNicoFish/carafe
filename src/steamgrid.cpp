#include "steamgrid.h"

#include <QDir>
#include <QFile>
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

void SteamGrid::fetchGrid(const QString &gameName, const QUuid &gameId, const QString &apiKey)
{
    fetchAsset(QStringLiteral("grids"), gameName, gameId, apiKey,
               QStringLiteral("grid"), false);
}

void SteamGrid::fetchIcon(const QString &gameName, const QUuid &gameId, const QString &apiKey)
{
    fetchAsset(QStringLiteral("icons"), gameName, gameId, apiKey,
               QStringLiteral("icon"), true);
}

void SteamGrid::fetchAsset(const QString &endpoint,
                            const QString &gameName,
                            const QUuid   &gameId,
                            const QString &apiKey,
                            const QString &suffix,
                            bool           isIcon)
{
    // Step 1: search for the game
    QUrl searchUrl(QString::fromLatin1(API_BASE) +
                   QStringLiteral("/search/autocomplete/") +
                   QString::fromUtf8(QUrl::toPercentEncoding(gameName)));

    QNetworkRequest req(searchUrl);
    req.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(apiKey).toUtf8());
    req.setTransferTimeout(15000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *searchReply = m_nam->get(req);

    connect(searchReply, &QNetworkReply::finished, this,
            [this, searchReply, endpoint, gameName, gameId, apiKey, suffix, isIcon]() {
        searchReply->deleteLater();

        if (searchReply->error() != QNetworkReply::NoError) {
            const QString err = QStringLiteral("Search failed: %1").arg(searchReply->errorString());
            isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(searchReply->readAll());
        const QJsonArray    data = doc[QStringLiteral("data")].toArray();

        if (data.isEmpty()) {
            const QString err = QStringLiteral("No games found for '%1'").arg(gameName);
            isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
            return;
        }

        // Prefer exact name match, fall back to first result
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

        // Step 2: fetch asset list
        QUrl assetUrl(QString::fromLatin1(API_BASE) +
                      QStringLiteral("/%1/game/%2").arg(endpoint).arg(steamId));

        QNetworkRequest assetReq(assetUrl);
        assetReq.setRawHeader("Authorization",
                              QStringLiteral("Bearer %1").arg(apiKey).toUtf8());
        assetReq.setTransferTimeout(15000);
        assetReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                              QNetworkRequest::NoLessSafeRedirectPolicy);

        QNetworkReply *assetReply = m_nam->get(assetReq);
        connect(assetReply, &QNetworkReply::finished, this,
                [this, assetReply, gameId, suffix, isIcon, gameName]() {
            assetReply->deleteLater();

            if (assetReply->error() != QNetworkReply::NoError) {
                const QString err = QStringLiteral("Asset request failed: %1")
                                        .arg(assetReply->errorString());
                isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
                return;
            }

            const QJsonDocument doc  = QJsonDocument::fromJson(assetReply->readAll());
            const QJsonArray    data = doc[QStringLiteral("data")].toArray();

            if (data.isEmpty()) {
                const QString err = QStringLiteral("No %1 images found for '%2'")
                                        .arg(suffix, gameName);
                isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
                return;
            }

            const QString imageUrl = data.first()[QStringLiteral("url")].toString();

            // Step 3: download the image
            QNetworkRequest imgReq{QUrl(imageUrl)};
            imgReq.setTransferTimeout(30000);
            imgReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                 QNetworkRequest::NoLessSafeRedirectPolicy);
            QNetworkReply *imgReply = m_nam->get(imgReq);
            connect(imgReply, &QNetworkReply::finished, this,
                    [this, imgReply, gameId, suffix, isIcon]() {
                imgReply->deleteLater();

                if (imgReply->error() != QNetworkReply::NoError) {
                    const QString err = QStringLiteral("Image download failed: %1")
                                            .arg(imgReply->errorString());
                    isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
                    return;
                }

                // Limit image size to 20 MB to avoid memory exhaustion
                constexpr qint64 maxImageBytes = 20 * 1024 * 1024;
                if (imgReply->size() > maxImageBytes) {
                    const QString err = QStringLiteral("Image response too large (%1 bytes)")
                                            .arg(imgReply->size());
                    isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
                    return;
                }

                const QByteArray data = imgReply->readAll();
                const QString dir = assetDir();
                QDir().mkpath(dir);

                const QString path = dir + QStringLiteral("/%1_%2.png")
                                         .arg(gameId.toString(QUuid::WithoutBraces), suffix);

                QFile f(path);
                if (!f.open(QIODevice::WriteOnly)) {
                    const QString err = QStringLiteral("Failed to save image to %1").arg(path);
                    isIcon ? Q_EMIT iconError(gameId, err) : Q_EMIT gridError(gameId, err);
                    return;
                }
                f.write(data);
                f.close();

                isIcon ? Q_EMIT iconFetched(gameId, path) : Q_EMIT gridFetched(gameId, path);
            });
        });
    });
}
