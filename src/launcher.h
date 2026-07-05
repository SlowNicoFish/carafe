#pragma once

#include "gamemodel.h"
#include "storage.h"
#include "settings.h"
#include "proton.h"
#include "steamgrid.h"

#include <QObject>
#include <QMap>
#include <QProcess>
#include <QUrl>

class Launcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GameModel *gameModel READ gameModel CONSTANT)
    Q_PROPERTY(QStringList protonBuilds READ protonBuilds NOTIFY protonBuildsChanged)
    Q_PROPERTY(QString defaultProton READ defaultProton NOTIFY defaultProtonChanged)
    Q_PROPERTY(QString steamgridApiKey READ steamgridApiKey NOTIFY steamgridApiKeyChanged)
    Q_PROPERTY(QString defaultLaunchArgs READ defaultLaunchArgs NOTIFY defaultLaunchArgsChanged)
    Q_PROPERTY(QString extraProtonPaths READ extraProtonPaths NOTIFY extraProtonPathsChanged)

public:
    explicit Launcher(QObject *parent = nullptr);
    ~Launcher() override;

    GameModel   *gameModel();
    QStringList  protonBuilds() const;
    QString      defaultProton() const;
    QString      steamgridApiKey() const;
    QString      defaultLaunchArgs() const;
    QString      extraProtonPaths() const;

    Q_INVOKABLE static QString urlToLocalFile(const QUrl &url);
    Q_INVOKABLE void loadLibrary();
    Q_INVOKABLE bool saveLibrary() const;
    Q_INVOKABLE void reloadProtonBuilds();
    Q_INVOKABLE QString suggestPrefix(const QString &title) const;
    Q_INVOKABLE bool addGame(const QString &title,
                             const QString &exePath,
                             const QString &prefixPath,
                             const QString &protonVersion,
                             const QString &umuId,
                             const QString &gridPath = {},
                             const QString &iconPath = {});
    Q_INVOKABLE bool updateGame(const QString &gameId, const QVariantMap &fields);
    Q_INVOKABLE bool removeGame(const QString &gameId, bool removePrefix = false);
    Q_INVOKABLE QVariantMap gameById(const QString &gameId) const;
    Q_INVOKABLE bool launchGame(const QString &gameId);
    Q_INVOKABLE void fetchGrid(const QString &gameId, const QString &apiKey);
    Q_INVOKABLE void fetchIcon(const QString &gameId, const QString &apiKey);
    Q_INVOKABLE void fetchGridArtwork(const QString &gameName);
    Q_INVOKABLE void fetchIconArtwork(const QString &gameName);
    Q_INVOKABLE bool saveSettings(const QVariantMap &settings);
    Q_INVOKABLE void runInstaller(const QString &installerPath,
                                   const QString &prefixPath,
                                   const QString &protonVersion);
    Q_INVOKABLE void runExeInPrefix(const QString &gameId, const QString &exePath);

Q_SIGNALS:
    void protonBuildsChanged();
    void defaultProtonChanged();
    void steamgridApiKeyChanged();
    void defaultLaunchArgsChanged();
    void extraProtonPathsChanged();
    void toastMessage(const QString &message);
    void gridPreviewReady(const QString &gameName, const QString &path);
    void iconPreviewReady(const QString &gameName, const QString &path);
    void gameLaunchFailed(const QString &gameId, const QString &reason);
    void installerStarted();
    void installerFinished(bool success, const QString &message);
    void runExeInPrefixFinished(bool success, const QString &message);

private:
    void    setSettings(const AppSettings &settings);
    QString resolveProtonPath(const QString &versionName) const;
    void    triggerIconExtraction(const QUuid &gameId, const QString &exePath);

    GameModel           m_gameModel;
    Storage             m_storage;
    SettingsStore       m_settingsStore;
    SteamGrid           m_steamGrid;
    AppSettings         m_settings;
    QStringList         m_protonBuilds;
    QList<ProtonBuild>  m_discoveredProtonBuilds;
    QMap<QUuid, QProcess*>  m_runningGames;
    QMap<QUuid, QString>    m_previewRequests;
};
