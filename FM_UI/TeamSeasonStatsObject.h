#pragma once

#include <QObject>

struct TeamSeasonStats;

class TeamSeasonStatsObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(TeamSeasonStatsObject)

    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(int played READ played NOTIFY changed)
    Q_PROPERTY(int wins READ wins NOTIFY changed)
    Q_PROPERTY(int draws READ draws NOTIFY changed)
    Q_PROPERTY(int losses READ losses NOTIFY changed)
    Q_PROPERTY(int goalsFor READ goalsFor NOTIFY changed)
    Q_PROPERTY(int goalsAgainst READ goalsAgainst NOTIFY changed)
    Q_PROPERTY(int cleanSheets READ cleanSheets NOTIFY changed)
    Q_PROPERTY(int failedToScore READ failedToScore NOTIFY changed)

public:
    explicit TeamSeasonStatsObject(QObject* parent = nullptr);

    bool hasData() const;
    int played() const;
    int wins() const;
    int draws() const;
    int losses() const;
    int goalsFor() const;
    int goalsAgainst() const;
    int cleanSheets() const;
    int failedToScore() const;

    void setFrom(const TeamSeasonStats& stats);
    void clear();

signals:
    void changed();

private:
    bool hasDataValue = false;
    int playedValue = 0;
    int winsValue = 0;
    int drawsValue = 0;
    int lossesValue = 0;
    int goalsForValue = 0;
    int goalsAgainstValue = 0;
    int cleanSheetsValue = 0;
    int failedToScoreValue = 0;
};
