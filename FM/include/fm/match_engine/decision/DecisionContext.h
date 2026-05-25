#pragma once

#include"fm/common/Types.h"
#include"fm/match/TacticalSetup.h"
#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/roster/FormationSlotRole.h"

#include<optional>

enum class DecisionMatchPhase {
    KickOff,
    BuildUp,
    MiddleThirdCirculation,
    Progression,
    FinalThird,
    BoxEntry,
    ChanceCreation,
    DefensiveBlock,
    AttackingTransition,
    DefensiveTransition,
    LooseBall,
    Restart
};

struct MatchContext {
    int matchSecond = 0;
    int homeScore = 0;
    int awayScore = 0;
    PitchPoint ballPosition;
    std::optional<TeamId> teamInPossession;
    std::optional<PlayerId> ballCarrierId;
    DecisionMatchPhase phase = DecisionMatchPhase::BuildUp;
};

struct PossessionContext {
    std::optional<TeamId> teamInPossession;
    std::optional<PlayerId> ballCarrierId;
    int possessionActionCount = 0;
    double secondsInPossession = 0.0;
    DecisionMatchPhase currentPhase = DecisionMatchPhase::BuildUp;
    PitchPoint ballPosition;
    double ballProgression = 0.0;
    double recentProgression = 0.0;
    bool progressionAvailable = false;
    bool safeCirculationAvailable = false;
    bool finalThirdEntryAvailable = false;
    bool boxEntryAvailable = false;
    double opponentBlockCompactness = 0.0;
    double localPressure = 0.0;
};

struct DefensiveContext {
    TeamId defendingTeamId = 0;
    TeamId opponentTeamId = 0;
    double secondsOutOfPossession = 0.0;
    int opponentPossessionActionCount = 0;
    DecisionMatchPhase opponentPhase = DecisionMatchPhase::BuildUp;
    PitchPoint ballPosition;
    double threatLevel = 0.0;
    double localPressOpportunity = 0.0;
    double blockCompactness = 0.0;
    double lineIntegrity = 0.0;
    bool pressTriggerActive = false;
    bool counterPressAvailable = false;
    bool dangerInAssignedZone = false;
};

struct TeamDecisionContext {
    TeamId teamId = 0;
    bool inPossession = false;
    TacticalSetup tacticalSetup;
    DecisionMatchPhase phase = DecisionMatchPhase::BuildUp;
    PossessionContext possession;
    DefensiveContext defensive;
};

struct PlayerDecisionContext {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    FormationSlotRole role = FormationSlotRole::Unknown;
    PitchPoint playerPosition;
    PitchPoint ballPosition;
    TacticalSetup tacticalSetup;
    DecisionMatchPhase phase = DecisionMatchPhase::BuildUp;
    PossessionContext possession;
    DefensiveContext defensive;
    double localPressure = 0.0;
};
