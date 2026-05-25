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

QString PostMatchInteractionObject::homePrimaryColor() const {
    return homePrimaryColorValue;
}

QString PostMatchInteractionObject::homeSecondaryColor() const {
    return homeSecondaryColorValue;
}

QString PostMatchInteractionObject::homeTextColor() const {
    return homeTextColorValue;
}

QString PostMatchInteractionObject::awayPrimaryColor() const {
    return awayPrimaryColorValue;
}

QString PostMatchInteractionObject::awaySecondaryColor() const {
    return awaySecondaryColorValue;
}

QString PostMatchInteractionObject::awayTextColor() const {
    return awayTextColorValue;
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

QString PostMatchInteractionObject::homeAverageOverallText() const {
    return homeAverageOverallTextValue;
}

QString PostMatchInteractionObject::awayAverageOverallText() const {
    return awayAverageOverallTextValue;
}

QVariantList PostMatchInteractionObject::homeLineup() const {
    return homeLineupValue;
}

QVariantList PostMatchInteractionObject::awayLineup() const {
    return awayLineupValue;
}

QVariantList PostMatchInteractionObject::statRows() const {
    return statRowsValue;
}

void PostMatchInteractionObject::setFromValues(qulonglong newMatchId,
    const QString& newDateText,
    int newMatchweek,
    int newHomeTeamId,
    int newAwayTeamId,
    const QString& newHomeTeamName,
    const QString& newAwayTeamName,
    const QString& newHomePrimaryColor,
    const QString& newHomeSecondaryColor,
    const QString& newHomeTextColor,
    const QString& newAwayPrimaryColor,
    const QString& newAwaySecondaryColor,
    const QString& newAwayTextColor,
    int newHomeGoals,
    int newAwayGoals,
    const QString& newScorerSummary,
    const QString& newAssistSummary,
    const QString& newCardSummary,
    const QString& newHomeCoachName,
    const QString& newAwayCoachName,
    const QString& newHomeFormationText,
    const QString& newAwayFormationText,
    const QString& newHomeAverageOverallText,
    const QString& newAwayAverageOverallText,
    const QVariantList& newHomeLineup,
    const QVariantList& newAwayLineup,
    const QVariantList& newStatRows) {
    if (hasDataValue
        && matchIdValue == newMatchId
        && dateTextValue == newDateText
        && matchweekValue == newMatchweek
        && homeTeamIdValue == newHomeTeamId
        && awayTeamIdValue == newAwayTeamId
        && homeTeamNameValue == newHomeTeamName
        && awayTeamNameValue == newAwayTeamName
        && homePrimaryColorValue == newHomePrimaryColor
        && homeSecondaryColorValue == newHomeSecondaryColor
        && homeTextColorValue == newHomeTextColor
        && awayPrimaryColorValue == newAwayPrimaryColor
        && awaySecondaryColorValue == newAwaySecondaryColor
        && awayTextColorValue == newAwayTextColor
        && homeGoalsValue == newHomeGoals
        && awayGoalsValue == newAwayGoals
        && scorerSummaryValue == newScorerSummary
        && assistSummaryValue == newAssistSummary
        && cardSummaryValue == newCardSummary
        && homeCoachNameValue == newHomeCoachName
        && awayCoachNameValue == newAwayCoachName
        && homeFormationTextValue == newHomeFormationText
        && awayFormationTextValue == newAwayFormationText
        && homeAverageOverallTextValue == newHomeAverageOverallText
        && awayAverageOverallTextValue == newAwayAverageOverallText
        && homeLineupValue == newHomeLineup
        && awayLineupValue == newAwayLineup
        && statRowsValue == newStatRows) {
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
    homePrimaryColorValue = newHomePrimaryColor;
    homeSecondaryColorValue = newHomeSecondaryColor;
    homeTextColorValue = newHomeTextColor;
    awayPrimaryColorValue = newAwayPrimaryColor;
    awaySecondaryColorValue = newAwaySecondaryColor;
    awayTextColorValue = newAwayTextColor;
    homeGoalsValue = newHomeGoals;
    awayGoalsValue = newAwayGoals;
    scorerSummaryValue = newScorerSummary;
    assistSummaryValue = newAssistSummary;
    cardSummaryValue = newCardSummary;
    homeCoachNameValue = newHomeCoachName;
    awayCoachNameValue = newAwayCoachName;
    homeFormationTextValue = newHomeFormationText;
    awayFormationTextValue = newAwayFormationText;
    homeAverageOverallTextValue = newHomeAverageOverallText;
    awayAverageOverallTextValue = newAwayAverageOverallText;
    homeLineupValue = newHomeLineup;
    awayLineupValue = newAwayLineup;
    statRowsValue = newStatRows;
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
        && homePrimaryColorValue == QStringLiteral("#22c55e")
        && homeSecondaryColorValue == QStringLiteral("#0f172a")
        && homeTextColorValue == QStringLiteral("#f8fafc")
        && awayPrimaryColorValue == QStringLiteral("#22c55e")
        && awaySecondaryColorValue == QStringLiteral("#0f172a")
        && awayTextColorValue == QStringLiteral("#f8fafc")
        && homeGoalsValue == 0
        && awayGoalsValue == 0
        && scorerSummaryValue.isEmpty()
        && assistSummaryValue.isEmpty()
        && cardSummaryValue.isEmpty()
        && homeCoachNameValue.isEmpty()
        && awayCoachNameValue.isEmpty()
        && homeFormationTextValue.isEmpty()
        && awayFormationTextValue.isEmpty()
        && homeAverageOverallTextValue == QStringLiteral("--")
        && awayAverageOverallTextValue == QStringLiteral("--")
        && homeLineupValue.isEmpty()
        && awayLineupValue.isEmpty()
        && statRowsValue.isEmpty()) {
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
    homePrimaryColorValue = QStringLiteral("#22c55e");
    homeSecondaryColorValue = QStringLiteral("#0f172a");
    homeTextColorValue = QStringLiteral("#f8fafc");
    awayPrimaryColorValue = QStringLiteral("#22c55e");
    awaySecondaryColorValue = QStringLiteral("#0f172a");
    awayTextColorValue = QStringLiteral("#f8fafc");
    homeGoalsValue = 0;
    awayGoalsValue = 0;
    scorerSummaryValue.clear();
    assistSummaryValue.clear();
    cardSummaryValue.clear();
    homeCoachNameValue.clear();
    awayCoachNameValue.clear();
    homeFormationTextValue.clear();
    awayFormationTextValue.clear();
    homeAverageOverallTextValue = QStringLiteral("--");
    awayAverageOverallTextValue = QStringLiteral("--");
    homeLineupValue.clear();
    awayLineupValue.clear();
    statRowsValue.clear();
    emit changed();
}
