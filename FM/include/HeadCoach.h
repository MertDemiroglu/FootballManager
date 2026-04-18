#pragma once

#include"Formation.h"
#include"fm/common/Types.h"

#include <string>


class HeadCoach {
private:
    CoachId id;
    std::string name;
    FormationId preferredFormation;

public:
    explicit HeadCoach(const std::string& name, FormationId preferredFormation);
    HeadCoach(CoachId id, const std::string& name, FormationId preferredFormation);

    CoachId getId() const;
    const std::string& getName() const;
    FormationId getPreferredFormation() const;

    void setPreferredFormation(FormationId formationId);
};
