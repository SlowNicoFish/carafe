#pragma once

#include <QString>
#include <QUuid>
#include <QJsonObject>

struct Game {
    QUuid   id;
    QString title;
    QString exePath;
    QString launchArgs;
    QString prefixPath;
    QString protonVersion;
    QString protonPath;
    QString umuId;
    QString iconPath;
    QString gridPath;
    QString steamgridIconPath;

    static Game create(const QString &title, const QString &exePath, const QString &prefixPath);

    QJsonObject toJson() const;
    static Game fromJson(const QJsonObject &obj);

    bool isValid() const { return !id.isNull() && !title.isEmpty() && !exePath.isEmpty(); }
};
