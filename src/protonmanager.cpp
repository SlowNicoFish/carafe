#include "protonmanager.h"

#include <algorithm>

ProtonManager::ProtonManager(QObject *parent)
    : QObject(parent) {}

void ProtonManager::reload() {
    m_builds = ProtonDetector::discoverBuilds();
    Q_EMIT buildNamesChanged();
}

QString ProtonManager::resolvePath(const QString &versionName) const {
    auto it =
        std::find_if(m_builds.cbegin(), m_builds.cend(), [&](const ProtonBuild &b) { return b.name == versionName; });
    return it != m_builds.cend() ? it->path : QString{};
}

QStringList ProtonManager::buildNames() const {
    return ProtonDetector::buildNames(m_builds);
}
