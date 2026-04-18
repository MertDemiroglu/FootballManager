#pragma once

#include"FormationSlotRole.h"

#include<array>
#include<vector>


enum class FormationId {
    FourFourTwo,
    FourThreeThree,
    ThreeFiveTwo
};

struct FormationDefinition {
    FormationId formationId;
    int goalkeeperCount;
    int defenderCount;
    int midfielderCount;
    int forwardCount;
};

const FormationDefinition& getFormationDefinition(FormationId formationId);
const std::array<FormationId, 3>& getSupportedFormationIds();
bool isFormationSupported(FormationId formationId);

const std::vector<FormationSlotRole>& getFormationSlotTemplate(FormationId formationId);
