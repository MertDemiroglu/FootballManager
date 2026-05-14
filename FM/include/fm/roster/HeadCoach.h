#pragma once

#include"fm/match/TacticalPreferences.h"
#include"fm/roster/Formation.h"
#include"fm/common/Types.h"

#include <string>


class HeadCoach {
private:
    CoachId id;
    std::string name;
    TacticalPreferences tacticalPreferences;

public:
    explicit HeadCoach(const std::string& name, FormationId preferredFormation);
    HeadCoach(CoachId id, const std::string& name, FormationId preferredFormation);

    CoachId getId() const;
    const std::string& getName() const;
    FormationId getPreferredFormation() const;
    TeamMentality getPreferredMentality() const;
    TeamTempo getPreferredTempo() const;
    const TacticalPreferences& getTacticalPreferences() const;

    void setPreferredFormation(FormationId formationId);
    void setTacticalPreferences(TacticalPreferences preferences);
};
