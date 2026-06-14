#pragma once

#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/offside/OffsideLineModel.h"
#include"fm/match_engine/offside/OffsideSnapshot.h"

struct OffsideSnapshotRequest {
    const TeamSimState* attackingTeam = nullptr;
    const TeamSimState* defendingTeam = nullptr;
    const MatchTeamSnapshot* attackingSnapshot = nullptr;
    TeamId passerTeamId = 0;
    PlayerId passerPlayerId = 0;
    PlayerId intendedReceiverId = 0;
    BallCarrierActionType actionType = BallCarrierActionType::Hold;
    PitchPoint passStartPoint;
    PitchPoint targetPoint;
    PitchPoint ballPosition;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
};

struct OffsideResolveRequest {
    const OffsideSnapshot* snapshot = nullptr;
    PlayerId activeAttackerId = 0;
};

struct OffsideResolveResult {
    bool offside = false;
    PlayerId offendingPlayerId = 0;
    FormationSlotRole offendingRole = FormationSlotRole::Unknown;
    BallCarrierActionType actionType = BallCarrierActionType::Hold;
};

class OffsideResolver {
public:
    OffsideSnapshot captureSnapshot(const OffsideSnapshotRequest& request) const;
    OffsideResolveResult resolve(const OffsideResolveRequest& request) const;

private:
    OffsideLineModel lineModel_;
};

