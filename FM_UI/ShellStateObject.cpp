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
    const QString& newCurrentDateText,
    const QString& newCurrentStateText,
    const QString& newManagerName,
    bool newTimePaused) {
    if (hasStartedGameValue == newHasStartedGame
        && selectedTeamNameValue == newSelectedTeamName
        && currentDateTextValue == newCurrentDateText
        && currentStateTextValue == newCurrentStateText
        && managerNameValue == newManagerName
        && timePausedValue == newTimePaused) {
        return;
    }

    hasStartedGameValue = newHasStartedGame;
    selectedTeamNameValue = newSelectedTeamName;
    currentDateTextValue = newCurrentDateText;
    currentStateTextValue = newCurrentStateText;
    managerNameValue = newManagerName;
    timePausedValue = newTimePaused;
    emit changed();
}

void ShellStateObject::clear() {
    setFromValues(false, QString(), QString(), QString(), QString(), true);
}
