#include "DashboardStateObject.h"

DashboardStateObject::DashboardStateObject(QObject* parent)
    : QObject(parent),
      nextMatchObject(this),
      teamStatsObject(this),
      standingsRowObject(this) {
}

bool DashboardStateObject::hasData() const {
    return hasDataValue;
}

QString DashboardStateObject::selectedTeamName() const {
    return selectedTeamNameValue;
}

QString DashboardStateObject::currentDateText() const {
    return currentDateTextValue;
}

QString DashboardStateObject::gameStateText() const {
    return gameStateTextValue;
}

QString DashboardStateObject::managerName() const {
    return managerNameValue;
}

QString DashboardStateObject::recentForm() const {
    return recentFormValue;
}

DashboardNextMatchObject* DashboardStateObject::nextMatch() const {
    return const_cast<DashboardNextMatchObject*>(&nextMatchObject);
}

DashboardTeamStatsObject* DashboardStateObject::teamStats() const {
    return const_cast<DashboardTeamStatsObject*>(&teamStatsObject);
}

DashboardStandingsRowObject* DashboardStateObject::standingsRow() const {
    return const_cast<DashboardStandingsRowObject*>(&standingsRowObject);
}

void DashboardStateObject::setRootValues(bool newHasData,
    const QString& newSelectedTeamName,
    const QString& newCurrentDateText,
    const QString& newGameStateText,
    const QString& newManagerName,
    const QString& newRecentForm) {
    hasDataValue = newHasData;
    selectedTeamNameValue = newSelectedTeamName;
    currentDateTextValue = newCurrentDateText;
    gameStateTextValue = newGameStateText;
    managerNameValue = newManagerName;
    recentFormValue = newRecentForm;
    emit changed();
}

void DashboardStateObject::clear() {
    setRootValues(false, QString(), QString(), QString(), QString(), QString());
    nextMatchObject.clear();
    teamStatsObject.clear();
    standingsRowObject.clear();
}
