#pragma once

#include <QString>
#include <QUuid>

namespace IcoExtract
{

QString iconDir();

QString extractIcon(const QUuid &gameId, const QString &exePath);

}
