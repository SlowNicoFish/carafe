#pragma once

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <functional>

/**
 * Manages the lifetime of external game/installer processes launched through
 * `umu-run`.
 *
 * Tracks running processes keyed by game id and cleans them up when they finish
 * or are stopped. The caller supplies callbacks for started/terminal events so
 * this class stays agnostic to how results are surfaced to the UI or the model.
 */
class LaunchManager : public QObject {
    Q_OBJECT

public:
    explicit LaunchManager(QObject *parent = nullptr);
    ~LaunchManager() override;

    struct Spec {
        QUuid gameUuid;         // valid => tracked as a running game
        QString umuId;          // GAMEID if set
        QString prefixPath;     // WINEPREFIX
        QString protonPath;     // PROTONPATH if set
        QString overrideGameId; // forces GAMEID when set (e.g. installer)
        QString exePath;
        QString wrapperCommand;
        QStringList args;       // extra args after the executable
        QString successMessage; // reported by onTerminal on clean exit
    };

    /** Whether the given game currently has a tracked process running. */
    bool isRunning(const QUuid &gameUuid) const;

    /**
     * Starts `umu-run <wrapper> <exePath> <args...>` with the environment
     * derived from the spec. `onTerminal(ok, message)` is invoked exactly once,
     * on either error or normal exit.
     */
    void start(const Spec &spec, const std::function<void()> &onStarted,
               const std::function<void(bool ok, const QString &message)> &onTerminal);

    /** Terminates the tracked process for a game and stops tracking it. */
    void stop(const QUuid &gameUuid);

    /** Terminates all tracked processes (used on shutdown). */
    void terminateAll();

private:
    QMap<QUuid, QProcess *> m_runningGames;
    QSet<QUuid> m_stopped;
};
