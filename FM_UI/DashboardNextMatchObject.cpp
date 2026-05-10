#include "DashboardNextMatchObject.h"

DashboardNextMatchObject::DashboardNextMatchObject(QObject* parent)
    : QObject(parent) {
}

bool DashboardNextMatchObject::hasNextMatch() const {
    return hasNextMatchValue;
}

QString DashboardNextMatchObject::dateText() const {
    return dateTextValue;
}

QString DashboardNextMatchObject::homeTeamName() const {
    return homeTeamNameValue;
}

QString DashboardNextMatchObject::homePrimaryColor() const {
    return homePrimaryColorValue;
}

QString DashboardNextMatchObject::homeSecondaryColor() const {
    return homeSecondaryColorValue;
}

QString DashboardNextMatchObject::homeTextColor() const {
    return homeTextColorValue;
}

QString DashboardNextMatchObject::awayTeamName() const {
    return awayTeamNameValue;
}

QString DashboardNextMatchObject::awayPrimaryColor() const {
    return awayPrimaryColorValue;
}

QString DashboardNextMatchObject::awaySecondaryColor() const {
    return awaySecondaryColorValue;
}

QString DashboardNextMatchObject::awayTextColor() const {
    return awayTextColorValue;
}

bool DashboardNextMatchObject::isHome() const {
    return isHomeValue;
}

int DashboardNextMatchObject::matchweek() const {
    return matchweekValue;
}

void DashboardNextMatchObject::setFromValues(const QString& newDateText,
    const QString& newHomeTeamName,
    const QString& newHomePrimaryColor,
    const QString& newHomeSecondaryColor,
    const QString& newHomeTextColor,
    const QString& newAwayTeamName,
    const QString& newAwayPrimaryColor,
    const QString& newAwaySecondaryColor,
    const QString& newAwayTextColor,
    bool newIsHome,
    int newMatchweek) {
    hasNextMatchValue = true;
    dateTextValue = newDateText;
    homeTeamNameValue = newHomeTeamName;
    homePrimaryColorValue = newHomePrimaryColor;
    homeSecondaryColorValue = newHomeSecondaryColor;
    homeTextColorValue = newHomeTextColor;
    awayTeamNameValue = newAwayTeamName;
    awayPrimaryColorValue = newAwayPrimaryColor;
    awaySecondaryColorValue = newAwaySecondaryColor;
    awayTextColorValue = newAwayTextColor;
    isHomeValue = newIsHome;
    matchweekValue = newMatchweek;
    emit changed();
}

void DashboardNextMatchObject::clear() {
    hasNextMatchValue = false;
    dateTextValue.clear();
    homeTeamNameValue.clear();
    homePrimaryColorValue.clear();
    homeSecondaryColorValue.clear();
    homeTextColorValue.clear();
    awayTeamNameValue.clear();
    awayPrimaryColorValue.clear();
    awaySecondaryColorValue.clear();
    awayTextColorValue.clear();
    isHomeValue = false;
    matchweekValue = 0;
    emit changed();
}
