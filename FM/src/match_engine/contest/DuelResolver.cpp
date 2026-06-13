#include"fm/match_engine/contest/DuelResolver.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<initializer_list>
#include<utility>

namespace {
    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double clampedAttribute(int value) {
        return std::clamp(static_cast<double>(value), 0.0, 100.0);
    }

    double weightedAttributeAverage(
        int baseOverall,
        std::initializer_list<std::pair<int, double>> values) {
        double total = 0.0;
        double weight = 0.0;
        for (const auto& [value, valueWeight] : values) {
            if (valueWeight <= 0.0) {
                continue;
            }
            total += clampedAttribute(value) * valueWeight;
            weight += valueWeight;
        }
        if (weight <= 0.0) {
            return clampedAttribute(baseOverall);
        }
        return (total / weight) * 0.84 + clampedAttribute(baseOverall) * 0.16;
    }

    PitchPoint shiftedCarryTarget(
        PitchPoint start,
        PitchPoint intendedTarget,
        AttackingDirection direction,
        bool backward) {
        const double dx = intendedTarget.x - start.x;
        const double dy = intendedTarget.y - start.y;
        const double forwardSign = direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
        const double lateralSign = dy >= 0.0 ? 1.0 : -1.0;
        if (backward) {
            return PitchGeometry::clampToPitch(PitchPoint{
                start.x - forwardSign * std::max(1.5, std::abs(dx) * 0.35),
                start.y + lateralSign * std::min(4.5, std::abs(dy) + 2.0)
            });
        }
        return PitchGeometry::clampToPitch(PitchPoint{
            start.x + dx * 0.35,
            start.y + lateralSign * std::max(2.5, std::abs(dy) + 2.5)
        });
    }

    double attackerContestScore(const DuelResolutionRequest& request) {
        const PlayerAttributes& attributes = request.attacker.attributes;
        double score = weightedAttributeAverage(
            request.attacker.baseOverall,
            {
                { attributes.technical.dribbling, 1.25 },
                { attributes.technical.technique, 0.95 },
                { attributes.physical.agility, 0.85 },
                { attributes.physical.pace, 0.55 },
                { attributes.physical.acceleration, 0.75 },
                { attributes.mental.composure, 0.8 },
                { attributes.mental.decisions, 0.45 }
            });
        score -= clampDouble(request.attacker.fatigue, 0.0, 1.0) * 8.0;
        score -= request.pressure.pressureStrength * 0.10;
        if (request.actionType == BallCarrierActionType::Carry) {
            score += 5.0;
        }
        return score;
    }

    double defenderContestScore(const DuelResolutionRequest& request) {
        const PlayerAttributes& attributes = request.defender.attributes;
        double score = weightedAttributeAverage(
            request.defender.baseOverall,
            {
                { attributes.technical.tackling, 1.25 },
                { attributes.technical.marking, 0.65 },
                { attributes.mental.positioning, 1.0 },
                { attributes.mental.decisions, 0.8 },
                { attributes.mental.aggression, 0.45 },
                { attributes.physical.strength, 0.7 },
                { attributes.physical.acceleration, 0.6 }
            });
        score -= clampDouble(request.defender.fatigue, 0.0, 1.0) * 7.0;
        score += request.pressure.defenderBetweenBallAndGoal ? 7.0 : 0.0;
        score += request.pressure.defenderBetweenBallAndPath ? 6.0 : 0.0;
        score += std::min(8.0, static_cast<double>(request.pressure.supportDefendersNearby) * 3.0);
        score += zonePressureBonus(request.pressure.dangerZone) * 0.45;
        score += clampDouble(4.0 - request.pressure.closestOutfieldDefenderDistance, 0.0, 4.0) * 2.0;
        return score;
    }

    double containWindowFor(const DuelResolutionRequest& request) {
        double window = 0.14
            + request.pressure.pressureStrength / 500.0
            + std::min(0.10, static_cast<double>(request.pressure.supportDefendersNearby) * 0.035);
        if (request.pressure.defenderBetweenBallAndGoal) {
            window += 0.07;
        }
        if (request.actionType == BallCarrierActionType::Carry) {
            window += 0.06;
        }
        return clampDouble(window, 0.10, 0.34);
    }

    double executionPressureFor(const DuelResolutionRequest& request) {
        double pressure = request.pressure.pressureStrength * 0.35;
        pressure += request.pressure.defenderBetweenBallAndPath ? 8.0 : 0.0;
        pressure += request.pressure.defenderBetweenBallAndGoal ? 7.0 : 0.0;
        pressure += request.pressure.closestOutfieldDefenderDistance <= 1.5 ? 10.0 : 0.0;
        return clampDouble(pressure, 0.0, 38.0);
    }
}

DuelResolutionResult DuelResolver::resolve(const DuelResolutionRequest& request) const {
    DuelResolutionResult result;
    result.resolvedTarget = request.intendedTarget;
    result.loosePoint = PitchGeometry::clampToPitch(PitchPoint{
        request.start.x * 0.42 + request.intendedTarget.x * 0.58,
        request.start.y * 0.42 + request.intendedTarget.y * 0.58
    });

    if (request.attacker.playerId == 0
        || request.defender.playerId == 0
        || !isRealContest(request.pressure, request.actionType)) {
        return result;
    }

    result.duelOccurred = true;
    result.defensiveAction = TackleResolver{}.selectAction(TackleResolverRequest{
        request.defender.intentType,
        request.defender.attributes,
        request.pressure,
        request.seed
    });
    result.tackleAttempted = result.defensiveAction == DefensiveActionType::Tackle;
    result.addedExecutionPressure = executionPressureFor(request);

    const double attackerScore = attackerContestScore(request);
    const double defenderScore = defenderContestScore(request);
    const double roll = matchEngineDeterministicUnitInterval(request.seed);
    const double defenderChance = clampDouble(
        0.44 + (defenderScore - attackerScore) / 120.0,
        0.12,
        0.82);

    if (result.tackleAttempted && roll < defenderChance * 0.72) {
        result.outcome = DuelOutcomeType::DefenderWinsTackle;
        return result;
    }
    if (roll < defenderChance) {
        result.outcome = DuelOutcomeType::BallLoose;
        return result;
    }

    const double containWindow = containWindowFor(request);
    if (!result.tackleAttempted && roll < defenderChance + containWindow) {
        const bool backward = matchEngineDeterministicUnitInterval(request.seed ^ 0x8fe4172f6b17d23dULL) < 0.44;
        result.outcome = backward
            ? DuelOutcomeType::ForcedBackward
            : DuelOutcomeType::ForcedSideways;
        result.resolvedTarget = shiftedCarryTarget(
            request.start,
            request.intendedTarget,
            request.direction,
            backward);
        return result;
    }

    const double beatWindow = request.actionType == BallCarrierActionType::Carry ? 0.12 : 0.24;
    result.outcome =
        matchEngineDeterministicUnitInterval(request.seed ^ 0x31f894b49be8a921ULL) < beatWindow
            ? DuelOutcomeType::AttackerBeatsDefender
            : DuelOutcomeType::AttackerKeepsBall;
    return result;
}
