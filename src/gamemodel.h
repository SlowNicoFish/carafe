#pragma once

#include "game.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QUuid>

/**
 * Qt list model that exposes the game library to QML.
 */
class GameModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    TitleRole,
    ExePathRole,
    LaunchArgsRole,
    PrefixPathRole,
    ProtonVersionRole,
    ProtonPathRole,
    UmuIdRole,
    IconPathRole,
    GridPathRole,
    SteamgridIconPathRole,
    IsRunningRole,
  };

  explicit GameModel(QObject *parent = nullptr);

  // ── QAbstractListModel ────────────────────────────────────────────────
  int rowCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  // ── Mutation helpers called from Launcher ─────────────────────────────
  void setGames(const QList<Game> &games);
  void addGame(const Game &game);
  void updateGame(const Game &game);
  void removeGame(const QUuid &id);

  // ── Running-state tracking ────────────────────────────────────────────
  void setRunning(const QUuid &id, bool running);

  // ── Accessors ─────────────────────────────────────────────────────────
  QList<Game> games() const { return m_games; }
  Game gameById(const QUuid &id) const;
  bool hasGame(const QUuid &id) const;

  // ── QML-invokable helpers ─────────────────────────────────────────────
  Q_INVOKABLE QVariantMap get(int row) const;

Q_SIGNALS:
  void countChanged();

private:
  int indexOfId(const QUuid &id) const;

  QList<Game> m_games;
  QSet<QUuid> m_running;
};
