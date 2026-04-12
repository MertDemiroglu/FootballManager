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

QString DashboardNextMatchObject::awayTeamName() const {
    return awayTeamNameValue;
}

bool DashboardNextMatchObject::isHome() const {
    return isHomeValue;
}

int DashboardNextMatchObject::matchweek() const {
    return matchweekValue;
}

void DashboardNextMatchObject::setFromValues(const QString& newDateText,
    const QString& newHomeTeamName,
    const QString& newAwayTeamName,
    bool newIsHome,
    int newMatchweek) {
    hasNextMatchValue = true;
    dateTextValue = newDateText;
    homeTeamNameValue = newHomeTeamName;
    awayTeamNameValue = newAwayTeamName;
    isHomeValue = newIsHome;
    matchweekValue = newMatchweek;
    emit changed();
}

void DashboardNextMatchObject::clear() {
    hasNextMatchValue = false;
    dateTextValue.clear();
    homeTeamNameValue.clear();
    awayTeamNameValue.clear();
    isHomeValue = false;
    matchweekValue = 0;
    emit changed();
}
