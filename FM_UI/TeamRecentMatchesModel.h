#pragma once

#include <QAbstractListModel>
#include <QVector>

class TeamRecentMatchesModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct Row {
        QString dateText;
        int matchweek = 0;
        int opponentId = 0;
        QString opponentName;
        bool isHome = false;
        int goalsFor = 0;
        int goalsAgainst = 0;
        QString resultLetter;
    };

    enum Role {
        DateTextRole = Qt::UserRole + 1,
        MatchweekRole,
        OpponentIdRole,
        OpponentNameRole,
        IsHomeRole,
        GoalsForRole,
        GoalsAgainstRole,
        ResultLetterRole,
    };

    explicit TeamRecentMatchesModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QVector<Row> rows);
    void clear();

private:
    QVector<Row> rows;
};
