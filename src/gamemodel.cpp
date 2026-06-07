#include "gamemodel.h"

#include <algorithm>

GameModel::GameModel(QObject *parent) : QAbstractListModel(parent) {}

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
  return {
      {IdRole, "gameId"},
      {TitleRole, "title"},
      {ExePathRole, "exePath"},
      {LaunchArgsRole, "launchArgs"},
      {PrefixPathRole, "prefixPath"},
      {ProtonVersionRole, "protonVersion"},
      {ProtonPathRole, "protonPath"},
      {UmuIdRole, "umuId"},
      {IconPathRole, "iconPath"},
      {GridPathRole, "gridPath"},
      {SteamgridIconPathRole, "steamgridIconPath"},
      {IsRunningRole, "isRunning"},
  };
}

void GameModel::setGames(const QList<Game> &games) {
  beginResetModel();
  m_games = games;
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

bool GameModel::hasGame(const QUuid &id) const { return indexOfId(id) >= 0; }

QVariantMap GameModel::get(int row) const {
  if (row < 0 || row >= m_games.size())
    return {};

  const Game &g = m_games.at(row);
  return {
      {QStringLiteral("gameId"), g.id.toString(QUuid::WithoutBraces)},
      {QStringLiteral("title"), g.title},
      {QStringLiteral("exePath"), g.exePath},
      {QStringLiteral("launchArgs"), g.launchArgs},
      {QStringLiteral("prefixPath"), g.prefixPath},
      {QStringLiteral("protonVersion"), g.protonVersion},
      {QStringLiteral("protonPath"), g.protonPath},
      {QStringLiteral("umuId"), g.umuId},
      {QStringLiteral("iconPath"), g.iconPath},
      {QStringLiteral("gridPath"), g.gridPath},
      {QStringLiteral("steamgridIconPath"), g.steamgridIconPath},
      {QStringLiteral("isRunning"), m_running.contains(g.id)},
  };
}

int GameModel::indexOfId(const QUuid &id) const {
  auto it = std::find_if(m_games.cbegin(), m_games.cend(),
                         [&](const Game &g) { return g.id == id; });
  return it != m_games.cend() ? std::distance(m_games.cbegin(), it) : -1;
}
