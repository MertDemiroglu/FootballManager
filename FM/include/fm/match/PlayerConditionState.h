#pragma once

class PlayerConditionState {
private:
    int form;
    int fitness;
    int morale;

    static int clampToRange(int value);

public:
    PlayerConditionState();
    PlayerConditionState(int form, int fitness, int morale);

    int getForm() const;
    int getFitness() const;
    int getMorale() const;

    void setForm(int value);
    void setFitness(int value);
    void setMorale(int value);

    void adjustForm(int delta);
    void adjustFitness(int delta);
    void adjustMorale(int delta);

    void resetToDefaults();
};