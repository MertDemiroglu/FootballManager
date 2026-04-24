#pragma once

#include <QAbstractListModel>
#include <QVector>

class EditableLineupRosterModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY rowsChanged)

public:
    struct Row {
        int playerId = 0;
        QString name;
        QString positionShort;
        int overall = 0;
        QString overallSummary;
        int form = 0;
        int fitness = 0;
        int morale = 0;
        bool isAssigned = false;
        int assignedSlotIndex = -1;
    };

    enum Role {
        PlayerIdRole = Qt::UserRole + 1,
        NameRole,
        PositionShortRole,
        OverallRole,
        OverallSummaryRole,
        FormRole,
        FitnessRole,
        MoraleRole,
        IsAssignedRole,
        AssignedSlotIndexRole,
    };

    explicit EditableLineupRosterModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int count() const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QVector<Row> rows);
    void clear();

signals:
    void rowsChanged();

private:
    QVector<Row> rowsValue;
};
