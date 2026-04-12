#include "DashboardStandingsRowObject.h"

DashboardStandingsRowObject::DashboardStandingsRowObject(QObject* parent)
    : QObject(parent) {
}

bool DashboardStandingsRowObject::hasData() const {
    return hasDataValue;
}

int DashboardStandingsRowObject::position() const {
    return positionValue;
}

QString DashboardStandingsRowObject::teamName() const {
    return teamNameValue;
}

int DashboardStandingsRowObject::played() const {
    return playedValue;
}

int DashboardStandingsRowObject::wins() const {
    return winsValue;
}

int DashboardStandingsRowObject::draws() const {
    return drawsValue;
}

int DashboardStandingsRowObject::losses() const {
    return lossesValue;
}

int DashboardStandingsRowObject::goalDifference() const {
    return goalDifferenceValue;
}

int DashboardStandingsRowObject::points() const {
    return pointsValue;
}

void DashboardStandingsRowObject::setFromValues(int newPosition,
    const QString& newTeamName,
    int newPlayed,
    int newWins,
    int newDraws,
    int newLosses,
    int newGoalDifference,
    int newPoints) {
    hasDataValue = true;
    positionValue = newPosition;
    teamNameValue = newTeamName;
    playedValue = newPlayed;
    winsValue = newWins;
    drawsValue = newDraws;
    lossesValue = newLosses;
    goalDifferenceValue = newGoalDifference;
    pointsValue = newPoints;
    emit changed();
}

void DashboardStandingsRowObject::clear() {
    hasDataValue = false;
    positionValue = 0;
    teamNameValue.clear();
    playedValue = 0;
    winsValue = 0;
    drawsValue = 0;
    lossesValue = 0;
    goalDifferenceValue = 0;
    pointsValue = 0;
    emit changed();
}
