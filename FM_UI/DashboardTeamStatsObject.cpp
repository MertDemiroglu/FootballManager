#include"DashboardTeamStatsObject.h"

#include"fm/competition/League.h"

DashboardTeamStatsObject::DashboardTeamStatsObject(QObject* parent)
    : QObject(parent) {
}

int DashboardTeamStatsObject::played() const {
    return playedValue;
}

int DashboardTeamStatsObject::wins() const {
    return winsValue;
}

int DashboardTeamStatsObject::draws() const {
    return drawsValue;
}

int DashboardTeamStatsObject::losses() const {
    return lossesValue;
}

int DashboardTeamStatsObject::goalsFor() const {
    return goalsForValue;
}

int DashboardTeamStatsObject::goalsAgainst() const {
    return goalsAgainstValue;
}

int DashboardTeamStatsObject::goalDifference() const {
    return goalDifferenceValue;
}

int DashboardTeamStatsObject::points() const {
    return pointsValue;
}

void DashboardTeamStatsObject::setFrom(const TeamSeasonStats& stats) {
    playedValue = stats.played;
    winsValue = stats.wins;
    drawsValue = stats.draws;
    lossesValue = stats.losses;
    goalsForValue = stats.goalsFor;
    goalsAgainstValue = stats.goalsAgainst;
    goalDifferenceValue = stats.goalsFor - stats.goalsAgainst;
    pointsValue = stats.points;
    emit changed();
}

void DashboardTeamStatsObject::clear() {
    playedValue = 0;
    winsValue = 0;
    drawsValue = 0;
    lossesValue = 0;
    goalsForValue = 0;
    goalsAgainstValue = 0;
    goalDifferenceValue = 0;
    pointsValue = 0;
    emit changed();
}
