#pragma once

#include "gamelibrary.h"
#include "launchmanager.h"
#include "protonmanager.h"
#include "settings.h"
#include "steamgrid.h"

#include <QObject>
#include <QObjectBindableProperty>
#include <QQmlEngine>
#include <QMap>
#include <QUuid>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/**
 * QML-facing facade ("Backend") that wires together the game library, Proton
 * discovery, process launching, settings, and SteamGrid artwork fetching.
 *
 * Individual concerns live in GameLibrary, ProtonManager, and LaunchManager;
 * this class coordinates them and adapts results into the signals QML consumes.
 */
class Launcher : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Backend)
    QML_SINGLETON
    Q_PROPERTY(GameModel *gameModel READ gameModel CONSTANT)
    Q_PROPERTY(QStringList protonBuilds READ protonBuilds BINDABLE bindableProtonBuilds NOTIFY protonBuildsChanged)
    Q_PROPERTY(QString defaultProton READ defaultProton BINDABLE bindableDefaultProton NOTIFY defaultProtonChanged)
    Q_PROPERTY(
        QString steamgridApiKey READ steamgridApiKey BINDABLE bindableSteamgridApiKey NOTIFY steamgridApiKeyChanged)
    Q_PROPERTY(QString defaultLaunchArgs READ defaultLaunchArgs BINDABLE bindableDefaultLaunchArgs NOTIFY
                   defaultLaunchArgsChanged)
    Q_PROPERTY(QString defaultWrapperCommand READ defaultWrapperCommand BINDABLE bindableDefaultWrapperCommand NOTIFY
                   defaultWrapperCommandChanged)

public:
    explicit Launcher(QObject *parent = nullptr);
    ~Launcher() override;

    static Launcher &instance() {
        static Launcher inst;
        return inst;
    }

    static Launcher *create(QQmlEngine *, QJSEngine *) {
        auto *inst = &instance();
        QQmlEngine::setObjectOwnership(inst, QQmlEngine::CppOwnership);
        return inst;
    }

    GameModel *gameModel();
    QStringList protonBuilds() const;
    QString defaultProton() const;
    QString steamgridApiKey();
    QString defaultLaunchArgs() const;
    QString defaultWrapperCommand() const;

    QBindable<QStringList> bindableProtonBuilds() { return &m_protonBuilds; }
    QBindable<QString> bindableDefaultProton() { return &m_defaultProton; }
    QBindable<QString> bindableSteamgridApiKey() { return &m_steamgridApiKey; }
    QBindable<QString> bindableDefaultLaunchArgs() { return &m_defaultLaunchArgs; }
    QBindable<QString> bindableDefaultWrapperCommand() { return &m_defaultWrapperCommand; }

    Q_INVOKABLE static QString urlToLocalFile(const QUrl &url);
    Q_INVOKABLE static QUrl localFileToUrl(const QString &path);
    Q_INVOKABLE void loadLibrary();
    Q_INVOKABLE bool saveLibrary() const;
    Q_INVOKABLE void reloadProtonBuilds();
    Q_INVOKABLE QString suggestPrefix(const QString &title) const;
    Q_INVOKABLE bool addGame(const QString &title, const QString &exePath, const QString &prefixPath,
                             const QString &protonVersion, const QString &umuId, const QString &gridPath = {},
                             const QString &steamgridIconPath = {}, const QString &wrapperCommand = {});
    Q_INVOKABLE bool updateGame(const QString &gameId, const QVariantMap &fields);
    Q_INVOKABLE bool removeGame(const QString &gameId, bool removePrefix = false);
    Q_INVOKABLE QVariantMap gameById(const QString &gameId);
    Q_INVOKABLE bool launchGame(const QString &gameId);
    Q_INVOKABLE void fetchGrid(const QString &gameId, const QString &apiKey);
    Q_INVOKABLE void fetchIcon(const QString &gameId, const QString &apiKey);
    Q_INVOKABLE void fetchGridArtwork(const QString &gameName);
    Q_INVOKABLE void fetchIconArtwork(const QString &gameName);
    Q_INVOKABLE bool saveSettings(const QVariantMap &settings);
    Q_INVOKABLE void runInstaller(const QString &installerPath, const QString &prefixPath,
                                  const QString &protonVersion);
    Q_INVOKABLE void runExeInPrefix(const QString &gameId, const QString &exePath);
    Q_INVOKABLE QString importImage(const QString &sourcePath, const QString &gameId, const QString &suffix);

Q_SIGNALS:
    void protonBuildsChanged();
    void defaultProtonChanged();
    void steamgridApiKeyChanged();
    void defaultLaunchArgsChanged();
    void defaultWrapperCommandChanged();
    void toastMessage(const QString &message);
    void gridPreviewReady(const QString &gameName, const QString &path);
    void iconPreviewReady(const QString &gameName, const QString &path);
    void gridPreviewFailed(const QString &gameName, const QString &error);
    void iconPreviewFailed(const QString &gameName, const QString &error);
    void gameLaunchFailed(const QString &gameId, const QString &reason);
    void installerStarted();
    void installerFinished(bool success, const QString &message);
    void runExeInPrefixFinished(bool success, const QString &message);

private:
    void setSettings(const AppSettings &settings);
    void connectSteamGrid();
    void fetchArtwork(const QString &gameId, const QString &apiKey, bool icon);
    Game gameForId(const QString &gameId, QUuid *uuid) const;

    GameLibrary m_games;
    ProtonManager m_proton;
    LaunchManager m_launch;
    SettingsStore m_settingsStore;
    SteamGrid m_steamGrid;
    bool m_apiKeyLoaded = false;
    QMap<QUuid, QString> m_previewRequests;

    Q_OBJECT_BINDABLE_PROPERTY(Launcher, QStringList, m_protonBuilds, &Launcher::protonBuildsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Launcher, QString, m_defaultProton, &Launcher::defaultProtonChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Launcher, QString, m_steamgridApiKey, &Launcher::steamgridApiKeyChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Launcher, QString, m_defaultLaunchArgs, &Launcher::defaultLaunchArgsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Launcher, QString, m_defaultWrapperCommand, &Launcher::defaultWrapperCommandChanged)
};
