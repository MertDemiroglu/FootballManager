#pragma once

#include <QAbstractListModel>
#include <QVector>

class TeamPlayersModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct Row {
        int playerId = 0;
        QString name;
        int age = 0;
        QString position;
        QString overallSummary;
        int appearances = 0;
        int minutes = 0;
        int goals = 0;
        int assists = 0;
    };

    enum Role {
        PlayerIdRole = Qt::UserRole + 1,
        NameRole,
        AgeRole,
        PositionRole,
        OverallSummaryRole,
        AppearancesRole,
        MinutesRole,
        GoalsRole,
        AssistsRole,
    };

    explicit TeamPlayersModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QVector<Row> rows);
    void clear();

private:
    QVector<Row> rows;
};
