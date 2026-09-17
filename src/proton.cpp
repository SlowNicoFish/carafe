#include "proton.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>
using namespace Qt::Literals::StringLiterals;

QList<ProtonBuild> ProtonDetector::discoverBuilds() {
    const QString home = QDir::homePath();

    QStringList searchDirs = {
        home + u"/.local/share/Steam/compatibilitytools.d"_s,
        home + u"/.steam/root/compatibilitytools.d"_s,
        home + u"/.local/share/Steam/steamapps/common"_s,
        home + u"/.var/app/com.valvesoftware.Steam/data/Steam/compatibilitytools.d"_s,
        home + u"/.var/app/com.valvesoftware.Steam/data/Steam/steamapps/common"_s,
        u"/usr/share/steam/compatibilitytools.d"_s,
    };

    QList<ProtonBuild> builds;

    for (const QString &dirPath : std::as_const(searchDirs)) {
        QDir dir(dirPath);
        if (!dir.exists())
            continue;

        const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            const QString protonBin = entry.filePath() + u"/proton"_s;
            if (!QFile::exists(protonBin))
                continue;

            ProtonBuild build;
            build.name = entry.fileName();
            build.path = entry.filePath();
            builds.append(build);
        }
    }

    QSet<QString> seen;
    builds.erase(std::remove_if(builds.begin(), builds.end(),
                                [&](const ProtonBuild &b) {
                                    const QString canonical = QFileInfo(b.path).canonicalFilePath();
                                    if (seen.contains(canonical))
                                        return true;
                                    seen.insert(canonical);
                                    return false;
                                }),
                 builds.end());

    std::sort(builds.begin(), builds.end(), [](const ProtonBuild &a, const ProtonBuild &b) { return a.name < b.name; });

    return builds;
}

QStringList ProtonDetector::buildNames(const QList<ProtonBuild> &builds) {
    QStringList names;
    names.reserve(builds.size());
    std::transform(builds.cbegin(), builds.cend(), std::back_inserter(names),
                   [](const ProtonBuild &b) { return b.name; });
    return names;
}
