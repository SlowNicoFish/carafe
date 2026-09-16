#include "glibrary.h"
#include "icoextract.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QStandardPaths>
#include <QtConcurrent>
using namespace Qt::Literals::StringLiterals;

GameLibrary::GameLibrary(QObject *parent)
    : QObject(parent)
    , m_model(this) {}

GameModel *GameLibrary::model() {
    return &m_model;
}

void GameLibrary::load(const QString &defaultProton, const std::function<QString(const QString &)> &resolvePath) {
    QList<Game> games = m_storage.loadLibrary();
    for (Game &game : games) {
        if (game.protonVersion.isEmpty() && !defaultProton.isEmpty())
            game.protonVersion = defaultProton;
        game.protonPath = resolvePath(game.protonVersion);
    }
    m_model.setGames(games);
}

bool GameLibrary::save() const {
    return m_storage.saveLibrary(m_model.games());
}

static QString slugify(const QString &title) {
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
        slug = u"game"_s;
    return slug;
}

QString GameLibrary::suggestPrefix(const QString &title) const {
    const QString slug = slugify(title.trimmed());
    const QString base = QDir::homePath() + u"/carafe/prefixes/"_s + slug;
    QString path = base;
    int suffix = 2;
    while (QDir(path).exists())
        path = base + u"_%1"_s.arg(suffix++);
    return path;
}

bool GameLibrary::addGame(Game game) {
    m_lastError.clear();
    if (!game.isValid()) {
        setError(u"The game details are incomplete."_s);
        return false;
    }

    QList<Game> candidate = m_model.games();
    candidate.append(game);
    if (!m_storage.saveLibrary(candidate)) {
        setError(u"Could not save the game library."_s);
        return false;
    }
    m_model.addGame(game);
    triggerIconExtraction(game.id, game.exePath);
    return true;
}

bool GameLibrary::updateGame(const Game &game) {
    m_lastError.clear();
    const QUuid id = game.id;
    if (id.isNull()) {
        setError(u"Invalid game identifier."_s);
        return false;
    }

    const Game existing = m_model.gameById(id);
    if (!existing.isValid()) {
        setError(u"Game not found."_s);
        return false;
    }

    const bool exeChanged = (existing.exePath != game.exePath);
    QList<Game> candidate = m_model.games();
    for (Game &stored : candidate) {
        if (stored.id == id) {
            stored = game;
            break;
        }
    }
    if (!m_storage.saveLibrary(candidate)) {
        setError(u"Could not save the game library."_s);
        return false;
    }
    m_model.updateGame(game);

    if (exeChanged)
        triggerIconExtraction(id, game.exePath);

    return save();
}

bool GameLibrary::removeGame(const QUuid &id, bool removePrefix) {
    m_lastError.clear();
    if (id.isNull()) {
        setError(u"Invalid game identifier."_s);
        return false;
    }

    const Game game = m_model.gameById(id);
    if (!game.isValid()) {
        setError(u"Game not found."_s);
        return false;
    }

    if (removePrefix && !game.prefixPath.isEmpty()) {
        const QFileInfo prefixInfo(game.prefixPath);
        if (!prefixInfo.exists()) {
            setError(u"The prefix path does not exist: %1"_s.arg(game.prefixPath));
            return false;
        }
        if (!prefixInfo.isDir()) {
            setError(u"The configured prefix path is not a directory."_s);
            return false;
        }
        const QFileInfo rootInfo(managedPrefixRoot());
        if (rootInfo.canonicalFilePath().isEmpty() || prefixInfo.canonicalFilePath().isEmpty()) {
            setError(u"The prefix path must exist and resolve to a real directory."_s);
            return false;
        }
        const QString relative = QDir(rootInfo.canonicalFilePath()).relativeFilePath(prefixInfo.canonicalFilePath());
        if (relative.isEmpty() || relative == u"."_s || relative == u".."_s || relative.startsWith(u"../"_s) ||
            QDir::isAbsolutePath(relative)) {
            setError(u"Only prefixes inside %1 can be deleted."_s.arg(rootInfo.canonicalFilePath()));
            return false;
        }
    }

    QList<Game> candidate = m_model.games();
    candidate.removeIf([&](const Game &stored) { return stored.id == id; });
    if (!m_storage.saveLibrary(candidate)) {
        setError(u"Could not save the game library."_s);
        return false;
    }
    m_model.removeGame(id);

    if (removePrefix && !game.prefixPath.isEmpty()) {
        QString error;
        if (!removeManagedPrefix(game.prefixPath, &error)) {
            setError(u"Game removed, but the prefix could not be deleted: %1"_s.arg(error));
            return false;
        }
    }
    return true;
}

