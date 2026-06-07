#include "game.h"

Game Game::create(const QString &title, const QString &exePath, const QString &prefixPath)
{
    Game g;
    g.id          = QUuid::createUuid();
    g.title       = title;
    g.exePath     = exePath;
    g.prefixPath  = prefixPath;
    return g;
}

QJsonObject Game::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")]                = id.toString(QUuid::WithoutBraces);
    obj[QStringLiteral("title")]             = title;
    obj[QStringLiteral("exePath")]           = exePath;
    obj[QStringLiteral("launchArgs")]        = launchArgs;
    obj[QStringLiteral("prefixPath")]        = prefixPath;
    obj[QStringLiteral("protonVersion")]     = protonVersion;
    // protonPath is intentionally not persisted, it is always re-resolved
    // from protonVersion at load time via ProtonDetector.
    obj[QStringLiteral("umuId")]             = umuId;
    obj[QStringLiteral("iconPath")]          = iconPath;
    obj[QStringLiteral("gridPath")]          = gridPath;
    obj[QStringLiteral("steamgridIconPath")] = steamgridIconPath;
    return obj;
}

Game Game::fromJson(const QJsonObject &obj)
{
    Game g;
    g.id               = QUuid::fromString(obj[QStringLiteral("id")].toString());
    g.title            = obj[QStringLiteral("title")].toString();
    g.exePath          = obj[QStringLiteral("exePath")].toString();
    g.launchArgs       = obj[QStringLiteral("launchArgs")].toString();
    g.prefixPath       = obj[QStringLiteral("prefixPath")].toString();
    g.protonVersion    = obj[QStringLiteral("protonVersion")].toString();
    // protonPath is left empty here; Launcher resolves it after loading.
    g.umuId            = obj[QStringLiteral("umuId")].toString();
    g.iconPath         = obj[QStringLiteral("iconPath")].toString();
    g.gridPath         = obj[QStringLiteral("gridPath")].toString();
    g.steamgridIconPath = obj[QStringLiteral("steamgridIconPath")].toString();
    return g;
}
