#include "launcher.h"
#include "icoextract.h"

#include <QDir>
#include <QPointer>
#include <QUrl>
#include <QtConcurrent>

static constexpr auto UMU_RUN = "umu-run";

// Splits a shell-style argument string into a list of tokens, respecting
// single-quoted ('...'), double-quoted ("..."), and backslash-escaped characters.
// Examples:
//   -foo "bar baz"       -> ["-foo", "bar baz"]
//   -x 'hello world'     -> ["-x", "hello world"]
//   -p pass\ word        -> ["-p", "pass word"]
static QStringList parseShellArgs(const QString &input)
{
    QStringList result;
    QString current;
    bool inSingle = false;
    bool inDouble = false;

    for (int i = 0; i < input.size(); ++i) {
        const QChar ch = input[i];

        if (inSingle) {
            if (ch == u'\'')
                inSingle = false;
            else
                current += ch;
        } else if (inDouble) {
            if (ch == u'"') {
                inDouble = false;
            } else if (ch == u'\\' && i + 1 < input.size()) {
                // Inside double quotes only \", \\, \$, \` and \newline are special.
                const QChar next = input[i + 1];
                if (next == u'"' || next == u'\\' || next == u'$' || next == u'`') {
                    current += next;
                    ++i;
                } else {
                    current += ch;
                }
            } else {
                current += ch;
            }
        } else {
            if (ch == u'\'') {
                inSingle = true;
            } else if (ch == u'"') {
                inDouble = true;
            } else if (ch == u'\\' && i + 1 < input.size()) {
                current += input[++i];
            } else if (ch.isSpace()) {
                if (!current.isEmpty()) {
                    result << current;
                    current.clear();
                }
            } else {
                current += ch;
            }
        }
    }

    if (!current.isEmpty())
        result << current;

    return result;
}

QString Launcher::urlToLocalFile(const QUrl &url)
{
    return url.toLocalFile();
}

Launcher::Launcher(QObject *parent)
    : QObject(parent)
    , m_gameModel(this)
    , m_storage(this)
    , m_settingsStore(this)
    , m_steamGrid(this)
    , m_settings(m_settingsStore.load())
{
    auto handleGridResult = [this](const QUuid &gameId, const QString &path) {
        auto it = m_previewRequests.find(gameId);
        if (it != m_previewRequests.end()) {
            const QString name = it.value();
            m_previewRequests.erase(it);
            Q_EMIT gridPreviewReady(name, path);
            return;
        }
        Game game = m_gameModel.gameById(gameId);
        if (!game.isValid())
            return;
        game.gridPath = path;
        m_gameModel.updateGame(game);
        saveLibrary();
    };

    auto handleIconResult = [this](const QUuid &gameId, const QString &path) {
        auto it = m_previewRequests.find(gameId);
        if (it != m_previewRequests.end()) {
            const QString name = it.value();
            m_previewRequests.erase(it);
            Q_EMIT iconPreviewReady(name, path);
            return;
        }
        Game game = m_gameModel.gameById(gameId);
        if (!game.isValid())
            return;
        game.steamgridIconPath = path;
        m_gameModel.updateGame(game);
        saveLibrary();
    };

    auto handleGridError = [this](const QUuid &gameId, const QString &error) {
        if (m_previewRequests.remove(gameId))
            return;
        Q_EMIT toastMessage(QStringLiteral("Grid art: %1").arg(error));
    };

    auto handleIconError = [this](const QUuid &gameId, const QString &error) {
        if (m_previewRequests.remove(gameId))
            return;
        Q_EMIT toastMessage(QStringLiteral("Icon: %1").arg(error));
    };

    connect(&m_steamGrid, &SteamGrid::gridFetched, this, handleGridResult);
    connect(&m_steamGrid, &SteamGrid::iconFetched, this, handleIconResult);
    connect(&m_steamGrid, &SteamGrid::gridError, this, handleGridError);
    connect(&m_steamGrid, &SteamGrid::iconError, this, handleIconError);
}

Launcher::~Launcher()
{
    for (QProcess *process : std::as_const(m_runningGames)) {
        if (process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(3000))
                process->kill();
        }
    }
}

