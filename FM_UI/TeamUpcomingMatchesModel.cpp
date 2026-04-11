#include "TeamUpcomingMatchesModel.h"

#include <utility>

TeamUpcomingMatchesModel::TeamUpcomingMatchesModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int TeamUpcomingMatchesModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rows.size();
}

QVariant TeamUpcomingMatchesModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size()) {
        return {};
    }

    const Row& row = rows[static_cast<qsizetype>(index.row())];
    switch (role) {
    case DateTextRole:
        return row.dateText;
    case HomeTeamIdRole:
        return row.homeTeamId;
    case AwayTeamIdRole:
        return row.awayTeamId;
    case HomeTeamNameRole:
        return row.homeTeamName;
    case AwayTeamNameRole:
        return row.awayTeamName;
    case IsHomeRole:
        return row.isHome;
    case MatchweekRole:
        return row.matchweek;
    default:
        return {};
    }
}

QHash<int, QByteArray> TeamUpcomingMatchesModel::roleNames() const {
    return {
        { DateTextRole, "dateText" },
        { HomeTeamIdRole, "homeTeamId" },
        { AwayTeamIdRole, "awayTeamId" },
        { HomeTeamNameRole, "homeTeamName" },
        { AwayTeamNameRole, "awayTeamName" },
        { IsHomeRole, "isHome" },
        { MatchweekRole, "matchweek" },
    };
}

void TeamUpcomingMatchesModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rows = std::move(newRows);
    endResetModel();
}

void TeamUpcomingMatchesModel::clear() {
    setRows({});
}
