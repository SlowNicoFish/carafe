#pragma once

#include "proton.h"

#include <QList>
#include <QObject>
#include <QStringList>

/**
 * Discovers installed Proton builds and resolves a build name to its directory.
 *
 * Owns the set of discovered builds and exposes the list of build names for the
 * UI. A constructed ProtonManager starts with an empty build list until
 * reload() is called (typically once at startup and whenever builds change).
 */
class ProtonManager : public QObject {
    Q_OBJECT

public:
    explicit ProtonManager(QObject *parent = nullptr);

    /** Re-scans the filesystem and updates the exposed build-name list. */
    void reload();

    /** Returns the resolved directory path for a build name, or empty. */
    QString resolvePath(const QString &versionName) const;

    /** The list of discovered build names, for UI combo boxes. */
    QStringList buildNames() const;

Q_SIGNALS:
    void buildNamesChanged();

private:
    QList<ProtonBuild> m_builds;
};
