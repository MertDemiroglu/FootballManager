#include "Formation.h"

#include <stdexcept>

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
