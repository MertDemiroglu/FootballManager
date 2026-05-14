#include"fm/roster/HeadCoach.h"

#include<atomic>
#include<stdexcept>

namespace {
    std::atomic<CoachId>& getNextCoachIdCounter() {
        static std::atomic<CoachId> nextId{ 1 };
        return nextId;
    }

    CoachId generateCoachId() {
        return getNextCoachIdCounter().fetch_add(1);
    }

    void reserveCoachId(CoachId id) {
        auto& nextId = getNextCoachIdCounter();
        CoachId desiredNextId = id + 1;
        CoachId currentNextId = nextId.load();

        while (currentNextId < desiredNextId && !nextId.compare_exchange_weak(currentNextId, desiredNextId)) {
        }
    }
}

HeadCoach::HeadCoach(const std::string& name, FormationId preferredFormation)
    : HeadCoach(generateCoachId(), name, preferredFormation) {
}

HeadCoach::HeadCoach(CoachId id, const std::string& name, FormationId preferredFormation)
    : id(id), name(name), tacticalPreferences{} {
    if (id == 0) {
        throw std::invalid_argument("coach id cannot be zero");
    }

    if (name.empty()) {
        throw std::invalid_argument("coach name cannot be empty");
    }

    if (!isFormationSupported(preferredFormation)) {
        throw std::invalid_argument("preferred formation is not supported");
    }

    // TODO: Coach tactical profile should later come from SQL/coach data and
    // include tactical tendencies beyond formation, mentality, and tempo.
    tacticalPreferences.preferredFormation = preferredFormation;
    tacticalPreferences.preferredMentality = TeamMentality::Balanced;
    tacticalPreferences.preferredTempo = TeamTempo::Normal;

    reserveCoachId(id);
}

CoachId HeadCoach::getId() const {
    return id;
}

const std::string& HeadCoach::getName() const {
    return name;
}

FormationId HeadCoach::getPreferredFormation() const {
    return tacticalPreferences.preferredFormation;
}

TeamMentality HeadCoach::getPreferredMentality() const {
    return tacticalPreferences.preferredMentality;
}

TeamTempo HeadCoach::getPreferredTempo() const {
    return tacticalPreferences.preferredTempo;
}

const TacticalPreferences& HeadCoach::getTacticalPreferences() const {
    return tacticalPreferences;
}

void HeadCoach::setPreferredFormation(FormationId formationId) {
    if (!isFormationSupported(formationId)) {
        throw std::invalid_argument("preferred formation is not supported");
    }

    tacticalPreferences.preferredFormation = formationId;
}

void HeadCoach::setTacticalPreferences(TacticalPreferences preferences) {
    if (!isFormationSupported(preferences.preferredFormation)) {
        throw std::invalid_argument("preferred formation is not supported");
    }

    tacticalPreferences = preferences;
}
