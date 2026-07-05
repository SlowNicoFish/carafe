#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QUuid>

class QNetworkReply;

/**
 * Fetches grid art and icons from SteamGridDB.
 */
class SteamGrid : public QObject
{
    Q_OBJECT

public:
    explicit SteamGrid(QObject *parent = nullptr);

    void fetchGrid(const QString &gameName, const QUuid &gameId, const QString &apiKey);
    void fetchIcon(const QString &gameName, const QUuid &gameId, const QString &apiKey);

Q_SIGNALS:
    void gridFetched(const QUuid &gameId, const QString &path);
    void iconFetched(const QUuid &gameId, const QString &path);
    void gridError(const QUuid &gameId, const QString &error);
    void iconError(const QUuid &gameId, const QString &error);

private:
    void searchGame(const QString &endpoint,
                    const QString &gameName,
                    const QUuid   &gameId,
                    const QString &apiKey,
                    const QString &suffix,
                    bool           isIcon);
    void onSearchReply(QNetworkReply   *reply,
                       const QString   &endpoint,
                       const QUuid     &gameId,
                       const QString   &apiKey,
                       const QString   &suffix,
                       bool             isIcon,
                       const QString   &gameName);
    void fetchAssetList(const QString &endpoint,
                        int            steamId,
                        const QUuid   &gameId,
                        const QString &apiKey,
                        const QString &suffix,
                        bool           isIcon,
                        const QString &gameName);
    void onAssetListReply(QNetworkReply *reply,
                          const QUuid   &gameId,
                          const QString &suffix,
                          bool           isIcon,
                          const QString &gameName);
    void downloadAsset(const QString  &imageUrl,
                       const QUuid    &gameId,
                       const QString  &suffix,
                       bool            isIcon);
    void onImageReply(QNetworkReply *reply,
                      const QUuid   &gameId,
                      const QString &suffix,
                      bool           isIcon);

    static void setCommonRequestAttrs(QNetworkRequest &req, const QString &apiKey);
    static QString assetDir();

    QNetworkAccessManager *m_nam;
};
