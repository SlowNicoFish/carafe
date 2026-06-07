#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QUuid>

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
    void fetchAsset(const QString &endpoint,
                    const QString &gameName,
                    const QUuid   &gameId,
                    const QString &apiKey,
                    const QString &suffix,
                    bool           isIcon);

    static QString assetDir();

    QNetworkAccessManager *m_nam;
};
