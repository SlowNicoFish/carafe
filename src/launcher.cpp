#include "launcher.h"
#include "shellargs.h"

#include <QDir>
#include <QUrl>
using namespace Qt::Literals::StringLiterals;

QString Launcher::urlToLocalFile(const QUrl &url) {
    return url.toLocalFile();
}

QUrl Launcher::localFileToUrl(const QString &path) {
    return QUrl::fromLocalFile(path);
}

Launcher::Launcher(QObject *parent)
    : QObject(parent)
    , m_games(this)
    , m_settingsStore(this)
    , m_steamGrid(this) {
    setSettings(m_settingsStore.loadBasic());
    connectSteamGrid();
}

Launcher::~Launcher() = default;

void Launcher::connectSteamGrid() {
    // An artwork fetch either belongs to an in-progress dialog preview (matched
    // by a random id in m_previewRequests) or targets an existing game.

    auto onAssetFetched = [this](const QUuid &gameId, const QString &path, bool isIcon) {
        if (auto it = m_previewRequests.find(gameId); it != m_previewRequests.end()) {
            const QString name = it.value();
            m_previewRequests.erase(it);
            if (isIcon)
                Q_EMIT iconPreviewReady(name, path);
            else
                Q_EMIT gridPreviewReady(name, path);
            return;
        }
        Game game = m_games.model()->gameById(gameId);
        if (!game.isValid())
            return;
        if (isIcon)
            game.steamgridIconPath = path;
        else
            game.gridPath = path;
        if (!m_games.updateGame(game))
            Q_EMIT toastMessage(m_games.lastError());
    };
    auto onAssetError = [this](const QUuid &gameId, const QString &error, bool isIcon) {
        if (auto it = m_previewRequests.find(gameId); it != m_previewRequests.end()) {
            const QString name = it.value();
            m_previewRequests.erase(it);
            if (isIcon)
                Q_EMIT iconPreviewFailed(name, error);
            else
                Q_EMIT gridPreviewFailed(name, error);
            return;
        }
        Q_EMIT toastMessage(u"%1: %2"_s.arg(isIcon ? u"Icon"_s : u"Grid art"_s, error));
    };

    connect(&m_steamGrid, &SteamGrid::gridFetched, this,
            [onAssetFetched](const QUuid &id, const QString &path) { onAssetFetched(id, path, false); });
    connect(&m_steamGrid, &SteamGrid::iconFetched, this,
            [onAssetFetched](const QUuid &id, const QString &path) { onAssetFetched(id, path, true); });
    connect(&m_steamGrid, &SteamGrid::gridError, this,
            [onAssetError](const QUuid &id, const QString &error) { onAssetError(id, error, false); });
    connect(&m_steamGrid, &SteamGrid::iconError, this,
            [onAssetError](const QUuid &id, const QString &error) { onAssetError(id, error, true); });
}

GameModel *Launcher::gameModel() {
    return m_games.model();
}

QStringList Launcher::protonBuilds() const {
    return m_protonBuilds;
}

QString Launcher::defaultProton() const {
    return m_defaultProton;
}

QString Launcher::steamgridApiKey() {
    // Deferred so a slow or missing keyring cannot delay startup.
    if (!m_apiKeyLoaded) {
        m_apiKeyLoaded = true;
        m_steamgridApiKey = SettingsStore::loadApiKey(m_steamgridApiKey);
    }
    return m_steamgridApiKey;
}

QString Launcher::defaultLaunchArgs() const {
    return m_defaultLaunchArgs;
}

QString Launcher::defaultWrapperCommand() const {
    return m_defaultWrapperCommand;
}

void Launcher::setSettings(const AppSettings &settings) {
    // Assignments emit the matching NOTIFY signals only on actual changes.
    m_defaultProton = settings.defaultProton;
    m_steamgridApiKey = settings.steamgridApiKey;
    m_defaultLaunchArgs = settings.defaultLaunchArgs;
    m_defaultWrapperCommand = settings.defaultWrapperCommand;
}

void Launcher::loadLibrary() {
    m_games.load(defaultProton(), [this](const QString &version) { return m_proton.resolvePath(version); });
}

bool Launcher::saveLibrary() const {
    return m_games.save();
}

void Launcher::reloadProtonBuilds() {
    m_proton.reload();
    m_protonBuilds = m_proton.buildNames();
}

QString Launcher::suggestPrefix(const QString &title) const {
    return m_games.suggestPrefix(title);
}

bool Launcher::addGame(const QString &title, const QString &exePath, const QString &prefixPath,
                       const QString &protonVersion, const QString &umuId, const QString &gridPath,
                       const QString &steamgridIconPath, const QString &wrapperCommand) {
    QString resolvedPrefix = prefixPath.trimmed();
    if (resolvedPrefix.isEmpty())
        resolvedPrefix = suggestPrefix(title);

    Game game = Game::create(title, exePath, resolvedPrefix);
    if (!game.isValid()) {
        Q_EMIT toastMessage(u"The game details are incomplete."_s);
        return false;
    }

    game.protonVersion = protonVersion.isEmpty() ? defaultProton() : protonVersion;
    game.protonPath = m_proton.resolvePath(game.protonVersion);
    game.umuId = umuId;
    game.launchArgs = defaultLaunchArgs();
    game.wrapperCommand = wrapperCommand.isEmpty() ? defaultWrapperCommand() : wrapperCommand;
    game.gridPath = gridPath;
    game.steamgridIconPath = steamgridIconPath;

    const bool saved = m_games.addGame(game);
    if (!saved)
        Q_EMIT toastMessage(m_games.lastError());
    return saved;
}

