#pragma once

#include <QAbstractListModel>
#include <QVector>

class TeamUpcomingMatchesModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct Row {
        QString dateText;
        int homeTeamId = 0;
        int awayTeamId = 0;
        QString homeTeamName;
        QString awayTeamName;
        bool isHome = false;
        int matchweek = 0;
    };

    enum Role {
        DateTextRole = Qt::UserRole + 1,
        HomeTeamIdRole,
        AwayTeamIdRole,
        HomeTeamNameRole,
        AwayTeamNameRole,
        IsHomeRole,
        MatchweekRole,
    };

    explicit TeamUpcomingMatchesModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QVector<Row> rows);
    void clear();

private:
    QVector<Row> rows;
};
