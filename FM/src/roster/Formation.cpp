#include"fm/roster/Formation.h"

#include<stdexcept>

namespace {
    constexpr std::array<FormationDefinition, 3> kFormationDefinitions{ {
        { FormationId::FourFourTwo, 1, 4, 4, 2 },
        { FormationId::FourThreeThree, 1, 4, 3, 3 },
        { FormationId::ThreeFiveTwo, 1, 3, 5, 2 }
    } };

    constexpr std::array<FormationId, 3> kSupportedFormationIds{ {
        FormationId::FourFourTwo,
        FormationId::FourThreeThree,
        FormationId::ThreeFiveTwo
    } };

    const std::vector<FormationSlotRole> kFourFourTwoTemplate {
        FormationSlotRole::Goalkeeper,
        FormationSlotRole::LeftBack,
        FormationSlotRole::CenterBack,
        FormationSlotRole::CenterBack,
        FormationSlotRole::RightBack,
        FormationSlotRole::LeftMidfielder,
        FormationSlotRole::CentralMidfielder,
        FormationSlotRole::CentralMidfielder,
        FormationSlotRole::RightMidfielder,
        FormationSlotRole::Striker,
        FormationSlotRole::Striker
    };

    // TODO: support tactical variants such as 4-3-3 with AM, 3-5-2 with 3 CM
    // or double DM, plus 4-2-3-1 and 3-4-2-1 once variant formations exist.
    const std::vector<FormationSlotRole> kFourThreeThreeTemplate {
        FormationSlotRole::Goalkeeper,
        FormationSlotRole::LeftBack,
        FormationSlotRole::CenterBack,
        FormationSlotRole::CenterBack,
        FormationSlotRole::RightBack,
        FormationSlotRole::DefensiveMidfielder,
        FormationSlotRole::CentralMidfielder,
        FormationSlotRole::CentralMidfielder,
        FormationSlotRole::LeftWinger,
        FormationSlotRole::Striker,
        FormationSlotRole::RightWinger
    };

    const std::vector<FormationSlotRole> kThreeFiveTwoTemplate {
        FormationSlotRole::Goalkeeper,
        FormationSlotRole::CenterBack,
        FormationSlotRole::CenterBack,
        FormationSlotRole::CenterBack,
        FormationSlotRole::LeftWingBack,
        FormationSlotRole::DefensiveMidfielder,
        FormationSlotRole::CentralMidfielder,
        FormationSlotRole::CentralMidfielder,
        FormationSlotRole::RightWingBack,
        FormationSlotRole::Striker,
        FormationSlotRole::Striker
    };
}

bool isSlotRoleSupported(FormationSlotRole role) {
    return role != FormationSlotRole::Unknown;
}

const FormationDefinition& getFormationDefinition(FormationId formationId) {
    for (const FormationDefinition& definition : kFormationDefinitions) {
        if (definition.formationId == formationId) {
            return definition;
        }
    }

    throw std::invalid_argument("unsupported formation");
}

const std::array<FormationId, 3>& getSupportedFormationIds() {
    return kSupportedFormationIds;
}

bool isFormationSupported(FormationId formationId) {
    for (FormationId supportedFormation : kSupportedFormationIds) {
        if (supportedFormation == formationId) {
            return true;
        }
    }

    return false;
}

const std::vector<FormationSlotRole>& getFormationSlotTemplate(FormationId formationId) {
    switch (formationId) {
    case FormationId::FourFourTwo:
        return kFourFourTwoTemplate;
    case FormationId::FourThreeThree:
        return kFourThreeThreeTemplate;
    case FormationId::ThreeFiveTwo:
        return kThreeFiveTwoTemplate;
    }

    throw std::invalid_argument("unsupported formation");
}
