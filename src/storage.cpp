#include "storage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
using namespace Qt::Literals::StringLiterals;

Storage::Storage(QObject *parent)
    : QObject(parent) {}

QString Storage::libraryPath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + u"/library.json"_s;
}

// Renames a corrupt library file out of the way so the next save cannot
// silently destroy whatever data it still contained.
void Storage::quarantineLibrary() const {
    const QString path = libraryPath();
    if (!QFile::exists(path))
        return;

    const QString backup = path + u".corrupt-%1"_s.arg(QDateTime::currentDateTime().toString(u"yyyyMMdd-HHmmss"_s));
    if (QFile::rename(path, backup))
        qWarning() << "Quarantined corrupt library file to" << backup;
    else
        qWarning() << "Could not quarantine corrupt library file:" << path;
}

void Storage::quarantineInvalidRecords(const QJsonArray &records) const {
    if (records.isEmpty())
        return;

    const QString path = libraryPath() + u".invalid"_s;
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not write invalid library records to" << path;
        return;
    }
    if (!f.write(QJsonDocument(records).toJson()) || !f.commit())
        qWarning() << "Could not commit invalid library records to" << path;
    else
        qWarning() << "Preserved invalid library records in" << path;
}

QList<Game> Storage::loadLibrary() const {
    const QString path = libraryPath();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (QFile::exists(path))
            qWarning() << "Could not open library file:" << path << f.errorString();
        return {};
    }

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
    QSet<QUuid> ids;
    QJsonArray invalid;
    for (const QJsonValue &val : doc.array()) {
        if (!val.isObject()) {
            invalid.append(val);
            continue;
        }
        Game g = Game::fromJson(val.toObject());
        if (!g.isValid() || ids.contains(g.id)) {
            invalid.append(val);
            continue;
        }
        ids.insert(g.id);
        games.append(g);
    }
    quarantineInvalidRecords(invalid);
    qInfo() << "Loaded" << games.size() << "games from" << path;
    return games;
}

bool Storage::saveLibrary(const QList<Game> &games) const {
    const QString path = libraryPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonArray arr;
    for (const Game &g : games)
        arr.append(g.toJson());

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    const QByteArray data = QJsonDocument(arr).toJson();
    if (f.write(data) != data.size())
        return false;
    return f.commit();
}
