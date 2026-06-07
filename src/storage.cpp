#include "storage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

Storage::Storage(QObject *parent)
    : QObject(parent)
{}

QString Storage::libraryPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/library.json");
}

QList<Game> Storage::loadLibrary() const
{
    QFile f(libraryPath());
    if (!f.open(QIODevice::ReadOnly))
        return {};

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray())
        return {};

    QList<Game> games;
    for (const QJsonValue &val : doc.array()) {
        if (val.isObject()) {
            Game g = Game::fromJson(val.toObject());
            if (g.isValid())
                games.append(g);
        }
    }
    return games;
}

bool Storage::saveLibrary(const QList<Game> &games) const
{
    const QString path = libraryPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonArray arr;
    for (const Game &g : games)
        arr.append(g.toJson());

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    f.write(QJsonDocument(arr).toJson());
    return f.commit();
}
