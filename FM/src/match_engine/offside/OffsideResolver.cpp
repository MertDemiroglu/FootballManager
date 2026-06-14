#include"fm/match_engine/offside/OffsideResolver.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>

namespace {
    double progress(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }

    bool isOpponentHalf(PitchPoint point, AttackingDirection direction) {
        return progress(point, direction) > PitchGeometry::LengthMeters / 2.0;
    }

    FormationSlotRole roleFor(const MatchTeamSnapshot* snapshot, PlayerId playerId) {
        if (snapshot == nullptr) {
            return FormationSlotRole::Unknown;
        }
        for (const TeamSheetSlotAssignment& assignment : snapshot->teamSheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }
        return FormationSlotRole::Unknown;
    }
}

OffsideSnapshot OffsideResolver::captureSnapshot(const OffsideSnapshotRequest& request) const {
    OffsideSnapshot snapshot;
    snapshot.passerTeamId = request.passerTeamId;
    snapshot.passerPlayerId = request.passerPlayerId;
    snapshot.actionType = request.actionType;
    snapshot.passStartPoint = request.passStartPoint;
    snapshot.targetPoint = request.targetPoint;
    snapshot.ballPosition = request.ballPosition;
    snapshot.attackingDirection = request.attackingDirection;

    if (request.attackingTeam == nullptr || request.defendingTeam == nullptr) {
        return snapshot;
    }

    const OffsideLineResult line =
        lineModel_.evaluate(*request.defendingTeam, request.ballPosition, request.attackingDirection);
    if (!line.valid) {
        return snapshot;
    }
    snapshot.offsideLineProgress = line.lineProgress;
    snapshot.secondLastDefenderId = line.secondLastDefenderId;
    snapshot.secondLastDefenderProgress = line.secondLastDefenderProgress;

    const double ballProgress = progress(request.ballPosition, request.attackingDirection);
    for (const PlayerSimState& attacker : request.attackingTeam->players) {
        if (attacker.playerId == request.passerPlayerId) {
            continue;
        }
        const double attackerProgress = progress(attacker.position, request.attackingDirection);
        if (!isOpponentHalf(attacker.position, request.attackingDirection)
            || attackerProgress <= ballProgress + 0.25
            || attackerProgress <= line.lineProgress + 1.20) {
            continue;
        }

        snapshot.attackersOffsideAtRelease.push_back(OffsidePlayerAtRelease{
            attacker.playerId,
            attacker.teamId,
            roleFor(request.attackingSnapshot, attacker.playerId),
            attackerProgress,
            attackerProgress - line.lineProgress
        });
    }

    return snapshot;
}

OffsideResolveResult OffsideResolver::resolve(const OffsideResolveRequest& request) const {
    OffsideResolveResult result;
    if (request.snapshot == nullptr || request.activeAttackerId == 0) {
        return result;
    }

    if (request.snapshot->actionType == BallCarrierActionType::ShortPass) {
        const double forward =
            progress(request.snapshot->targetPoint, request.snapshot->attackingDirection)
            - progress(request.snapshot->passStartPoint, request.snapshot->attackingDirection);
        const double targetProgress =
            progress(request.snapshot->targetPoint, request.snapshot->attackingDirection);
        const bool lineBreakingFinalThirdPass =
            targetProgress >= PitchGeometry::LengthMeters * 0.70 && forward > 18.0;
        if (!lineBreakingFinalThirdPass) {
            return result;
        }
    }

    for (const OffsidePlayerAtRelease& player : request.snapshot->attackersOffsideAtRelease) {
        if (player.playerId == request.activeAttackerId) {
            result.offside = true;
            result.offendingPlayerId = player.playerId;
            result.offendingRole = player.role;
            result.actionType = request.snapshot->actionType;
            return result;
        }
    }
    return result;
}
