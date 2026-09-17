#include "gamemodel.h"

#include <algorithm>
#include <array>

namespace {

struct RoleInfo {
    int role;
    const char *name;
};

constexpr std::array<RoleInfo, 13> roleInfos = {{
    {GameModel::IdRole, "gameId"},
    {GameModel::TitleRole, "title"},
    {GameModel::ExePathRole, "exePath"},
    {GameModel::LaunchArgsRole, "launchArgs"},
    {GameModel::WrapperCommandRole, "wrapperCommand"},
    {GameModel::PrefixPathRole, "prefixPath"},
    {GameModel::ProtonVersionRole, "protonVersion"},
    {GameModel::ProtonPathRole, "protonPath"},
    {GameModel::UmuIdRole, "umuId"},
    {GameModel::IconPathRole, "iconPath"},
    {GameModel::GridPathRole, "gridPath"},
    {GameModel::SteamgridIconPathRole, "steamgridIconPath"},
    {GameModel::IsRunningRole, "isRunning"},
}};

} // namespace

GameModel::GameModel(QObject *parent)
    : QAbstractListModel(parent) {}

int GameModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_games.size();
}

QVariant GameModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_games.size())
        return {};

    const Game &g = m_games.at(index.row());

    switch (role) {
        case IdRole:
            return g.id.toString(QUuid::WithoutBraces);
        case TitleRole:
            return g.title;
        case ExePathRole:
            return g.exePath;
        case LaunchArgsRole:
            return g.launchArgs;
        case WrapperCommandRole:
            return g.wrapperCommand;
        case PrefixPathRole:
            return g.prefixPath;
        case ProtonVersionRole:
            return g.protonVersion;
        case ProtonPathRole:
            return g.protonPath;
        case UmuIdRole:
            return g.umuId;
        case IconPathRole:
            return g.iconPath;
        case GridPathRole:
            return g.gridPath;
        case SteamgridIconPathRole:
            return g.steamgridIconPath;
        case IsRunningRole:
            return m_running.contains(g.id);
        default:
            return {};
    }
}

QHash<int, QByteArray> GameModel::roleNames() const {
    QHash<int, QByteArray> roles;
    for (const RoleInfo &info : roleInfos)
        roles.insert(info.role, info.name);
    return roles;
}

void GameModel::setGames(const QList<Game> &games) {
    beginResetModel();
    m_games = games;
    m_running.clear();
    endResetModel();
    Q_EMIT countChanged();
}

void GameModel::addGame(const Game &game) {
    beginInsertRows({}, m_games.size(), m_games.size());
    m_games.append(game);
    endInsertRows();
    Q_EMIT countChanged();
}

void GameModel::updateGame(const Game &game) {
    int row = indexOfId(game.id);
    if (row < 0)
        return;
    m_games[row] = game;
    Q_EMIT dataChanged(index(row), index(row));
}

void GameModel::removeGame(const QUuid &id) {
    int row = indexOfId(id);
    if (row < 0)
        return;
    beginRemoveRows({}, row, row);
    m_games.removeAt(row);
    endRemoveRows();
    m_running.remove(id);
    Q_EMIT countChanged();
}

void GameModel::setRunning(const QUuid &id, bool running) {
    const bool wasRunning = m_running.contains(id);
    if (wasRunning == running)
        return;

    if (running)
        m_running.insert(id);
    else
        m_running.remove(id);

    int row = indexOfId(id);
    if (row >= 0)
        Q_EMIT dataChanged(index(row), index(row), {IsRunningRole});
}

Game GameModel::gameById(const QUuid &id) const {
    int row = indexOfId(id);
    return row >= 0 ? m_games.at(row) : Game{};
}

QVariantMap GameModel::getById(const QString &id) const {
    const int row = indexOfId(QUuid(id));
    if (row < 0)
        return {};

    const QModelIndex idx = index(row);
    const auto roles = roleNames();
    QVariantMap map;
    for (auto it = roles.cbegin(); it != roles.cend(); ++it)
        map.insert(QString::fromUtf8(it.value()), data(idx, it.key()));
    return map;
}

int GameModel::indexOfId(const QUuid &id) const {
    auto it = std::find_if(m_games.cbegin(), m_games.cend(), [&](const Game &g) { return g.id == id; });
    return it != m_games.cend() ? std::distance(m_games.cbegin(), it) : -1;
}
