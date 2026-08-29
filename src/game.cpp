#include "game.h"
using namespace Qt::Literals::StringLiterals;

Game Game::create(const QString &title, const QString &exePath, const QString &prefixPath) {
    Game g;
    g.id = QUuid::createUuid();
    g.title = title;
    g.exePath = exePath;
    g.prefixPath = prefixPath;
    return g;
}

QJsonObject Game::toJson() const {
    QJsonObject obj;
    obj[u"id"_s] = id.toString(QUuid::WithoutBraces);
    obj[u"title"_s] = title;
    obj[u"exePath"_s] = exePath;
    obj[u"launchArgs"_s] = launchArgs;
    obj[u"wrapperCommand"_s] = wrapperCommand;
    obj[u"prefixPath"_s] = prefixPath;
    obj[u"protonVersion"_s] = protonVersion;
    // protonPath is intentionally not persisted, it is always re-resolved from
    // protonVersion at load time via ProtonManager::resolvePath.
    obj[u"umuId"_s] = umuId;
    obj[u"iconPath"_s] = iconPath;
    obj[u"gridPath"_s] = gridPath;
    obj[u"steamgridIconPath"_s] = steamgridIconPath;
    return obj;
}

Game Game::fromJson(const QJsonObject &obj) {
    Game g;
    g.id = QUuid::fromString(obj[u"id"_s].toString());
    g.title = obj[u"title"_s].toString();
    g.exePath = obj[u"exePath"_s].toString();
    g.launchArgs = obj[u"launchArgs"_s].toString();
    g.wrapperCommand = obj[u"wrapperCommand"_s].toString();
    g.prefixPath = obj[u"prefixPath"_s].toString();
    g.protonVersion = obj[u"protonVersion"_s].toString();
    // protonPath is left empty here; Launcher resolves it after loading.
    g.umuId = obj[u"umuId"_s].toString();
    g.iconPath = obj[u"iconPath"_s].toString();
    g.gridPath = obj[u"gridPath"_s].toString();
    g.steamgridIconPath = obj[u"steamgridIconPath"_s].toString();
    return g;
}
