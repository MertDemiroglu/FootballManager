#pragma once

#include"fm/common/Types.h"
#include"fm/match/TeamSheet.h"
#include"fm/match/TacticalSetup.h"
#include"fm/match_engine/PitchGeometry.h"
#include"fm/roster/FormationSlotRole.h"

#include<vector>

enum class TeamShapePhase {
    InPossession,
    OutOfPossession,
    DefensiveTransition,
    AttackingTransition
};

enum class AttackingDirection {
    HomeToAway,
    AwayToHome
};

enum class ShapeTargetSource {
    FormationBase,
    TacticalAdjustment,
    LocalIntentAdjustment
};

AttackingDirection attackingDirectionFor(bool isHomeTeam);

struct TeamShapeContext {
    TeamId teamId = 0;
    bool isHomeTeam = true;
    bool hasPossession = false;
    TeamShapePhase phase = TeamShapePhase::OutOfPossession;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    TacticalSetup tacticalSetup;
    PitchPoint ballPosition;
};

struct PlayerShapeTarget {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    int slotIndex = -1;
    FormationSlotRole slotRole = FormationSlotRole::Unknown;
    PitchPoint basePosition;
    PitchPoint tacticalPosition;
    PitchPoint finalTarget;
};

class TeamShapeModel {
public:
    std::vector<PlayerShapeTarget> buildTargets(
        const TeamShapeContext& context,
        const TeamSheet& teamSheet) const;
};