// QML-facing: accepts a map with only the fields that should change.
// Recognized keys: title, exePath, launchArgs, wrapperCommand, prefixPath,
// umuId, iconPath, gridPath, steamgridIconPath. Each falls back to the existing
// game value if absent/empty; protonVersion is kept if omitted or empty.
bool Launcher::updateGame(const QString &gameId, const QVariantMap &fields) {
    const QUuid uuid(gameId);
    if (uuid.isNull()) {
        Q_EMIT toastMessage(u"Invalid game identifier."_s);
        return false;
    }

    Game game = m_games.model()->gameById(uuid);
    if (!game.isValid()) {
        Q_EMIT toastMessage(u"Game not found."_s);
        return false;
    }

    const QString newExePath = fields.value(u"exePath"_s, game.exePath).toString();
    const QString newProtonVersion = fields.value(u"protonVersion"_s, game.protonVersion).toString();

    // An empty proton string means "keep the current selection" so clearing the
    // combo in the edit dialog cannot silently wipe the stored version.
    game.protonVersion = newProtonVersion.isEmpty() ? game.protonVersion : newProtonVersion;

    game.title = fields.value(u"title"_s, game.title).toString();
    game.exePath = newExePath;
    game.launchArgs = fields.value(u"launchArgs"_s, game.launchArgs).toString();
    game.wrapperCommand = fields.value(u"wrapperCommand"_s, game.wrapperCommand).toString();
    game.prefixPath = fields.value(u"prefixPath"_s, game.prefixPath).toString();
    game.protonPath = m_proton.resolvePath(game.protonVersion);
    game.umuId = fields.value(u"umuId"_s, game.umuId).toString();
    game.iconPath = fields.value(u"iconPath"_s, game.iconPath).toString();
    game.gridPath = fields.value(u"gridPath"_s, game.gridPath).toString();
    game.steamgridIconPath = fields.value(u"steamgridIconPath"_s, game.steamgridIconPath).toString();

    const bool saved = m_games.updateGame(game);
    if (!saved)
        Q_EMIT toastMessage(m_games.lastError());
    return saved;
}

bool Launcher::removeGame(const QString &gameId, bool removePrefix) {
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return false;

    m_launch.stop(uuid);
    const bool removed = m_games.removeGame(uuid, removePrefix);
    if (!removed)
        Q_EMIT toastMessage(m_games.lastError());
    return removed;
}

QVariantMap Launcher::gameById(const QString &gameId) {
    return m_games.model()->getById(gameId);
}

bool Launcher::launchGame(const QString &gameId) {
    const QUuid uuid(gameId);
    if (uuid.isNull()) {
        Q_EMIT gameLaunchFailed(gameId, u"Invalid game identifier."_s);
        return false;
    }

    const Game game = m_games.model()->gameById(uuid);
    if (!game.isValid()) {
        Q_EMIT gameLaunchFailed(gameId, u"Game not found."_s);
        return false;
    }
    if (game.exePath.isEmpty()) {
        Q_EMIT gameLaunchFailed(gameId, u"No executable configured for this game."_s);
        return false;
    }
    if (game.prefixPath.isEmpty()) {
        Q_EMIT gameLaunchFailed(gameId, u"No Wine prefix configured for this game."_s);
        return false;
    }
    if (m_launch.isRunning(uuid)) {
        Q_EMIT gameLaunchFailed(gameId, u"Game is already running."_s);
        return false;
    }

    reloadProtonBuilds();
    const QString protonPath = m_proton.resolvePath(game.protonVersion);
    if (!game.protonVersion.isEmpty() && protonPath.isEmpty()) {
        Q_EMIT gameLaunchFailed(gameId,
                                u"The selected Proton version is no longer available: %1"_s.arg(game.protonVersion));
        return false;
    }

    LaunchManager::Spec spec;
    spec.gameUuid = uuid;
    spec.umuId = game.umuId;
    spec.prefixPath = game.prefixPath;
    spec.protonPath = protonPath;
    spec.exePath = game.exePath;
    spec.wrapperCommand = game.wrapperCommand;
    if (!game.launchArgs.trimmed().isEmpty())
        spec.args = parseShellArgs(game.launchArgs);

    m_launch.start(
        spec, [this, uuid]() { m_games.model()->setRunning(uuid, true); },
        [this, uuid, gameId](bool, const QString &message) {
            m_games.model()->setRunning(uuid, false);
            if (!message.isEmpty())
                Q_EMIT gameLaunchFailed(gameId, message);
        });
    return true;
}

