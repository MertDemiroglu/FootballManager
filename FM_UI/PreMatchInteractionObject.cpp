#include "PreMatchInteractionObject.h"

PreMatchInteractionObject::PreMatchInteractionObject(QObject* parent)
    : QObject(parent) {
}

bool PreMatchInteractionObject::hasData() const {
    return hasDataValue;
}

QString PreMatchInteractionObject::dateText() const {
    return dateTextValue;
}

int PreMatchInteractionObject::matchweek() const {
    return matchweekValue;
}

int PreMatchInteractionObject::homeTeamId() const {
    return homeTeamIdValue;
}

int PreMatchInteractionObject::awayTeamId() const {
    return awayTeamIdValue;
}

QString PreMatchInteractionObject::homeTeamName() const {
    return homeTeamNameValue;
}

QString PreMatchInteractionObject::awayTeamName() const {
    return awayTeamNameValue;
}

void PreMatchInteractionObject::setFromValues(const QString& newDateText,
    int newMatchweek,
    int newHomeTeamId,
    int newAwayTeamId,
    const QString& newHomeTeamName,
    const QString& newAwayTeamName) {
    if (hasDataValue
        && dateTextValue == newDateText
        && matchweekValue == newMatchweek
        && homeTeamIdValue == newHomeTeamId
        && awayTeamIdValue == newAwayTeamId
        && homeTeamNameValue == newHomeTeamName
        && awayTeamNameValue == newAwayTeamName) {
        return;
    }

    hasDataValue = true;
    dateTextValue = newDateText;
    matchweekValue = newMatchweek;
    homeTeamIdValue = newHomeTeamId;
    awayTeamIdValue = newAwayTeamId;
    homeTeamNameValue = newHomeTeamName;
    awayTeamNameValue = newAwayTeamName;
    emit changed();
}

void PreMatchInteractionObject::clear() {
    if (!hasDataValue
        && dateTextValue.isEmpty()
        && matchweekValue == 0
        && homeTeamIdValue == 0
        && awayTeamIdValue == 0
        && homeTeamNameValue.isEmpty()
        && awayTeamNameValue.isEmpty()) {
        return;
    }

    hasDataValue = false;
    dateTextValue.clear();
    matchweekValue = 0;
    homeTeamIdValue = 0;
    awayTeamIdValue = 0;
    homeTeamNameValue.clear();
    awayTeamNameValue.clear();
    emit changed();
}
