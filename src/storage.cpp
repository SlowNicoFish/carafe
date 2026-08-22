#include "storage.h"

#include <QDateTime>
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

// Renames a corrupt library file out of the way so the next save cannot
// silently destroy whatever data it still contained.
void Storage::quarantineLibrary() const
{
    const QString path = libraryPath();
    if (!QFile::exists(path))
        return;

    const QString backup = path + QStringLiteral(".corrupt-%1")
                               .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    if (QFile::rename(path, backup))
        qWarning() << "Quarantined corrupt library file to" << backup;
    else
        qWarning() << "Could not quarantine corrupt library file:" << path;
}

QList<Game> Storage::loadLibrary() const
{
    QFile f(libraryPath());
    if (!f.open(QIODevice::ReadOnly))
        return {};

    const QByteArray raw = f.readAll();
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse library file:" << parseError.errorString();
        quarantineLibrary();
        return {};
    }
    if (!doc.isArray()) {
        qWarning() << "Library file does not contain a JSON array.";
        quarantineLibrary();
        return {};
    }

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
