#include "PostMatchInteractionObject.h"

PostMatchInteractionObject::PostMatchInteractionObject(QObject* parent)
    : QObject(parent) {
}

bool PostMatchInteractionObject::hasData() const {
    return hasDataValue;
}

qulonglong PostMatchInteractionObject::matchId() const {
    return matchIdValue;
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

QString PostMatchInteractionObject::scoreLine() const {
    return QStringLiteral("%1 %2 - %3 %4")
        .arg(homeTeamNameValue.isEmpty() ? QStringLiteral("Home") : homeTeamNameValue)
        .arg(homeGoalsValue)
        .arg(awayGoalsValue)
        .arg(awayTeamNameValue.isEmpty() ? QStringLiteral("Away") : awayTeamNameValue);
}

QString PostMatchInteractionObject::scorerSummary() const {
    return scorerSummaryValue;
}

QString PostMatchInteractionObject::assistSummary() const {
    return assistSummaryValue;
}

QString PostMatchInteractionObject::cardSummary() const {
    return cardSummaryValue;
}

QString PostMatchInteractionObject::homeCoachName() const {
    return homeCoachNameValue;
}

QString PostMatchInteractionObject::awayCoachName() const {
    return awayCoachNameValue;
}

QString PostMatchInteractionObject::homeFormationText() const {
    return homeFormationTextValue;
}

QString PostMatchInteractionObject::awayFormationText() const {
    return awayFormationTextValue;
}

QVariantList PostMatchInteractionObject::homeLineup() const {
    return homeLineupValue;
}

QVariantList PostMatchInteractionObject::awayLineup() const {
    return awayLineupValue;
}

void PostMatchInteractionObject::setFromValues(qulonglong newMatchId,
    const QString& newDateText,
    int newMatchweek,
    int newHomeTeamId,
    int newAwayTeamId,
    const QString& newHomeTeamName,
    const QString& newAwayTeamName,
    int newHomeGoals,
    int newAwayGoals,
    const QString& newScorerSummary,
    const QString& newAssistSummary,
    const QString& newCardSummary,
    const QString& newHomeCoachName,
    const QString& newAwayCoachName,
    const QString& newHomeFormationText,
    const QString& newAwayFormationText,
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
        && homeGoalsValue == newHomeGoals
        && awayGoalsValue == newAwayGoals
        && scorerSummaryValue == newScorerSummary
        && assistSummaryValue == newAssistSummary
        && cardSummaryValue == newCardSummary
        && homeCoachNameValue == newHomeCoachName
        && awayCoachNameValue == newAwayCoachName
        && homeFormationTextValue == newHomeFormationText
        && awayFormationTextValue == newAwayFormationText
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
    homeGoalsValue = newHomeGoals;
    awayGoalsValue = newAwayGoals;
    scorerSummaryValue = newScorerSummary;
    assistSummaryValue = newAssistSummary;
    cardSummaryValue = newCardSummary;
    homeCoachNameValue = newHomeCoachName;
    awayCoachNameValue = newAwayCoachName;
    homeFormationTextValue = newHomeFormationText;
    awayFormationTextValue = newAwayFormationText;
    homeLineupValue = newHomeLineup;
    awayLineupValue = newAwayLineup;
    emit changed();
}

void PostMatchInteractionObject::clear() {
    if (!hasDataValue
        && matchIdValue == 0
        && dateTextValue.isEmpty()
        && matchweekValue == 0
        && homeTeamIdValue == 0
        && awayTeamIdValue == 0
        && homeTeamNameValue.isEmpty()
        && awayTeamNameValue.isEmpty()
        && homeGoalsValue == 0
        && awayGoalsValue == 0
        && scorerSummaryValue.isEmpty()
        && assistSummaryValue.isEmpty()
        && cardSummaryValue.isEmpty()
        && homeCoachNameValue.isEmpty()
        && awayCoachNameValue.isEmpty()
        && homeFormationTextValue.isEmpty()
        && awayFormationTextValue.isEmpty()
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
    homeGoalsValue = 0;
    awayGoalsValue = 0;
    scorerSummaryValue.clear();
    assistSummaryValue.clear();
    cardSummaryValue.clear();
    homeCoachNameValue.clear();
    awayCoachNameValue.clear();
    homeFormationTextValue.clear();
    awayFormationTextValue.clear();
    homeLineupValue.clear();
    awayLineupValue.clear();
    emit changed();
}
