#include "TeamRecentMatchesModel.h"

#include <utility>

TeamRecentMatchesModel::TeamRecentMatchesModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int TeamRecentMatchesModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rows.size();
}

QVariant TeamRecentMatchesModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size()) {
        return {};
    }

    const Row& row = rows[static_cast<qsizetype>(index.row())];
    switch (role) {
    case DateTextRole:
        return row.dateText;
    case MatchweekRole:
        return row.matchweek;
    case OpponentIdRole:
        return row.opponentId;
    case OpponentNameRole:
        return row.opponentName;
    case IsHomeRole:
        return row.isHome;
    case GoalsForRole:
        return row.goalsFor;
    case GoalsAgainstRole:
        return row.goalsAgainst;
    case ResultLetterRole:
        return row.resultLetter;
    default:
        return {};
    }
}

QHash<int, QByteArray> TeamRecentMatchesModel::roleNames() const {
    return {
        { DateTextRole, "dateText" },
        { MatchweekRole, "matchweek" },
        { OpponentIdRole, "opponentId" },
        { OpponentNameRole, "opponentName" },
        { IsHomeRole, "isHome" },
        { GoalsForRole, "goalsFor" },
        { GoalsAgainstRole, "goalsAgainst" },
        { ResultLetterRole, "resultLetter" },
    };
}

void TeamRecentMatchesModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rows = std::move(newRows);
    endResetModel();
}

void TeamRecentMatchesModel::clear() {
    setRows({});
}
