#pragma once

#include <QObject>

struct TeamSeasonStats;

class DashboardTeamStatsObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DashboardTeamStatsObject)

    Q_PROPERTY(int played READ played NOTIFY changed)
    Q_PROPERTY(int wins READ wins NOTIFY changed)
    Q_PROPERTY(int draws READ draws NOTIFY changed)
    Q_PROPERTY(int losses READ losses NOTIFY changed)
    Q_PROPERTY(int goalsFor READ goalsFor NOTIFY changed)
    Q_PROPERTY(int goalsAgainst READ goalsAgainst NOTIFY changed)
    Q_PROPERTY(int goalDifference READ goalDifference NOTIFY changed)
    Q_PROPERTY(int points READ points NOTIFY changed)

public:
    explicit DashboardTeamStatsObject(QObject* parent = nullptr);

    int played() const;
    int wins() const;
    int draws() const;
    int losses() const;
    int goalsFor() const;
    int goalsAgainst() const;
    int goalDifference() const;
    int points() const;

    void setFrom(const TeamSeasonStats& stats);
    void clear();

signals:
    void changed();

private:
    int playedValue = 0;
    int winsValue = 0;
    int drawsValue = 0;
    int lossesValue = 0;
    int goalsForValue = 0;
    int goalsAgainstValue = 0;
    int goalDifferenceValue = 0;
    int pointsValue = 0;
};
