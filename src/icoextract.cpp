#include "icoextract.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
using namespace Qt::Literals::StringLiterals;

namespace IcoExtract {

namespace {

QString iconDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/icons"_s;
}

bool runProcess(QProcess &process, const QString &program, const QStringList &arguments) {
    process.start(program, arguments);
    if (!process.waitForFinished(30000)) {
        if (process.state() != QProcess::NotRunning) {
            process.terminate();
            if (!process.waitForFinished(3000))
                process.kill();
        }
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

} // namespace

QString extractIcon(const QUuid &gameId, const QString &exePath) {
    if (gameId.isNull() || exePath.isEmpty() || !QFileInfo(exePath).isFile())
        return {};

    const QString dirPath = iconDirectory();
    QDir dir(dirPath);
    if (!QDir().mkpath(dirPath))
        return {};

    const QString gameIdStr = gameId.toString(QUuid::WithoutBraces);
    const QString finalPngPath = dirPath + u"/%1.png"_s.arg(gameIdStr);

    QFile::remove(finalPngPath);

    QProcess wrestool;
    if (!runProcess(wrestool, u"wrestool"_s, {u"-t"_s, u"14"_s, u"-x"_s, exePath}))
        return {};

    const QByteArray icoData = wrestool.readAllStandardOutput();
    if (icoData.isEmpty()) {
        return {};
    }

    // Security: limit icon data size (10MB)
    if (icoData.size() > 10 * 1024 * 1024) {
        return {};
    }

    const QString tmpIcoPath = dirPath + u"/%1_tmp.ico"_s.arg(gameIdStr);
    QFile tmpIcoFile(tmpIcoPath);
    if (!tmpIcoFile.open(QIODevice::WriteOnly)) {
        return {};
    }
    if (tmpIcoFile.write(icoData) != icoData.size()) {
        tmpIcoFile.close();
        QFile::remove(tmpIcoPath);
        return {};
    }
    tmpIcoFile.close();

    QProcess icotool;
    icotool.setWorkingDirectory(dirPath);
    if (!runProcess(icotool, u"icotool"_s, {u"-x"_s, tmpIcoPath})) {
        QFile::remove(tmpIcoPath);
        return {};
    }

    QFile::remove(tmpIcoPath);

    const QString stem = u"%1_tmp"_s.arg(gameIdStr);
    const auto entries = dir.entryInfoList({stem + u"*.png"_s}, QDir::Files);

    QString bestPngPath;
    qint64 maxSize = -1;

    for (const QFileInfo &entry : entries) {
        const qint64 size = entry.size();
        if (size > maxSize) {
            maxSize = size;
            bestPngPath = entry.filePath();
        }
    }

    if (bestPngPath.isEmpty()) {
        return {};
    }

    QFile::remove(finalPngPath);
    const bool renamed = QFile::rename(bestPngPath, finalPngPath);

    // Clean up all other extracted files (and the best one if rename failed)
    for (const QFileInfo &entry : entries) {
        if (entry.filePath() != bestPngPath) {
            QFile::remove(entry.filePath());
        }
    }
    if (!renamed) {
        QFile::remove(bestPngPath);
    }

    return renamed ? finalPngPath : QString{};
}

} // namespace IcoExtract
