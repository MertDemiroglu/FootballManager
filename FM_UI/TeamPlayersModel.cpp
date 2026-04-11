#include "TeamPlayersModel.h"

#include <utility>

TeamPlayersModel::TeamPlayersModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int TeamPlayersModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rows.size();
}

QVariant TeamPlayersModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size()) {
        return {};
    }

    const Row& row = rows[static_cast<qsizetype>(index.row())];
    switch (role) {
    case PlayerIdRole:
        return row.playerId;
    case NameRole:
        return row.name;
    case AgeRole:
        return row.age;
    case PositionRole:
        return row.position;
    case OverallSummaryRole:
        return row.overallSummary;
    case AppearancesRole:
        return row.appearances;
    case MinutesRole:
        return row.minutes;
    case GoalsRole:
        return row.goals;
    case AssistsRole:
        return row.assists;
    default:
        return {};
    }
}

QHash<int, QByteArray> TeamPlayersModel::roleNames() const {
    return {
        { PlayerIdRole, "playerId" },
        { NameRole, "name" },
        { AgeRole, "age" },
        { PositionRole, "position" },
        { OverallSummaryRole, "overallSummary" },
        { AppearancesRole, "appearances" },
        { MinutesRole, "minutes" },
        { GoalsRole, "goals" },
        { AssistsRole, "assists" },
    };
}

void TeamPlayersModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rows = std::move(newRows);
    endResetModel();
}

void TeamPlayersModel::clear() {
    setRows({});
}
