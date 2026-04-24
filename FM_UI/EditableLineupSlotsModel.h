#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QVector>

class EditableLineupSlotsModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowsChanged)

public:
    struct Row {
        int slotIndex = 0;
        QString slotRole;
        int slotRoleKey = 0;
        QString slotRoleText;
        QString slotLabel;
        bool isEmpty = true;
        bool hasAssignedPlayer = false;
        int assignedPlayerId = 0;
        QString assignedPlayerName;
        int assignedPlayerOverall = 0;
        QString assignedPlayerOverallSummary;
        int assignedPlayerForm = 0;
        int assignedPlayerFitness = 0;
        int assignedPlayerMorale = 0;
    };

    enum Role {
        SlotIndexRole = Qt::UserRole + 1,
        SlotRoleRole,
        SlotRoleKeyRole,
        SlotRoleTextRole,
        SlotLabelRole,
        IsEmptyRole,
        HasAssignedPlayerRole,
        AssignedPlayerIdRole,
        AssignedPlayerNameRole,
        AssignedPlayerOverallRole,
        AssignedPlayerOverallSummaryRole,
        AssignedPlayerFormRole,
        AssignedPlayerFitnessRole,
        AssignedPlayerMoraleRole,
    };

    explicit EditableLineupSlotsModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantList rows() const;

    void setRows(QVector<Row> rows);
    void clear();

signals:
    void rowsChanged();

private:
    QVector<Row> rowsValue;
    QVariantMap toVariantMap(const Row& row) const;
};
