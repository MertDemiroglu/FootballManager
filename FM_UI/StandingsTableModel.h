#pragma once

#include<QAbstractListModel>
#include<QVector>

class StandingsTableModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct Row {
        int position = 0;
        int teamId = 0;
        QString teamName;
        int played = 0;
        int wins = 0;
        int draws = 0;
        int losses = 0;
        int goalsFor = 0;
        int goalsAgainst = 0;
        int goalDifference = 0;
        int points = 0;
        QString recentForm;
        bool isSelectedTeam = false;
    };

    enum Role {
        PositionRole = Qt::UserRole + 1,
        TeamIdRole,
        TeamNameRole,
        PlayedRole,
        WinsRole,
        DrawsRole,
        LossesRole,
        GoalsForRole,
        GoalsAgainstRole,
        GoalDifferenceRole,
        PointsRole,
        RecentFormRole,
        IsSelectedTeamRole,
    };

    explicit StandingsTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QVector<Row> rows);
    void clear();

private:
    QVector<Row> rows;
};
