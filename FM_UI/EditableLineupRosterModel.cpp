#include "EditableLineupRosterModel.h"

#include <utility>

EditableLineupRosterModel::EditableLineupRosterModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int EditableLineupRosterModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rowsValue.size();
}

QVariant EditableLineupRosterModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowsValue.size()) {
        return {};
    }

    const Row& row = rowsValue[static_cast<qsizetype>(index.row())];
    switch (role) {
    case PlayerIdRole:
        return row.playerId;
    case NameRole:
        return row.name;
    case PositionShortRole:
        return row.positionShort;
    case OverallRole:
        return row.overall;
    case OverallSummaryRole:
        return row.overallSummary;
    case FormRole:
        return row.form;
    case FitnessRole:
        return row.fitness;
    case MoraleRole:
        return row.morale;
    case IsAssignedRole:
        return row.isAssigned;
    case AssignedSlotIndexRole:
        return row.assignedSlotIndex;
    default:
        return {};
    }
}

QHash<int, QByteArray> EditableLineupRosterModel::roleNames() const {
    return {
        { PlayerIdRole, "playerId" },
        { NameRole, "name" },
        { PositionShortRole, "positionShort" },
        { OverallRole, "overall" },
        { OverallSummaryRole, "overallSummary" },
        { FormRole, "form" },
        { FitnessRole, "fitness" },
        { MoraleRole, "morale" },
        { IsAssignedRole, "isAssigned" },
        { AssignedSlotIndexRole, "assignedSlotIndex" },
    };
}

void EditableLineupRosterModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rowsValue = std::move(newRows);
    endResetModel();
}

void EditableLineupRosterModel::clear() {
    setRows({});
}
