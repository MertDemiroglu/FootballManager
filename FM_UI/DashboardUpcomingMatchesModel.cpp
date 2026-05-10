#include "DashboardUpcomingMatchesModel.h"

#include <utility>

DashboardUpcomingMatchesModel::DashboardUpcomingMatchesModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int DashboardUpcomingMatchesModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rows.size();
}

QVariant DashboardUpcomingMatchesModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size()) {
        return {};
    }

    const Row& row = rows[static_cast<qsizetype>(index.row())];
    switch (role) {
    case DateTextRole:
        return row.dateText;
    case HomeTeamNameRole:
        return row.homeTeamName;
    case HomePrimaryColorRole:
        return row.homePrimaryColor;
    case HomeSecondaryColorRole:
        return row.homeSecondaryColor;
    case HomeTextColorRole:
        return row.homeTextColor;
    case AwayTeamNameRole:
        return row.awayTeamName;
    case AwayPrimaryColorRole:
        return row.awayPrimaryColor;
    case AwaySecondaryColorRole:
        return row.awaySecondaryColor;
    case AwayTextColorRole:
        return row.awayTextColor;
    case IsHomeRole:
        return row.isHome;
    case MatchweekRole:
        return row.matchweek;
    default:
        return {};
    }
}

QHash<int, QByteArray> DashboardUpcomingMatchesModel::roleNames() const {
    return {
        { DateTextRole, "dateText" },
        { HomeTeamNameRole, "homeTeamName" },
        { HomePrimaryColorRole, "homePrimaryColor" },
        { HomeSecondaryColorRole, "homeSecondaryColor" },
        { HomeTextColorRole, "homeTextColor" },
        { AwayTeamNameRole, "awayTeamName" },
        { AwayPrimaryColorRole, "awayPrimaryColor" },
        { AwaySecondaryColorRole, "awaySecondaryColor" },
        { AwayTextColorRole, "awayTextColor" },
        { IsHomeRole, "isHome" },
        { MatchweekRole, "matchweek" },
    };
}

void DashboardUpcomingMatchesModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rows = std::move(newRows);
    endResetModel();
}

void DashboardUpcomingMatchesModel::clear() {
    setRows({});
}