GameModel *Launcher::gameModel()
{
    return &m_gameModel;
}

QStringList Launcher::protonBuilds() const
{
    return m_protonBuilds;
}

QString Launcher::defaultProton() const
{
    return m_settings.defaultProton;
}

QString Launcher::steamgridApiKey() const
{
    return m_settings.steamgridApiKey;
}

QString Launcher::defaultLaunchArgs() const
{
    return m_settings.defaultLaunchArgs;
}

QString Launcher::extraProtonPaths() const
{
    return m_settings.extraProtonPaths;
}

void Launcher::setSettings(const AppSettings &settings)
{
    if (m_settings.defaultProton != settings.defaultProton) {
        m_settings.defaultProton = settings.defaultProton;
        Q_EMIT defaultProtonChanged();
    }
    if (m_settings.steamgridApiKey != settings.steamgridApiKey) {
        m_settings.steamgridApiKey = settings.steamgridApiKey;
        Q_EMIT steamgridApiKeyChanged();
    }
    if (m_settings.defaultLaunchArgs != settings.defaultLaunchArgs) {
        m_settings.defaultLaunchArgs = settings.defaultLaunchArgs;
        Q_EMIT defaultLaunchArgsChanged();
    }
    if (m_settings.extraProtonPaths != settings.extraProtonPaths) {
        m_settings.extraProtonPaths = settings.extraProtonPaths;
        Q_EMIT extraProtonPathsChanged();
    }
}

void Launcher::loadLibrary()
{
    m_gameModel.setGames(m_storage.loadLibrary());
}

bool Launcher::saveLibrary() const
{
    return m_storage.saveLibrary(m_gameModel.games());
}

void Launcher::reloadProtonBuilds()
{
    const QStringList extraPaths = m_settings.extraProtonPaths
        .split(QRegularExpression(QStringLiteral("[\n\r]+")), Qt::SkipEmptyParts);
    m_discoveredProtonBuilds = ProtonDetector::discoverBuilds(extraPaths);
    m_protonBuilds = ProtonDetector::buildNames(m_discoveredProtonBuilds);
    Q_EMIT protonBuildsChanged();
}

QString Launcher::resolveProtonPath(const QString &versionName) const
{
    auto it = std::find_if(m_discoveredProtonBuilds.cbegin(), m_discoveredProtonBuilds.cend(),
                           [&](const ProtonBuild &b) { return b.name == versionName; });
    return it != m_discoveredProtonBuilds.cend() ? it->path : QString{};
}

void Launcher::triggerIconExtraction(const QUuid &gameId, const QString &exePath)
{
    QPointer<Launcher> guard(this);
    [[maybe_unused]] QFuture<void> future = QtConcurrent::run([guard, gameId, exePath]() {
        const QString path = IcoExtract::extractIcon(gameId, exePath);
        if (!path.isEmpty() && guard) {
            QMetaObject::invokeMethod(guard.data(), [guard, gameId, path]() {
                if (!guard)
                    return;
                Game game = guard->m_gameModel.gameById(gameId);
                if (game.isValid()) {
                    game.iconPath = path;
                    guard->m_gameModel.updateGame(game);
                    guard->saveLibrary();
                }
            });
        }
    });
}

static QString slugify(const QString &title)
{
    QString slug;
    bool needSep = false;
    for (const QChar c : title) {
        if (c.isLetterOrNumber()) {
            slug.append(c.toLower());
            needSep = false;
        } else if (!needSep && !slug.isEmpty()) {
            slug.append(QLatin1Char('_'));
            needSep = true;
        }
    }
    if (slug.endsWith(QLatin1Char('_')))
        slug.chop(1);
    if (slug.isEmpty())
        slug = QStringLiteral("game");
    return slug;
}

QString Launcher::suggestPrefix(const QString &title) const
{
    const QString slug = slugify(title.trimmed());
    return QDir::homePath() + QStringLiteral("/carafe/prefixes/") + slug;
}

