#include "PreMatchInteractionObject.h"

PreMatchInteractionObject::PreMatchInteractionObject(QObject* parent)
    : QObject(parent) {
}

bool PreMatchInteractionObject::hasData() const {
    return hasDataValue;
}

qulonglong PreMatchInteractionObject::matchId() const {
    return matchIdValue;
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

QString PreMatchInteractionObject::fixtureLabel() const {
    return QStringLiteral("%1 vs %2")
        .arg(homeTeamNameValue.isEmpty() ? QStringLiteral("Home") : homeTeamNameValue)
        .arg(awayTeamNameValue.isEmpty() ? QStringLiteral("Away") : awayTeamNameValue);
}

void PreMatchInteractionObject::setFromValues(qulonglong newMatchId,
    const QString& newDateText,
    int newMatchweek,
    int newHomeTeamId,
    int newAwayTeamId,
    const QString& newHomeTeamName,
    const QString& newAwayTeamName) {
    if (hasDataValue
        && matchIdValue == newMatchId
        && dateTextValue == newDateText
        && matchweekValue == newMatchweek
        && homeTeamIdValue == newHomeTeamId
        && awayTeamIdValue == newAwayTeamId
        && homeTeamNameValue == newHomeTeamName
        && awayTeamNameValue == newAwayTeamName) {
        return;
    }

    hasDataValue = true;
    matchIdValue = newMatchId;
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
        && matchIdValue == 0
        && dateTextValue.isEmpty()
        && matchweekValue == 0
        && homeTeamIdValue == 0
        && awayTeamIdValue == 0
        && homeTeamNameValue.isEmpty()
        && awayTeamNameValue.isEmpty()) {
        return;
    }

    hasDataValue = false;
    matchIdValue = 0;
    dateTextValue.clear();
    matchweekValue = 0;
    homeTeamIdValue = 0;
    awayTeamIdValue = 0;
    homeTeamNameValue.clear();
    awayTeamNameValue.clear();
    emit changed();
}
