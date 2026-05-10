#pragma once

#include <QAbstractListModel>
#include <QVector>

class DashboardUpcomingMatchesModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct Row {
        QString dateText;
        QString homeTeamName;
        QString homePrimaryColor;
        QString homeSecondaryColor;
        QString homeTextColor;
        QString awayTeamName;
        QString awayPrimaryColor;
        QString awaySecondaryColor;
        QString awayTextColor;
        bool isHome = false;
        int matchweek = 0;
    };

    enum Role {
        DateTextRole = Qt::UserRole + 1,
        HomeTeamNameRole,
        HomePrimaryColorRole,
        HomeSecondaryColorRole,
        HomeTextColorRole,
        AwayTeamNameRole,
        AwayPrimaryColorRole,
        AwaySecondaryColorRole,
        AwayTextColorRole,
        IsHomeRole,
        MatchweekRole,
    };

    explicit DashboardUpcomingMatchesModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QVector<Row> rows);
    void clear();

private:
    QVector<Row> rows;
};
