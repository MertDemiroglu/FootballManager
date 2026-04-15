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

QString PreMatchInteractionObject::homeCoachName() const {
    return homeCoachNameValue;
}

QString PreMatchInteractionObject::awayCoachName() const {
    return awayCoachNameValue;
}

QString PreMatchInteractionObject::homeFormationText() const {
    return homeFormationTextValue;
}

QString PreMatchInteractionObject::awayFormationText() const {
    return awayFormationTextValue;
}

QString PreMatchInteractionObject::formationText() const {
    return formationTextValue;
}

QVariantList PreMatchInteractionObject::homeLineup() const {
    return homeLineupValue;
}

QVariantList PreMatchInteractionObject::awayLineup() const {
    return awayLineupValue;
}

void PreMatchInteractionObject::setFromValues(qulonglong newMatchId,
    const QString& newDateText,
    int newMatchweek,
    int newHomeTeamId,
    int newAwayTeamId,
    const QString& newHomeTeamName,
    const QString& newAwayTeamName,
    const QString& newHomeCoachName,
    const QString& newAwayCoachName,
    const QString& newHomeFormationText,
    const QString& newAwayFormationText,
    const QString& newFormationText,
    const QVariantList& newHomeLineup,
    const QVariantList& newAwayLineup) {
    if (hasDataValue
        && matchIdValue == newMatchId
        && dateTextValue == newDateText
        && matchweekValue == newMatchweek
        && homeTeamIdValue == newHomeTeamId
        && awayTeamIdValue == newAwayTeamId
        && homeTeamNameValue == newHomeTeamName
        && awayTeamNameValue == newAwayTeamName
        && homeCoachNameValue == newHomeCoachName
        && awayCoachNameValue == newAwayCoachName
        && homeFormationTextValue == newHomeFormationText
        && awayFormationTextValue == newAwayFormationText
        && formationTextValue == newFormationText
        && homeLineupValue == newHomeLineup
        && awayLineupValue == newAwayLineup) {
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
    homeCoachNameValue = newHomeCoachName;
    awayCoachNameValue = newAwayCoachName;
    homeFormationTextValue = newHomeFormationText;
    awayFormationTextValue = newAwayFormationText;
    formationTextValue = newFormationText;
    homeLineupValue = newHomeLineup;
    awayLineupValue = newAwayLineup;
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
        && awayTeamNameValue.isEmpty()
        && homeCoachNameValue.isEmpty()
        && awayCoachNameValue.isEmpty()
        && homeFormationTextValue.isEmpty()
        && awayFormationTextValue.isEmpty()
        && formationTextValue.isEmpty()
        && homeLineupValue.isEmpty()
        && awayLineupValue.isEmpty()) {
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
    homeCoachNameValue.clear();
    awayCoachNameValue.clear();
    homeFormationTextValue.clear();
    awayFormationTextValue.clear();
    formationTextValue.clear();
    homeLineupValue.clear();
    awayLineupValue.clear();
    emit changed();
}