bool Launcher::addGame(const QString &title,
                       const QString &exePath,
                       const QString &prefixPath,
                       const QString &protonVersion,
                       const QString &umuId,
                       const QString &gridPath,
                       const QString &iconPath)
{
    QString resolvedPrefix = prefixPath.trimmed();
    if (resolvedPrefix.isEmpty())
        resolvedPrefix = suggestPrefix(title);

    Game game = Game::create(title, exePath, resolvedPrefix);
    if (!game.isValid())
        return false;

    game.protonVersion = protonVersion.isEmpty() ? m_settings.defaultProton : protonVersion;
    game.protonPath    = resolveProtonPath(game.protonVersion);
    game.umuId         = umuId;
    game.launchArgs    = m_settings.defaultLaunchArgs;
    game.gridPath      = gridPath;
    game.steamgridIconPath = iconPath;

    m_gameModel.addGame(game);
    triggerIconExtraction(game.id, game.exePath);
    return saveLibrary();
}

// QML-facing: accepts a map with only the fields that should change.
// Required keys: title, exePath, launchArgs, prefixPath, protonVersion, umuId.
// Optional keys: iconPath, gridPath, steamgridIconPath (preserved from existing game if absent).
bool Launcher::updateGame(const QString &gameId, const QVariantMap &fields)
{
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return false;

    Game game = m_gameModel.gameById(uuid);
    if (!game.isValid())
        return false;

    const QString newExePath      = fields.value(QStringLiteral("exePath"), game.exePath).toString();
    const QString newProtonVersion = fields.value(QStringLiteral("protonVersion"), game.protonVersion).toString();
    const bool exeChanged = (game.exePath != newExePath) || game.iconPath.isEmpty();

    game.title        = fields.value(QStringLiteral("title"),        game.title).toString();
    game.exePath      = newExePath;
    game.launchArgs   = fields.value(QStringLiteral("launchArgs"),   game.launchArgs).toString();
    game.prefixPath   = fields.value(QStringLiteral("prefixPath"),   game.prefixPath).toString();
    game.protonVersion = newProtonVersion;
    game.protonPath   = resolveProtonPath(newProtonVersion);
    game.umuId        = fields.value(QStringLiteral("umuId"),        game.umuId).toString();
    game.iconPath     = fields.value(QStringLiteral("iconPath"),     game.iconPath).toString();
    game.gridPath     = fields.value(QStringLiteral("gridPath"),     game.gridPath).toString();
    game.steamgridIconPath = fields.value(QStringLiteral("steamgridIconPath"), game.steamgridIconPath).toString();

    m_gameModel.updateGame(game);

    if (exeChanged)
        triggerIconExtraction(uuid, game.exePath);

    return saveLibrary();
}

bool Launcher::removeGame(const QString &gameId, bool removePrefix)
{
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return false;

    const Game game = m_gameModel.gameById(uuid);

    if (QProcess *process = m_runningGames.take(uuid)) {
        process->terminate();
        if (!process->waitForFinished(3000))
            process->kill();
        process->deleteLater();
    }

    m_gameModel.removeGame(uuid);

    if (removePrefix && game.isValid() && !game.prefixPath.isEmpty()) {
        QDir prefixDir(game.prefixPath);
        if (prefixDir.exists())
            prefixDir.removeRecursively();
    }

    return saveLibrary();
}

QVariantMap Launcher::gameById(const QString &gameId) const
{
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return {};

    const Game game = m_gameModel.gameById(uuid);
    if (!game.isValid())
        return {};

    return QVariantMap{
        {QStringLiteral("gameId"),             game.id.toString(QUuid::WithoutBraces)},
        {QStringLiteral("title"),              game.title},
        {QStringLiteral("exePath"),            game.exePath},
        {QStringLiteral("launchArgs"),         game.launchArgs},
        {QStringLiteral("prefixPath"),         game.prefixPath},
        {QStringLiteral("protonVersion"),      game.protonVersion},
        {QStringLiteral("protonPath"),         game.protonPath},
        {QStringLiteral("umuId"),              game.umuId},
        {QStringLiteral("iconPath"),           game.iconPath},
        {QStringLiteral("gridPath"),           game.gridPath},
        {QStringLiteral("steamgridIconPath"),  game.steamgridIconPath},
    };
}

