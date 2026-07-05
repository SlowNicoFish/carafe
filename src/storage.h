#pragma once

#include "game.h"

#include <QList>
#include <QObject>

/**
 * Persists the game library to
 * ~/.local/share/io.marlonn.carafe/library.json.
 */
class Storage : public QObject
{
    Q_OBJECT

public:
    explicit Storage(QObject *parent = nullptr);

    QList<Game> loadLibrary() const;
    bool        saveLibrary(const QList<Game> &games) const;

private:
    static QString libraryPath();
};
