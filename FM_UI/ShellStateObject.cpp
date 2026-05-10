#include "ShellStateObject.h"

ShellStateObject::ShellStateObject(QObject* parent)
    : QObject(parent) {
}

bool ShellStateObject::hasStartedGame() const {
    return hasStartedGameValue;
}

QString ShellStateObject::selectedTeamName() const {
    return selectedTeamNameValue;
}

QString ShellStateObject::selectedTeamPrimaryColor() const {
    return selectedTeamPrimaryColorValue;
}

QString ShellStateObject::selectedTeamSecondaryColor() const {
    return selectedTeamSecondaryColorValue;
}

QString ShellStateObject::selectedTeamTextColor() const {
    return selectedTeamTextColorValue;
}

QString ShellStateObject::currentDateText() const {
    return currentDateTextValue;
}

QString ShellStateObject::currentStateText() const {
    return currentStateTextValue;
}

QString ShellStateObject::managerName() const {
    return managerNameValue;
}

bool ShellStateObject::timePaused() const {
    return timePausedValue;
}

void ShellStateObject::setFromValues(bool newHasStartedGame,
    const QString& newSelectedTeamName,
    const QString& newSelectedTeamPrimaryColor,
    const QString& newSelectedTeamSecondaryColor,
    const QString& newSelectedTeamTextColor,
    const QString& newCurrentDateText,
    const QString& newCurrentStateText,
    const QString& newManagerName,
    bool newTimePaused) {
    if (hasStartedGameValue == newHasStartedGame
        && selectedTeamNameValue == newSelectedTeamName
        && selectedTeamPrimaryColorValue == newSelectedTeamPrimaryColor
        && selectedTeamSecondaryColorValue == newSelectedTeamSecondaryColor
        && selectedTeamTextColorValue == newSelectedTeamTextColor
        && currentDateTextValue == newCurrentDateText
        && currentStateTextValue == newCurrentStateText
        && managerNameValue == newManagerName
        && timePausedValue == newTimePaused) {
        return;
    }

    hasStartedGameValue = newHasStartedGame;
    selectedTeamNameValue = newSelectedTeamName;
    selectedTeamPrimaryColorValue = newSelectedTeamPrimaryColor;
    selectedTeamSecondaryColorValue = newSelectedTeamSecondaryColor;
    selectedTeamTextColorValue = newSelectedTeamTextColor;
    currentDateTextValue = newCurrentDateText;
    currentStateTextValue = newCurrentStateText;
    managerNameValue = newManagerName;
    timePausedValue = newTimePaused;
    emit changed();
}

void ShellStateObject::clear() {
    setFromValues(false, QString(), QString(), QString(), QString(), QString(), QString(), QString(), true);
}
