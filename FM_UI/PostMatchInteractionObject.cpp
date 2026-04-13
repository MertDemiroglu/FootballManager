#include "PostMatchInteractionObject.h"

PostMatchInteractionObject::PostMatchInteractionObject(QObject* parent)
    : QObject(parent) {
}

bool PostMatchInteractionObject::hasData() const {
    return hasDataValue;
}

QString PostMatchInteractionObject::dateText() const {
    return dateTextValue;
}

int PostMatchInteractionObject::matchweek() const {
    return matchweekValue;
}

int PostMatchInteractionObject::homeTeamId() const {
    return homeTeamIdValue;
}

int PostMatchInteractionObject::awayTeamId() const {
    return awayTeamIdValue;
}

QString PostMatchInteractionObject::homeTeamName() const {
    return homeTeamNameValue;
}

QString PostMatchInteractionObject::awayTeamName() const {
    return awayTeamNameValue;
}

int PostMatchInteractionObject::homeGoals() const {
    return homeGoalsValue;
}

int PostMatchInteractionObject::awayGoals() const {
    return awayGoalsValue;
}

void PostMatchInteractionObject::setFromValues(const QString& newDateText,
    int newMatchweek,
    int newHomeTeamId,
    int newAwayTeamId,
    const QString& newHomeTeamName,
    const QString& newAwayTeamName,
    int newHomeGoals,
    int newAwayGoals) {
    if (hasDataValue
        && dateTextValue == newDateText
        && matchweekValue == newMatchweek
        && homeTeamIdValue == newHomeTeamId
        && awayTeamIdValue == newAwayTeamId
        && homeTeamNameValue == newHomeTeamName
        && awayTeamNameValue == newAwayTeamName
        && homeGoalsValue == newHomeGoals
        && awayGoalsValue == newAwayGoals) {
        return;
    }

    hasDataValue = true;
    dateTextValue = newDateText;
    matchweekValue = newMatchweek;
    homeTeamIdValue = newHomeTeamId;
    awayTeamIdValue = newAwayTeamId;
    homeTeamNameValue = newHomeTeamName;
    awayTeamNameValue = newAwayTeamName;
    homeGoalsValue = newHomeGoals;
    awayGoalsValue = newAwayGoals;
    emit changed();
}

void PostMatchInteractionObject::clear() {
    if (!hasDataValue
        && dateTextValue.isEmpty()
        && matchweekValue == 0
        && homeTeamIdValue == 0
        && awayTeamIdValue == 0
        && homeTeamNameValue.isEmpty()
        && awayTeamNameValue.isEmpty()
        && homeGoalsValue == 0
        && awayGoalsValue == 0) {
        return;
    }

    hasDataValue = false;
    dateTextValue.clear();
    matchweekValue = 0;
    homeTeamIdValue = 0;
    awayTeamIdValue = 0;
    homeTeamNameValue.clear();
    awayTeamNameValue.clear();
    homeGoalsValue = 0;
    awayGoalsValue = 0;
    emit changed();
}