void GameLibrary::setError(const QString &error) {
    m_lastError = error;
}

QString GameLibrary::managedPrefixRoot() {
    return QDir::homePath() + u"/carafe/prefixes"_s;
}

bool GameLibrary::removeManagedPrefix(const QString &path, QString *error) {
    const QFileInfo rootInfo(managedPrefixRoot());
    const QFileInfo selectedInfo(path);
    const QString root = rootInfo.canonicalFilePath();
    const QString selected = selectedInfo.canonicalFilePath();
    if (root.isEmpty() || selected.isEmpty()) {
        *error = u"The prefix path must exist and resolve to a real directory."_s;
        return false;
    }
    const QString relative = QDir(root).relativeFilePath(selected);
    if (relative.isEmpty() || relative == u"."_s || relative == u".."_s || relative.startsWith(u"../"_s) ||
        QDir::isAbsolutePath(relative)) {
        *error = u"Only prefixes inside %1 can be deleted."_s.arg(root);
        return false;
    }
    if (!selectedInfo.isDir() || !QDir(selected).removeRecursively()) {
        *error = u"Could not delete %1."_s.arg(selected);
        return false;
    }
    return true;
}

QString GameLibrary::importImage(const QString &sourcePath, const QString &gameId, const QString &suffix) const {
    if (sourcePath.isEmpty())
        return {};

    const QFileInfo srcInfo(sourcePath);
    if (!srcInfo.exists() || !srcInfo.isFile())
        return {};

    const QString id = gameId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : gameId;
    const QString ext = srcInfo.suffix().toLower();
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/icons"_s;
    QDir().mkpath(dir);

    const QString dest = u"%1/%2_%3.%4"_s.arg(dir, id, suffix, ext);
    if (sourcePath == dest)
        return dest;

    QFile::remove(dest);
    if (!QFile::copy(sourcePath, dest))
        return {};

    return dest;
}

void GameLibrary::triggerIconExtraction(const QUuid &gameId, const QString &exePath) {
    // Avoid spawning concurrent wrestool/icotool processes for the same game
    // when addGame/updateGame is called repeatedly with a new exePath.
    if (gameId.isNull() || exePath.isEmpty())
        return;

    m_pendingIconExtractions.insert(gameId, exePath);
    if (m_runningIconExtractions.contains(gameId))
        return; // the completion handler will pick up the updated exe
    startIconExtraction(gameId, exePath);
}

void GameLibrary::startIconExtraction(const QUuid &gameId, const QString &exePath) {
    m_runningIconExtractions.insert(gameId);

    QPointer<GameLibrary> guard(this);
    [[maybe_unused]] QFuture<void> future = QtConcurrent::run([guard, gameId, exePath]() {
        const QString path = IcoExtract::extractIcon(gameId, exePath);
        if (!guard)
            return;
        QMetaObject::invokeMethod(guard.data(), [guard, gameId, exePath, path]() {
            if (!guard)
                return;

            const QString next = guard->m_pendingIconExtractions.value(gameId);
            guard->m_runningIconExtractions.remove(gameId);
            guard->m_pendingIconExtractions.remove(gameId);

            if (!path.isEmpty()) {
                Game game = guard->m_model.gameById(gameId);
                if (game.isValid() && game.exePath == exePath) {
                    QList<Game> candidate = guard->m_model.games();
                    for (Game &stored : candidate) {
                        if (stored.id == gameId) {
                            stored.iconPath = path;
                            break;
                        }
                    }
                    if (!guard->m_storage.saveLibrary(candidate)) {
                        guard->setError(u"Could not save the extracted game icon."_s);
                    } else {
                        game.iconPath = path;
                        guard->m_model.updateGame(game);
                    }
                }
            }

            if (!next.isEmpty())
                guard->startIconExtraction(gameId, next);
        });
    });
}