void Launcher::fetchGrid(const QString &gameId, const QString &apiKey) {
    if (apiKey.trimmed().isEmpty())
        return;
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return;
    const Game game = m_games.model()->gameById(uuid);
    if (game.isValid())
        m_steamGrid.fetchGrid(game.title, uuid, apiKey);
}

void Launcher::fetchIcon(const QString &gameId, const QString &apiKey) {
    if (apiKey.trimmed().isEmpty())
        return;
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return;
    const Game game = m_games.model()->gameById(uuid);
    if (game.isValid())
        m_steamGrid.fetchIcon(game.title, uuid, apiKey);
}

void Launcher::fetchGridArtwork(const QString &gameName) {
    if (steamgridApiKey().isEmpty()) {
        Q_EMIT toastMessage(u"Set a SteamGridDB API key in Settings first."_s);
        return;
    }
    const QUuid id = QUuid::createUuid();
    m_previewRequests.insert(id, gameName);
    m_steamGrid.fetchGrid(gameName, id, steamgridApiKey());
}

void Launcher::fetchIconArtwork(const QString &gameName) {
    if (steamgridApiKey().isEmpty()) {
        Q_EMIT toastMessage(u"Set a SteamGridDB API key in Settings first."_s);
        return;
    }
    const QUuid id = QUuid::createUuid();
    m_previewRequests.insert(id, gameName);
    m_steamGrid.fetchIcon(gameName, id, steamgridApiKey());
}

void Launcher::runInstaller(const QString &installerPath, const QString &prefixPath, const QString &protonVersion) {
    if (installerPath.isEmpty()) {
        Q_EMIT installerFinished(false, u"No installer path specified."_s);
        return;
    }

    // Ensure the prefix directory exists before launching.
    const QString resolvedPrefix = prefixPath.trimmed();
    if (resolvedPrefix.isEmpty()) {
        Q_EMIT installerFinished(false, u"A Wine prefix path is required to run the installer."_s);
        return;
    }
    QDir prefixDir(resolvedPrefix);
    if (!prefixDir.exists() && !prefixDir.mkpath(u"."_s)) {
        Q_EMIT installerFinished(false, u"Could not create Wine prefix directory: %1"_s.arg(resolvedPrefix));
        return;
    }

    reloadProtonBuilds();
    const QString resolved = m_proton.resolvePath(protonVersion);

    LaunchManager::Spec spec;
    spec.prefixPath = resolvedPrefix;
    spec.protonPath = resolved;
    spec.overrideGameId = u"carafe-installer"_s;
    spec.exePath = installerPath;
    spec.wrapperCommand = defaultWrapperCommand();
    spec.successMessage = u"Installer finished successfully."_s;

    m_launch.start(
        spec, [this]() { Q_EMIT installerStarted(); },
        [this](bool ok, const QString &message) { Q_EMIT installerFinished(ok, message); });
}

void Launcher::runExeInPrefix(const QString &gameId, const QString &exePath) {
    if (exePath.isEmpty()) {
        Q_EMIT runExeInPrefixFinished(false, u"No executable path specified."_s);
        return;
    }

    const QUuid uuid(gameId);
    if (uuid.isNull()) {
        Q_EMIT runExeInPrefixFinished(false, u"Invalid game identifier."_s);
        return;
    }

    const Game game = m_games.model()->gameById(uuid);
    if (!game.isValid()) {
        Q_EMIT runExeInPrefixFinished(false, u"Game not found."_s);
        return;
    }

    reloadProtonBuilds();
    const QString protonPath = m_proton.resolvePath(game.protonVersion);
    if (!game.protonVersion.isEmpty() && protonPath.isEmpty()) {
        Q_EMIT runExeInPrefixFinished(
            false, u"The selected Proton version is no longer available: %1"_s.arg(game.protonVersion));
        return;
    }

    LaunchManager::Spec spec;
    spec.umuId = game.umuId;
    spec.prefixPath = game.prefixPath;
    spec.protonPath = protonPath;
    spec.exePath = exePath;
    spec.wrapperCommand = game.wrapperCommand;
    spec.successMessage = u"Executable finished successfully."_s;

    m_launch.start(
        spec, []() {}, [this](bool ok, const QString &message) { Q_EMIT runExeInPrefixFinished(ok, message); });
}

QString Launcher::importImage(const QString &sourcePath, const QString &gameId, const QString &suffix) {
    return m_games.importImage(sourcePath, gameId, suffix);
}

bool Launcher::saveSettings(const QVariantMap &settings) {
    AppSettings s;
    s.defaultProton = settings.value(u"defaultProton"_s).toString();
    s.steamgridApiKey = settings.value(u"steamgridApiKey"_s).toString();
    s.defaultLaunchArgs = settings.value(u"defaultLaunchArgs"_s).toString();
    s.defaultWrapperCommand = settings.value(u"defaultWrapperCommand"_s).toString();

    if (!m_settingsStore.save(s)) {
        Q_EMIT toastMessage(u"Could not save settings."_s);
        return false;
    }

    setSettings(s);
    reloadProtonBuilds();
    return true;
}
