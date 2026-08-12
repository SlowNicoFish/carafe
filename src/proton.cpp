#include "proton.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>

QList<ProtonBuild> ProtonDetector::discoverBuilds()
{
    const QString home = QDir::homePath();

    QStringList searchDirs = {
        home + QStringLiteral("/.local/share/Steam/compatibilitytools.d"),
        home + QStringLiteral("/.steam/root/compatibilitytools.d"),
        home + QStringLiteral("/.local/share/Steam/steamapps/common"),
        QStringLiteral("/usr/share/steam/compatibilitytools.d"),
    };

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

    QSet<QString> seen;
    builds.erase(std::remove_if(builds.begin(), builds.end(), [&](const ProtonBuild &b) {
        const QString canonical = QFileInfo(b.path).canonicalFilePath();
        if (seen.contains(canonical))
            return true;
        seen.insert(canonical);
        return false;
    }), builds.end());

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