bool Launcher::launchGame(const QString &gameId)
{
    const QUuid uuid(gameId);
    if (uuid.isNull()) {
        Q_EMIT gameLaunchFailed(gameId, QStringLiteral("Invalid game identifier."));
        return false;
    }

    const Game game = m_gameModel.gameById(uuid);
    if (!game.isValid()) {
        Q_EMIT gameLaunchFailed(gameId, QStringLiteral("Game not found."));
        return false;
    }

    if (game.exePath.isEmpty()) {
        Q_EMIT gameLaunchFailed(gameId, QStringLiteral("No executable configured for this game."));
        return false;
    }

    if (m_runningGames.contains(uuid))
        return false;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GAMEID"),     game.umuId.isEmpty() ? game.title : game.umuId);
    env.insert(QStringLiteral("WINEPREFIX"), game.prefixPath);
    if (!game.protonPath.isEmpty())
        env.insert(QStringLiteral("PROTONPATH"), game.protonPath);

    QStringList args;
    args << game.exePath;
    if (!game.launchArgs.trimmed().isEmpty())
        args << parseShellArgs(game.launchArgs);

    QProcess *process = new QProcess(this);
    process->setProcessEnvironment(env);

    // Insert before starting so signal handlers always see a consistent state.
    m_runningGames.insert(uuid, process);

    connect(process, &QProcess::started, this, [this, uuid]() {
        m_gameModel.setRunning(uuid, true);
    });

    connect(process, &QProcess::errorOccurred, this,
            [this, uuid, gameId](QProcess::ProcessError error) {
        const QString msg = error == QProcess::FailedToStart
            ? QStringLiteral("Failed to start %1. Is it installed?").arg(QString::fromLatin1(UMU_RUN))
            : QStringLiteral("Process error: %1").arg(static_cast<int>(error));
        Q_EMIT gameLaunchFailed(gameId, msg);
        m_gameModel.setRunning(uuid, false);
        if (QProcess *p = m_runningGames.take(uuid))
            p->deleteLater();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, uuid](int, QProcess::ExitStatus) {
        m_gameModel.setRunning(uuid, false);
        if (QProcess *p = m_runningGames.take(uuid))
            p->deleteLater();
    });

    process->start(QString::fromLatin1(UMU_RUN), args);
    return true;
}

void Launcher::fetchGrid(const QString &gameId, const QString &apiKey)
{
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return;
    const Game game = m_gameModel.gameById(uuid);
    if (game.isValid())
        m_steamGrid.fetchGrid(game.title, uuid, apiKey);
}

void Launcher::fetchIcon(const QString &gameId, const QString &apiKey)
{
    const QUuid uuid(gameId);
    if (uuid.isNull())
        return;
    const Game game = m_gameModel.gameById(uuid);
    if (game.isValid())
        m_steamGrid.fetchIcon(game.title, uuid, apiKey);
}

void Launcher::fetchGridArtwork(const QString &gameName)
{
    if (m_settings.steamgridApiKey.isEmpty()) {
        Q_EMIT toastMessage(QStringLiteral("Set a SteamGridDB API key in Settings first."));
        return;
    }
    const QUuid id = QUuid::createUuid();
    m_previewRequests.insert(id, gameName);
    m_steamGrid.fetchGrid(gameName, id, m_settings.steamgridApiKey);
}

void Launcher::fetchIconArtwork(const QString &gameName)
{
    if (m_settings.steamgridApiKey.isEmpty()) {
        Q_EMIT toastMessage(QStringLiteral("Set a SteamGridDB API key in Settings first."));
        return;
    }
    const QUuid id = QUuid::createUuid();
    m_previewRequests.insert(id, gameName);
    m_steamGrid.fetchIcon(gameName, id, m_settings.steamgridApiKey);
}

