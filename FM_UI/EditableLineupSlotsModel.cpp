#include "EditableLineupSlotsModel.h"

#include <utility>

EditableLineupSlotsModel::EditableLineupSlotsModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int EditableLineupSlotsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rowsValue.size();
}

int EditableLineupSlotsModel::count() const {
    return rowsValue.size();
}

QVariant EditableLineupSlotsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowsValue.size()) {
        return {};
    }

    const Row& row = rowsValue[static_cast<qsizetype>(index.row())];
    switch (role) {
    case SlotIndexRole:
        return row.slotIndex;
    case SlotRoleRole:
        return row.slotRole;
    case SlotRoleKeyRole:
        return row.slotRoleKey;
    case SlotRoleTextRole:
        return row.slotRoleText;
    case SlotLabelRole:
        return row.slotLabel;
    case IsEmptyRole:
        return row.isEmpty;
    case HasAssignedPlayerRole:
        return row.hasAssignedPlayer;
    case AssignedPlayerIdRole:
        return row.assignedPlayerId;
    case AssignedPlayerNameRole:
        return row.assignedPlayerName;
    case AssignedPlayerOverallRole:
        return row.assignedPlayerOverall;
    case AssignedPlayerOverallSummaryRole:
        return row.assignedPlayerOverallSummary;
    case AssignedPlayerFormRole:
        return row.assignedPlayerForm;
    case AssignedPlayerFitnessRole:
        return row.assignedPlayerFitness;
    case AssignedPlayerMoraleRole:
        return row.assignedPlayerMorale;
    case PitchXRole:
        return row.pitchX;
    case PitchYRole:
        return row.pitchY;
    case DisplayOrderRole:
        return row.displayOrder;
    default:
        return {};
    }
}

QHash<int, QByteArray> EditableLineupSlotsModel::roleNames() const {
    return {
        { SlotIndexRole, "slotIndex" },
        { SlotRoleRole, "slotRole" },
        { SlotRoleKeyRole, "slotRoleKey" },
        { SlotRoleTextRole, "slotRoleText" },
        { SlotLabelRole, "slotLabel" },
        { IsEmptyRole, "isEmpty" },
        { HasAssignedPlayerRole, "hasAssignedPlayer" },
        { AssignedPlayerIdRole, "assignedPlayerId" },
        { AssignedPlayerNameRole, "assignedPlayerName" },
        { AssignedPlayerOverallRole, "assignedPlayerOverall" },
        { AssignedPlayerOverallSummaryRole, "assignedPlayerOverallSummary" },
        { AssignedPlayerFormRole, "assignedPlayerForm" },
        { AssignedPlayerFitnessRole, "assignedPlayerFitness" },
        { AssignedPlayerMoraleRole, "assignedPlayerMorale" },
        { PitchXRole, "pitchX" },
        { PitchYRole, "pitchY" },
        { DisplayOrderRole, "displayOrder" },
    };
}

QVariantList EditableLineupSlotsModel::rows() const {
    QVariantList list;
    list.reserve(rowsValue.size());
    for (const Row& row : rowsValue) {
        list.push_back(toVariantMap(row));
    }
    return list;
}

void EditableLineupSlotsModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rowsValue = std::move(newRows);
    endResetModel();
    emit rowsChanged();
}

void EditableLineupSlotsModel::clear() {
    setRows({});
}

QVariantMap EditableLineupSlotsModel::toVariantMap(const Row& row) const {
    QVariantMap map;
    map.insert(QStringLiteral("slotIndex"), row.slotIndex);
    map.insert(QStringLiteral("slotRole"), row.slotRole);
    map.insert(QStringLiteral("slotRoleKey"), row.slotRoleKey);
    map.insert(QStringLiteral("slotRoleText"), row.slotRoleText);
    map.insert(QStringLiteral("slotLabel"), row.slotLabel);
    map.insert(QStringLiteral("isEmpty"), row.isEmpty);
    map.insert(QStringLiteral("hasAssignedPlayer"), row.hasAssignedPlayer);
    map.insert(QStringLiteral("assignedPlayerId"), row.assignedPlayerId);
    map.insert(QStringLiteral("assignedPlayerName"), row.assignedPlayerName);
    map.insert(QStringLiteral("assignedPlayerOverall"), row.assignedPlayerOverall);
    map.insert(QStringLiteral("assignedPlayerOverallSummary"), row.assignedPlayerOverallSummary);
    map.insert(QStringLiteral("assignedPlayerForm"), row.assignedPlayerForm);
    map.insert(QStringLiteral("assignedPlayerFitness"), row.assignedPlayerFitness);
    map.insert(QStringLiteral("assignedPlayerMorale"), row.assignedPlayerMorale);
    map.insert(QStringLiteral("pitchX"), row.pitchX);
    map.insert(QStringLiteral("pitchY"), row.pitchY);
    map.insert(QStringLiteral("displayOrder"), row.displayOrder);
    return map;
}
