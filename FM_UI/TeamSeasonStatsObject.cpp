#include"TeamSeasonStatsObject.h"

#include"fm/competition/League.h"

TeamSeasonStatsObject::TeamSeasonStatsObject(QObject* parent)
    : QObject(parent) {
}

bool TeamSeasonStatsObject::hasData() const {
    return hasDataValue;
}

int TeamSeasonStatsObject::played() const {
    return playedValue;
}

int TeamSeasonStatsObject::wins() const {
    return winsValue;
}

int TeamSeasonStatsObject::draws() const {
    return drawsValue;
}

int TeamSeasonStatsObject::losses() const {
    return lossesValue;
}

int TeamSeasonStatsObject::goalsFor() const {
    return goalsForValue;
}

int TeamSeasonStatsObject::goalsAgainst() const {
    return goalsAgainstValue;
}

int TeamSeasonStatsObject::cleanSheets() const {
    return cleanSheetsValue;
}

int TeamSeasonStatsObject::failedToScore() const {
    return failedToScoreValue;
}

void TeamSeasonStatsObject::setFrom(const TeamSeasonStats& stats) {
    hasDataValue = true;
    playedValue = stats.played;
    winsValue = stats.wins;
    drawsValue = stats.draws;
    lossesValue = stats.losses;
    goalsForValue = stats.goalsFor;
    goalsAgainstValue = stats.goalsAgainst;
    cleanSheetsValue = stats.cleanSheets;
    failedToScoreValue = stats.failedToScore;
    emit changed();
}

void TeamSeasonStatsObject::clear() {
    hasDataValue = false;
    playedValue = 0;
    winsValue = 0;
    drawsValue = 0;
    lossesValue = 0;
    goalsForValue = 0;
    goalsAgainstValue = 0;
    cleanSheetsValue = 0;
    failedToScoreValue = 0;
    emit changed();
}
