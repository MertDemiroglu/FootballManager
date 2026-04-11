#include"StandingsTableModel.h"

#include<utility>

StandingsTableModel::StandingsTableModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int StandingsTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rows.size();
}

QVariant StandingsTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size()) {
        return {};
    }

    const Row& row = rows[static_cast<qsizetype>(index.row())];
    switch (role) {
    case PositionRole:
        return row.position;
    case TeamIdRole:
        return row.teamId;
    case TeamNameRole:
        return row.teamName;
    case PlayedRole:
        return row.played;
    case WinsRole:
        return row.wins;
    case DrawsRole:
        return row.draws;
    case LossesRole:
        return row.losses;
    case GoalsForRole:
        return row.goalsFor;
    case GoalsAgainstRole:
        return row.goalsAgainst;
    case GoalDifferenceRole:
        return row.goalDifference;
    case PointsRole:
        return row.points;
    case RecentFormRole:
        return row.recentForm;
    case IsSelectedTeamRole:
        return row.isSelectedTeam;
    default:
        return {};
    }
}

QHash<int, QByteArray> StandingsTableModel::roleNames() const {
    return {
        { PositionRole, "position" },
        { TeamIdRole, "teamId" },
        { TeamNameRole, "teamName" },
        { PlayedRole, "played" },
        { WinsRole, "wins" },
        { DrawsRole, "draws" },
        { LossesRole, "losses" },
        { GoalsForRole, "goalsFor" },
        { GoalsAgainstRole, "goalsAgainst" },
        { GoalDifferenceRole, "goalDifference" },
        { PointsRole, "points" },
        { RecentFormRole, "recentForm" },
        { IsSelectedTeamRole, "isSelectedTeam" },
    };
}

void StandingsTableModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rows = std::move(newRows);
    endResetModel();
}

void StandingsTableModel::clear() {
    setRows({});
}
