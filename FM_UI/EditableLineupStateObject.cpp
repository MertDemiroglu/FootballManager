#include "EditableLineupStateObject.h"

EditableLineupStateObject::EditableLineupStateObject(QObject* parent)
    : QObject(parent) {
}

bool EditableLineupStateObject::hasLineup() const {
    return hasLineupValue;
}

int EditableLineupStateObject::teamId() const {
    return teamIdValue;
}

int EditableLineupStateObject::formation() const {
    return formationValue;
}

QString EditableLineupStateObject::formationText() const {
    return formationTextValue;
}

int EditableLineupStateObject::slotCount() const {
    return slotCountValue;
}

int EditableLineupStateObject::rosterCount() const {
    return rosterCountValue;
}

int EditableLineupStateObject::assignedCount() const {
    return assignedCountValue;
}

bool EditableLineupStateObject::isFull() const {
    return isFullValue;
}

void EditableLineupStateObject::setFromValues(bool newHasLineup,
    int newTeamId,
    int newFormation,
    const QString& newFormationText,
    int newSlotCount,
    int newRosterCount,
    int newAssignedCount,
    bool newIsFull) {
    hasLineupValue = newHasLineup;
    teamIdValue = newTeamId;
    formationValue = newFormation;
    formationTextValue = newFormationText;
    slotCountValue = newSlotCount;
    rosterCountValue = newRosterCount;
    assignedCountValue = newAssignedCount;
    isFullValue = newIsFull;
    emit changed();
}

void EditableLineupStateObject::clear() {
    setFromValues(false, 0, 0, QString(), 0, 0, 0, false);
}
