#include"fm/match/PlayerConditionState.h"

namespace {
    constexpr int kMinimumConditionValue = 0;
    constexpr int kMaximumConditionValue = 100;

    constexpr int kDefaultForm = 65;
    constexpr int kDefaultFitness = 100;
    constexpr int kDefaultMorale = 70;
}

PlayerConditionState::PlayerConditionState()
    : form(kDefaultForm), fitness(kDefaultFitness), morale(kDefaultMorale) {
}

PlayerConditionState::PlayerConditionState(int initialForm, int initialFitness, int initialMorale)
    : form(clampToRange(initialForm)),
    fitness(clampToRange(initialFitness)),
    morale(clampToRange(initialMorale)) {
}

int PlayerConditionState::clampToRange(int value) {
    if (value < kMinimumConditionValue) {
        return kMinimumConditionValue;
    }
    if (value > kMaximumConditionValue) {
        return kMaximumConditionValue;
    }
    return value;
}

int PlayerConditionState::getForm() const {
    return form;
}

int PlayerConditionState::getFitness() const {
    return fitness;
}

int PlayerConditionState::getMorale() const {
    return morale;
}

void PlayerConditionState::setForm(int value) {
    form = clampToRange(value);
}

void PlayerConditionState::setFitness(int value) {
    fitness = clampToRange(value);
}

void PlayerConditionState::setMorale(int value) {
    morale = clampToRange(value);
}

void PlayerConditionState::adjustForm(int delta) {
    setForm(form + delta);
}

void PlayerConditionState::adjustFitness(int delta) {
    setFitness(fitness + delta);
}

void PlayerConditionState::adjustMorale(int delta) {
    setMorale(morale + delta);
}

void PlayerConditionState::resetToDefaults() {
    form = kDefaultForm;
    fitness = kDefaultFitness;
    morale = kDefaultMorale;
}