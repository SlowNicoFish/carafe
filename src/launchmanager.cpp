#include "launchmanager.h"
#include "shellargs.h"

#include <QTimer>
using namespace Qt::Literals::StringLiterals;

static constexpr auto UMU_RUN = "umu-run";

LaunchManager::LaunchManager(QObject *parent)
    : QObject(parent) {}

LaunchManager::~LaunchManager() {
    terminateAll();
}

bool LaunchManager::isRunning(const QUuid &gameUuid) const {
    return m_runningGames.contains(gameUuid);
}

void LaunchManager::start(const Spec &spec, const std::function<void()> &onStarted,
                          const std::function<void(bool ok, const QString &message)> &onTerminal) {
    const bool tracked = !spec.gameUuid.isNull();

    QString program = QString::fromLatin1(UMU_RUN);
    QStringList programArgs = parseShellArgs(spec.wrapperCommand.trimmed());
    if (!programArgs.isEmpty()) {
        program = programArgs.takeFirst();
        programArgs << QString::fromLatin1(UMU_RUN);
    }
    programArgs << spec.exePath << spec.args;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString gameIdValue = spec.overrideGameId.isEmpty() ? spec.umuId : spec.overrideGameId;
    if (!gameIdValue.isEmpty())
        env.insert(u"GAMEID"_s, gameIdValue);
    if (!spec.prefixPath.isEmpty())
        env.insert(u"WINEPREFIX"_s, spec.prefixPath);
    if (!spec.protonPath.isEmpty())
        env.insert(u"PROTONPATH"_s, spec.protonPath);

    QProcess *process = new QProcess(this);
    process->setProcessEnvironment(env);

    // Register before starting so signal handlers always see consistent state.
    if (tracked)
        m_runningGames.insert(spec.gameUuid, process);

    connect(process, &QProcess::started, this, [onStarted]() { onStarted(); });

    auto finished = std::make_shared<bool>(false);
    auto finishProcess = [this, process, tracked, uuid = spec.gameUuid, onTerminal, finished](bool ok,
                                                                                              const QString &message) {
        if (*finished)
            return;
        *finished = true;
        const bool wasStopped = tracked && m_stoppedProcesses.remove(process);
        if (tracked && m_runningGames.value(uuid) == process)
            m_runningGames.remove(uuid);
        process->deleteLater();
        if (wasStopped)
            Q_EMIT processStopped(uuid);
        else
            onTerminal(ok, message);
    };

    connect(process, &QProcess::errorOccurred, this, [program, finishProcess](QProcess::ProcessError error) {
        QString message;
        if (error == QProcess::FailedToStart)
            message = u"Failed to start %1. Is it installed?"_s.arg(program);
        else if (error == QProcess::Crashed)
            message = u"%1 crashed."_s.arg(program);
        else
            message = u"Process error: %1"_s.arg(static_cast<int>(error));

        if (error != QProcess::Crashed)
            finishProcess(false, message);
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [finishProcess, successMessage = spec.successMessage](int exitCode, QProcess::ExitStatus status) {
                if (status == QProcess::CrashExit) {
                    finishProcess(false, u"Process crashed."_s);
                    return;
                }
                const bool ok = (exitCode == 0);
                finishProcess(ok, ok ? successMessage : u"Process exited with code %1."_s.arg(exitCode));
            });

    process->start(program, programArgs);
}

void LaunchManager::stop(const QUuid &gameUuid) {
    QProcess *process = m_runningGames.value(gameUuid);
    if (!process || m_stoppedProcesses.contains(process))
        return;

    m_stoppedProcesses.insert(process);
    process->terminate();
    QTimer::singleShot(1500, process, [process] {
        if (process->state() != QProcess::NotRunning)
            process->kill();
    });
}

void LaunchManager::terminateAll() {
    const auto processes = m_runningGames.values();
    for (QProcess *process : processes) {
        if (process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(3000))
                process->kill();
        }
    }
    m_runningGames.clear();
    m_stoppedProcesses.clear();
}
