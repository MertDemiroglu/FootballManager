#pragma once

#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/match_engine/phase/MatchPhaseTypes.h"
#include"fm/roster/FormationSlotRole.h"

#include<array>

enum class PhaseDecisionIntent {
    Circulate,
    Progress,
    Create,
    Finish,
    CounterProgress,
    Recycle
};

enum class PhaseDecisionRoleBucket {
    CenterBack,
    Fullback,
    DefensiveOrCentralMidfielder,
    Winger,
    Striker,
    Other
};

constexpr int PhaseDecisionRoleBucketCount = 6;

inline int phaseDecisionRoleBucketIndex(PhaseDecisionRoleBucket bucket) {
    return static_cast<int>(bucket);
}

inline const char* phaseDecisionRoleBucketName(PhaseDecisionRoleBucket bucket) {
    switch (bucket) {
    case PhaseDecisionRoleBucket::CenterBack:
        return "CB";
    case PhaseDecisionRoleBucket::Fullback:
        return "FB";
    case PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder:
        return "DMCM";
    case PhaseDecisionRoleBucket::Winger:
        return "Winger";
    case PhaseDecisionRoleBucket::Striker:
        return "ST";
    case PhaseDecisionRoleBucket::Other:
        return "Other";
    }
    return "Other";
}

inline PhaseDecisionRoleBucket phaseDecisionRoleBucket(FormationSlotRole role) {
    switch (role) {
    case FormationSlotRole::CenterBack:
        return PhaseDecisionRoleBucket::CenterBack;
    case FormationSlotRole::LeftBack:
    case FormationSlotRole::RightBack:
    case FormationSlotRole::LeftWingBack:
    case FormationSlotRole::RightWingBack:
        return PhaseDecisionRoleBucket::Fullback;
    case FormationSlotRole::DefensiveMidfielder:
    case FormationSlotRole::CentralMidfielder:
    case FormationSlotRole::LeftMidfielder:
    case FormationSlotRole::RightMidfielder:
    case FormationSlotRole::AttackingMidfielder:
        return PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder;
    case FormationSlotRole::LeftWinger:
    case FormationSlotRole::RightWinger:
        return PhaseDecisionRoleBucket::Winger;
    case FormationSlotRole::Striker:
        return PhaseDecisionRoleBucket::Striker;
    case FormationSlotRole::Goalkeeper:
    case FormationSlotRole::Unknown:
        return PhaseDecisionRoleBucket::Other;
    }
    return PhaseDecisionRoleBucket::Other;
}

struct PhaseDecisionDiagnostics {
    std::array<int, MatchTeamPhaseCount> passCandidatesGenerated{};
    std::array<int, MatchTeamPhaseCount> carryCandidatesGenerated{};
    std::array<int, MatchTeamPhaseCount> shotCandidatesGenerated{};
    std::array<int, MatchTeamPhaseCount> finalBallCandidatesGenerated{};
    std::array<int, MatchTeamPhaseCount> selectedPasses{};
    std::array<int, MatchTeamPhaseCount> selectedCarries{};
    std::array<int, MatchTeamPhaseCount> selectedShots{};
    std::array<int, MatchTeamPhaseCount> selectedFinalBalls{};
    std::array<int, MatchTeamPhaseCount> selectedRecyclePasses{};
    std::array<int, MatchTeamPhaseCount> selectedProgressivePasses{};
    std::array<int, MatchTeamPhaseCount> selectedSTTargets{};
    std::array<int, MatchTeamPhaseCount> selectedCBTargets{};
    std::array<int, MatchTeamPhaseCount> selectedFBTargets{};
    std::array<int, MatchTeamPhaseCount> selectedDMCMTargets{};
    std::array<int, MatchTeamPhaseCount> selectedWingerTargets{};

    std::array<int, PhaseDecisionRoleBucketCount> buildUpPassesByRole{};
    std::array<int, PhaseDecisionRoleBucketCount> buildUpReceptionsByRole{};
    int buildUpShots = 0;
    int buildUpFinalBalls = 0;
    int buildUpSTTargets = 0;
    int buildUpCBFBDMCMTargets = 0;

    std::array<int, PhaseDecisionRoleBucketCount> finalizingShotsByRole{};
    std::array<int, PhaseDecisionRoleBucketCount> finalizingFinalBallsByRole{};
    int finalizingSTTargets = 0;
    int finalizingWingerCMInvolvement = 0;

    int counterSelectedForwardPasses = 0;
    int counterSelectedCarries = 0;
    int counterSelectedRecyclePasses = 0;
};

