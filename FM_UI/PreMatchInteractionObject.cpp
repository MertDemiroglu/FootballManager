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

QString PreMatchInteractionObject::homePrimaryColor() const {
    return homePrimaryColorValue;
}

QString PreMatchInteractionObject::homeSecondaryColor() const {
    return homeSecondaryColorValue;
}

QString PreMatchInteractionObject::homeTextColor() const {
    return homeTextColorValue;
}

QString PreMatchInteractionObject::awayPrimaryColor() const {
    return awayPrimaryColorValue;
}

QString PreMatchInteractionObject::awaySecondaryColor() const {
    return awaySecondaryColorValue;
}

QString PreMatchInteractionObject::awayTextColor() const {
    return awayTextColorValue;
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

QString PreMatchInteractionObject::homeRecentForm() const {
    return homeRecentFormValue;
}

QString PreMatchInteractionObject::awayRecentForm() const {
    return awayRecentFormValue;
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

QString PreMatchInteractionObject::homeAverageOverallText() const {
    return homeAverageOverallTextValue;
}

QString PreMatchInteractionObject::awayAverageOverallText() const {
    return awayAverageOverallTextValue;
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
    const QString& newHomePrimaryColor,
    const QString& newHomeSecondaryColor,
    const QString& newHomeTextColor,
    const QString& newAwayPrimaryColor,
    const QString& newAwaySecondaryColor,
    const QString& newAwayTextColor,
    const QString& newHomeCoachName,
    const QString& newAwayCoachName,
    const QString& newHomeRecentForm,
    const QString& newAwayRecentForm,
    const QString& newHomeFormationText,
    const QString& newAwayFormationText,
    const QString& newFormationText,
    const QString& newHomeAverageOverallText,
    const QString& newAwayAverageOverallText,
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
        && homePrimaryColorValue == newHomePrimaryColor
        && homeSecondaryColorValue == newHomeSecondaryColor
        && homeTextColorValue == newHomeTextColor
        && awayPrimaryColorValue == newAwayPrimaryColor
        && awaySecondaryColorValue == newAwaySecondaryColor
        && awayTextColorValue == newAwayTextColor
        && homeCoachNameValue == newHomeCoachName
        && awayCoachNameValue == newAwayCoachName
        && homeRecentFormValue == newHomeRecentForm
        && awayRecentFormValue == newAwayRecentForm
        && homeFormationTextValue == newHomeFormationText
        && awayFormationTextValue == newAwayFormationText
        && formationTextValue == newFormationText
        && homeAverageOverallTextValue == newHomeAverageOverallText
        && awayAverageOverallTextValue == newAwayAverageOverallText
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
    homePrimaryColorValue = newHomePrimaryColor;
    homeSecondaryColorValue = newHomeSecondaryColor;
    homeTextColorValue = newHomeTextColor;
    awayPrimaryColorValue = newAwayPrimaryColor;
    awaySecondaryColorValue = newAwaySecondaryColor;
    awayTextColorValue = newAwayTextColor;
    homeCoachNameValue = newHomeCoachName;
    awayCoachNameValue = newAwayCoachName;
    homeRecentFormValue = newHomeRecentForm;
    awayRecentFormValue = newAwayRecentForm;
    homeFormationTextValue = newHomeFormationText;
    awayFormationTextValue = newAwayFormationText;
    formationTextValue = newFormationText;
    homeAverageOverallTextValue = newHomeAverageOverallText;
    awayAverageOverallTextValue = newAwayAverageOverallText;
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
        && homePrimaryColorValue == QStringLiteral("#22c55e")
        && homeSecondaryColorValue == QStringLiteral("#0f172a")
        && homeTextColorValue == QStringLiteral("#f8fafc")
        && awayPrimaryColorValue == QStringLiteral("#22c55e")
        && awaySecondaryColorValue == QStringLiteral("#0f172a")
        && awayTextColorValue == QStringLiteral("#f8fafc")
        && homeCoachNameValue.isEmpty()
        && awayCoachNameValue.isEmpty()
        && homeRecentFormValue.isEmpty()
        && awayRecentFormValue.isEmpty()
        && homeFormationTextValue.isEmpty()
        && awayFormationTextValue.isEmpty()
        && formationTextValue.isEmpty()
        && homeAverageOverallTextValue == QStringLiteral("--")
        && awayAverageOverallTextValue == QStringLiteral("--")
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
    homePrimaryColorValue = QStringLiteral("#22c55e");
    homeSecondaryColorValue = QStringLiteral("#0f172a");
    homeTextColorValue = QStringLiteral("#f8fafc");
    awayPrimaryColorValue = QStringLiteral("#22c55e");
    awaySecondaryColorValue = QStringLiteral("#0f172a");
    awayTextColorValue = QStringLiteral("#f8fafc");
    homeCoachNameValue.clear();
    awayCoachNameValue.clear();
    homeRecentFormValue.clear();
    awayRecentFormValue.clear();
    homeFormationTextValue.clear();
    awayFormationTextValue.clear();
    formationTextValue.clear();
    homeAverageOverallTextValue = QStringLiteral("--");
    awayAverageOverallTextValue = QStringLiteral("--");
    homeLineupValue.clear();
    awayLineupValue.clear();
    emit changed();
}
