#pragma once

#include <QString>
#include <QList>
#include <QStringList>

struct ProtonBuild {
    QString name;
    QString path;
    bool    isValveProton = false;
};

class ProtonDetector
{
public:
    static QList<ProtonBuild> discoverBuilds(const QStringList &extraPaths = {});
    static QStringList buildNames(const QList<ProtonBuild> &builds);
};
