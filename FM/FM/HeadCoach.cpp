#include "HeadCoach.h"

#include <atomic>
#include <stdexcept>

namespace {
    CoachId generateCoachId() {
        static std::atomic<CoachId> nextId{ 1 };
        return nextId++;
    }
}

HeadCoach::HeadCoach(const std::string& name, FormationId preferredFormation)
    : HeadCoach(generateCoachId(), name, preferredFormation) {
}

HeadCoach::HeadCoach(CoachId id, const std::string& name, FormationId preferredFormation)
    : id(id), name(name), preferredFormation(preferredFormation) {
    if (id == 0) {
        throw std::invalid_argument("coach id cannot be zero");
    }

    if (name.empty()) {
        throw std::invalid_argument("coach name cannot be empty");
    }

    if (!isFormationSupported(preferredFormation)) {
        throw std::invalid_argument("preferred formation is not supported");
    }
}

CoachId HeadCoach::getId() const {
    return id;
}

const std::string& HeadCoach::getName() const {
    return name;
}

FormationId HeadCoach::getPreferredFormation() const {
    return preferredFormation;
}

void HeadCoach::setPreferredFormation(FormationId formationId) {
    if (!isFormationSupported(formationId)) {
        throw std::invalid_argument("preferred formation is not supported");
    }

    preferredFormation = formationId;
}
