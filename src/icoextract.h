#pragma once

#include <QString>
#include <QUuid>

namespace IcoExtract {

QString extractIcon(const QUuid &gameId, const QString &exePath);

} // namespace IcoExtract
