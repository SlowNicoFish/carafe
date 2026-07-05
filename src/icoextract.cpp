#include "icoextract.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace IcoExtract {

QString iconDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/icons");
}

QString extractIcon(const QUuid &gameId, const QString &exePath)
{
    if (gameId.isNull() || exePath.isEmpty())
        return {};

    const QString dirPath = iconDir();
    QDir dir(dirPath);
    dir.mkpath(dirPath);

    const QString gameIdStr = gameId.toString(QUuid::WithoutBraces);
    const QString finalPngPath = dirPath + QStringLiteral("/%1.png").arg(gameIdStr);

    if (QFile::exists(finalPngPath)) {
        return finalPngPath;
    }

    // Step 1: Run wrestool to extract ICO data from exe.
    // Large executables can take a while; allow up to 30 seconds before giving up.
    QProcess wrestool;
    wrestool.start(QStringLiteral("wrestool"), {QStringLiteral("-t"), QStringLiteral("14"), QStringLiteral("-x"), exePath});
    if (!wrestool.waitForFinished(30000)) {
        wrestool.terminate();
        if (!wrestool.waitForFinished(3000))
            wrestool.kill();
        return {};
    }

    if (wrestool.exitStatus() != QProcess::NormalExit || wrestool.exitCode() != 0) {
        return {};
    }

    const QByteArray icoData = wrestool.readAllStandardOutput();
    if (icoData.isEmpty()) {
        return {};
    }

    // Security: limit icon data size (10MB)
    if (icoData.size() > 10 * 1024 * 1024) {
        return {};
    }

    const QString tmpIcoPath = dirPath + QStringLiteral("/%1_tmp.ico").arg(gameIdStr);
    QFile tmpIcoFile(tmpIcoPath);
    if (!tmpIcoFile.open(QIODevice::WriteOnly)) {
        return {};
    }
    tmpIcoFile.write(icoData);
    tmpIcoFile.close();

    // Step 2: Run icotool to extract PNGs from the .ico file.
    // Use the same generous timeout as wrestool for consistency.
    QProcess icotool;
    icotool.setWorkingDirectory(dirPath);
    icotool.start(QStringLiteral("icotool"), {QStringLiteral("-x"), tmpIcoPath});
    if (!icotool.waitForFinished(30000)) {
        icotool.terminate();
        if (!icotool.waitForFinished(3000))
            icotool.kill();
        QFile::remove(tmpIcoPath);
        return {};
    }

    QFile::remove(tmpIcoPath);

    if (icotool.exitStatus() != QProcess::NormalExit || icotool.exitCode() != 0) {
        return {};
    }

    // Step 3: Find the best PNG file matching the stem and clean up others
    const QString stem = QStringLiteral("%1_tmp").arg(gameIdStr);
    const auto entries = dir.entryInfoList({stem + QStringLiteral("*.png")}, QDir::Files);

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

    // Rename the best one to the final path
    QFile::remove(finalPngPath);
    const bool renamed = QFile::rename(bestPngPath, finalPngPath);

    // Clean up all other extracted files (and the best one if rename failed)
    for (const QFileInfo &entry : entries) {
        if (entry.filePath() != bestPngPath) {
            QFile::remove(entry.filePath());
        }
    }
    // If rename failed, remove the leftover best file too
    if (!renamed) {
        QFile::remove(bestPngPath);
    }

    return renamed ? finalPngPath : QString{};
}

} // namespace IcoExtract
