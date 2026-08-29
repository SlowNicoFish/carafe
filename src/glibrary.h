#pragma once

#include "game.h"
#include "gamemodel.h"
#include "storage.h"

#include <QObject>
#include <QHash>
#include <QSet>
#include <QUuid>

#include <functional>

/**
 * Owns the in-memory game library (GameModel), its persistence (Storage), and
 * the asset lifecycle associated with games (prefix suggestion, image import,
 * exe icon extraction).
 *
 * Mutations keep the model, the running state, and the on-disk library in sync,
 * so callers only need to construct a Game and hand it to us.
 */
class GameLibrary : public QObject {
    Q_OBJECT

public:
    explicit GameLibrary(QObject *parent = nullptr);

    /** The list model exposed to QML. */
    GameModel *model();

    /** Loads the library from disk, filling missing proton versions from
     *  `defaultProton` and resolving each game's protonPath via `resolvePath`. */
    void load(const QString &defaultProton, const std::function<QString(const QString &)> &resolvePath);

    /** Persists the current library; returns false on failure. */
    bool save() const;

    /** Builds a prefix path suggestion for a title that doesn't already exist. */
    QString suggestPrefix(const QString &title) const;

    /** Inserts `game`, triggers icon extraction, and persists. */
    bool addGame(Game game);

    /**
     * Replaces the stored game whose id matches `game.id`. If the executable
     * changed, re-extracts the exe icon. Persists on success.
     */
    bool updateGame(const Game &game);

    /** Removes the game, optionally deleting its prefix directory. Persists. */
    bool removeGame(const QUuid &id, bool removePrefix);

    /** Copies an image into the app's asset folder for the given game. */
    QString importImage(const QString &sourcePath, const QString &gameId, const QString &suffix) const;

    /** Asynchronously extracts an icon from `exePath` and stores it on the game. */
    void triggerIconExtraction(const QUuid &gameId, const QString &exePath);

private:
    void startIconExtraction(const QUuid &gameId, const QString &exePath);

    GameModel m_model;
    Storage m_storage;
    QHash<QUuid, QString> m_pendingIconExtractions;
    QSet<QUuid> m_runningIconExtractions;
};
