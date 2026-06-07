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
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)

public:
    explicit Launcher(QObject *parent = nullptr);
    ~Launcher() override;

    GameModel   *gameModel();
    QStringList  protonBuilds() const;
    QVariantMap  settings() const;

    Q_INVOKABLE static QString urlToLocalFile(const QUrl &url);
    Q_INVOKABLE void loadLibrary();
    Q_INVOKABLE bool saveLibrary() const;
    Q_INVOKABLE void reloadProtonBuilds();
    Q_INVOKABLE QString suggestPrefix(const QString &title) const;
    Q_INVOKABLE bool addGame(const QString &title,
                             const QString &exePath,
                             const QString &prefixPath,
                             const QString &protonVersion,
                             const QString &umuId);
    Q_INVOKABLE bool updateGame(const QString &gameId, const QVariantMap &fields);
    Q_INVOKABLE bool removeGame(const QString &gameId, bool removePrefix = false);
    Q_INVOKABLE QVariantMap gameById(const QString &gameId) const;
    Q_INVOKABLE bool launchGame(const QString &gameId);
    Q_INVOKABLE void fetchGrid(const QString &gameId, const QString &apiKey);
    Q_INVOKABLE void fetchIcon(const QString &gameId, const QString &apiKey);
    Q_INVOKABLE bool saveSettings(const QVariantMap &settings);
    Q_INVOKABLE void runInstaller(const QString &installerPath,
                                   const QString &prefixPath,
                                   const QString &protonVersion);

Q_SIGNALS:
    void protonBuildsChanged();
    void settingsChanged();
    void toastMessage(const QString &message);
    void gameLaunchFailed(const QString &gameId, const QString &reason);
    void installerStarted();
    void installerFinished(bool success, const QString &message);

private:
    void    setSettings(const AppSettings &settings);
    QString resolveProtonPath(const QString &versionName) const;
    void    triggerIconExtraction(const QUuid &gameId, const QString &exePath);

    GameModel          m_gameModel;
    Storage            m_storage;
    SettingsStore      m_settingsStore;
    SteamGrid          m_steamGrid;
    AppSettings        m_settings;
    QStringList        m_protonBuilds;
    QList<ProtonBuild> m_discoveredProtonBuilds;
    QMap<QUuid, QProcess*> m_runningGames;
};
