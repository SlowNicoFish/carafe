#include "proton.h"

#include <QDir>
#include <QStandardPaths>

#include <algorithm>

QList<ProtonBuild> ProtonDetector::discoverBuilds(const QStringList &extraPaths)
{
    const QString home = QDir::homePath();

    QStringList searchDirs = {
        home + QStringLiteral("/.local/share/Steam/compatibilitytools.d"),
        home + QStringLiteral("/.steam/root/compatibilitytools.d"),
        home + QStringLiteral("/.local/share/Steam/steamapps/common"),
        QStringLiteral("/usr/share/steam/compatibilitytools.d"),
    };

    for (const QString &extra : extraPaths) {
        if (!extra.trimmed().isEmpty())
            searchDirs.append(extra.trimmed());
    }

    QList<ProtonBuild> builds;

    for (const QString &dirPath : std::as_const(searchDirs)) {
        QDir dir(dirPath);
        if (!dir.exists())
            continue;

        const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            const QString protonBin = entry.filePath() + QStringLiteral("/proton");
            if (!QFile::exists(protonBin))
                continue;

            const bool isValve = QFile::exists(
                entry.filePath() + QStringLiteral("/files/bin/wine64"));

            ProtonBuild build;
            build.name = entry.fileName();
            build.path = entry.filePath();
            build.isValveProton = isValve;
            builds.append(build);
        }
    }

    std::sort(builds.begin(), builds.end(), [](const ProtonBuild &a, const ProtonBuild &b) {
        return a.name < b.name;
    });

    return builds;
}

QStringList ProtonDetector::buildNames(const QList<ProtonBuild> &builds)
{
    QStringList names;
    names.reserve(builds.size());
    std::transform(builds.cbegin(), builds.cend(), std::back_inserter(names),
                   [](const ProtonBuild &b) { return b.name; });
    return names;
}