void Launcher::runInstaller(const QString &installerPath,
                             const QString &prefixPath,
                             const QString &protonVersion)
{
    if (installerPath.isEmpty()) {
        Q_EMIT installerFinished(false, QStringLiteral("No installer path specified."));
        return;
    }

    // Ensure the prefix directory exists before launching
    const QString resolvedPrefix = prefixPath.trimmed();
    if (resolvedPrefix.isEmpty()) {
        Q_EMIT installerFinished(false, QStringLiteral("A Wine prefix path is required to run the installer."));
        return;
    }
    QDir prefixDir(resolvedPrefix);
    if (!prefixDir.exists() && !prefixDir.mkpath(QStringLiteral("."))) {
        Q_EMIT installerFinished(false, QStringLiteral("Could not create Wine prefix directory: %1").arg(resolvedPrefix));
        return;
    }

    auto *process = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GAMEID"),     QStringLiteral("carafe-installer"));
    env.insert(QStringLiteral("WINEPREFIX"), resolvedPrefix);

    const QString resolved = resolveProtonPath(protonVersion);
    if (!resolved.isEmpty())
        env.insert(QStringLiteral("PROTONPATH"), resolved);

    process->setProcessEnvironment(env);

    connect(process, &QProcess::started, this, [this]() {
        Q_EMIT installerStarted();
    });

    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
        const QString msg = error == QProcess::FailedToStart
            ? QStringLiteral("Failed to start %1. Is it installed?").arg(QString::fromLatin1(UMU_RUN))
            : QStringLiteral("Process error: %1").arg(static_cast<int>(error));
        Q_EMIT installerFinished(false, msg);
        process->deleteLater();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process](int exitCode, QProcess::ExitStatus status) {
        const bool ok = (status == QProcess::NormalExit && exitCode == 0);
        Q_EMIT installerFinished(ok,
            ok ? QStringLiteral("Installer finished successfully.")
               : QStringLiteral("Installer exited with code %1.").arg(exitCode));
        process->deleteLater();
    });

    process->start(QString::fromLatin1(UMU_RUN), {installerPath});
}

void Launcher::runExeInPrefix(const QString &gameId, const QString &exePath)
{
    if (exePath.isEmpty()) {
        Q_EMIT runExeInPrefixFinished(false, QStringLiteral("No executable path specified."));
        return;
    }

    const QUuid uuid(gameId);
    if (uuid.isNull()) {
        Q_EMIT runExeInPrefixFinished(false, QStringLiteral("Invalid game identifier."));
        return;
    }

    const Game game = m_gameModel.gameById(uuid);
    if (!game.isValid()) {
        Q_EMIT runExeInPrefixFinished(false, QStringLiteral("Game not found."));
        return;
    }

    auto *process = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GAMEID"),     game.umuId.isEmpty() ? game.title : game.umuId);
    env.insert(QStringLiteral("WINEPREFIX"), game.prefixPath);
    if (!game.protonPath.isEmpty())
        env.insert(QStringLiteral("PROTONPATH"), game.protonPath);

    process->setProcessEnvironment(env);

    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
        const QString msg = error == QProcess::FailedToStart
            ? QStringLiteral("Failed to start %1. Is it installed?").arg(QString::fromLatin1(UMU_RUN))
            : QStringLiteral("Process error: %1").arg(static_cast<int>(error));
        Q_EMIT runExeInPrefixFinished(false, msg);
        process->deleteLater();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process](int exitCode, QProcess::ExitStatus status) {
        const bool ok = (status == QProcess::NormalExit && exitCode == 0);
        Q_EMIT runExeInPrefixFinished(ok,
            ok ? QStringLiteral("Executable finished successfully.")
               : QStringLiteral("Executable exited with code %1.").arg(exitCode));
        process->deleteLater();
    });

    process->start(QString::fromLatin1(UMU_RUN), {exePath});
}

bool Launcher::saveSettings(const QVariantMap &settings)
{
    AppSettings s;
    s.defaultProton     = settings.value(QStringLiteral("defaultProton")).toString();
    s.steamgridApiKey   = settings.value(QStringLiteral("steamgridApiKey")).toString();
    s.defaultLaunchArgs = settings.value(QStringLiteral("defaultLaunchArgs")).toString();
    s.extraProtonPaths  = settings.value(QStringLiteral("extraProtonPaths")).toString();

    if (!m_settingsStore.save(s))
        return false;

    setSettings(s);
    reloadProtonBuilds();
    return true;
}
