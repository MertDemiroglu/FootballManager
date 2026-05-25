#include"fm/match_engine/contest/ContestResolver.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<initializer_list>
#include<iterator>
#include<limits>
#include<utility>

namespace {
    constexpr double DefaultContestSpeedMetersPerSecond = 6.0;
    constexpr double MinimumContestSpeedMetersPerSecond = 0.5;
    constexpr double SelectionWeightExponent = 1.6;

    struct ScoredParticipant {
        ContestParticipant participant;
        ContestantScore score;
    };

    bool isValidParticipant(const ContestParticipant& participant) {
        return participant.playerId != 0 && participant.teamId != 0;
    }

    double clampedAttribute(int value) {
        return std::clamp(static_cast<double>(value), 0.0, 100.0);
    }

    double baseOverallScore(const ContestParticipant& participant) {
        return std::clamp(static_cast<double>(participant.baseOverall), 0.0, 100.0);
    }

    double weightedAverageOrBase(
        const ContestParticipant& participant,
        std::initializer_list<std::pair<int, double>> values) {
        double weightedTotal = 0.0;
        double weightTotal = 0.0;

        for (const auto& [value, weight] : values) {
            if (weight <= 0.0) {
                continue;
            }

            weightedTotal += clampedAttribute(value) * weight;
            weightTotal += weight;
        }

        if (weightTotal <= 0.0) {
            return baseOverallScore(participant);
        }

        const double attributeAverage = weightedTotal / weightTotal;
        const double base = baseOverallScore(participant);
        if (base <= 0.0) {
            return attributeAverage;
        }

        return (attributeAverage * 0.85) + (base * 0.15);
    }

    double executionScore(double executionQuality) {
        return std::clamp(executionQuality, 0.0, 100.0);
    }

    double paceFor(const ContestParticipant& participant) {
        if (participant.effectivePace > 0.0) {
            return std::max(participant.effectivePace, MinimumContestSpeedMetersPerSecond);
        }

        const double attributePace = clampedAttribute(participant.attributes.physical.pace);
        if (attributePace > 0.0) {
            return std::max(3.0 + (attributePace / 20.0), MinimumContestSpeedMetersPerSecond);
        }

        return DefaultContestSpeedMetersPerSecond;
    }

    double accelerationSupport(const ContestParticipant& participant) {
        if (participant.effectiveAcceleration > 0.0) {
            return std::clamp(participant.effectiveAcceleration * 4.0, 0.0, 10.0);
        }

        return clampedAttribute(participant.attributes.physical.acceleration) / 10.0;
    }

    double arrivalSecondFor(const ContestResolverRequest& request, const ContestParticipant& participant) {
        if (participant.arrivalSecond > 0.0) {
            return participant.arrivalSecond;
        }

        const double distanceMeters =
            PitchGeometry::distance(participant.position, request.contestPoint);
        return distanceMeters / paceFor(participant);
    }

    double timingScoreFor(const ContestResolverRequest& request, const ContestParticipant& participant) {
        const double arrivalSecond = arrivalSecondFor(request, participant);
        const double arrivalDelta = arrivalSecond - request.ballArrivalSecond;
        const double distanceMeters =
            PitchGeometry::distance(participant.position, request.contestPoint);

        const double arrivalScore = std::clamp(12.0 - (std::abs(arrivalDelta) * 7.0), -8.0, 12.0);
        const double earlyBonus = arrivalDelta < 0.0 ? std::min(4.0, std::abs(arrivalDelta) * 2.0) : 0.0;
        const double distanceScore = std::clamp(8.0 - (distanceMeters * 0.25), -6.0, 8.0);
        return arrivalScore + earlyBonus + distanceScore + participant.startingAdvantage + accelerationSupport(participant);
    }

    double fatiguePenalty(const ContestParticipant& participant) {
        return std::clamp(participant.fatigue, 0.0, 1.0) * 8.0;
    }

    double pressurePenalty(const ContestResolverRequest& request, const ContestParticipant& participant) {
        if (participant.side != ContestSide::Attacking) {
            return 0.0;
        }

        const double pressureFactor = std::clamp(request.pressure, 0.0, 100.0) / 100.0;
        return pressureFactor * 8.0;
    }

    double interceptionCandidateBonus(
        const ContestResolverRequest& request,
        const ContestParticipant& participant) {
        if (!request.interceptionCandidate
            || request.interceptionCandidate->playerId != participant.playerId) {
            return 0.0;
        }

        const double marginBonus =
            std::clamp(6.0 - (std::abs(request.interceptionCandidate->arrivalMarginSeconds) * 8.0), 0.0, 6.0);
        return marginBonus + std::clamp(request.interceptionCandidate->qualityScore, 0.0, 10.0);
    }

    double interceptionAttributeScore(const ContestParticipant& participant) {
        if (participant.side == ContestSide::Defending) {
            return weightedAverageOrBase(
                participant,
                {
                    { participant.attributes.mental.positioning, 1.25 },
                    { participant.attributes.mental.decisions, 1.0 },
                    { participant.attributes.technical.marking, 1.0 },
                    { participant.attributes.technical.tackling, 0.85 },
                    { participant.attributes.physical.pace, 0.55 },
                    { participant.attributes.physical.acceleration, 0.55 },
                    { participant.attributes.mental.concentration, 0.65 }
                });
        }

        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.passing, 1.0 },
                { participant.attributes.technical.crossing, 0.9 },
                { participant.attributes.technical.technique, 0.8 },
                { participant.attributes.mental.composure, 0.75 },
                { participant.attributes.mental.decisions, 0.5 }
            });
    }

    double receptionAttributeScore(const ContestParticipant& participant) {
        if (participant.side == ContestSide::Defending) {
            return weightedAverageOrBase(
                participant,
                {
                    { participant.attributes.technical.marking, 1.15 },
                    { participant.attributes.technical.tackling, 1.0 },
                    { participant.attributes.physical.strength, 0.8 },
                    { participant.attributes.mental.positioning, 0.9 },
                    { participant.attributes.mental.concentration, 0.7 }
                });
        }

        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.firstTouch, 1.25 },
                { participant.attributes.technical.technique, 1.0 },
                { participant.attributes.physical.strength, 0.65 },
                { participant.attributes.mental.composure, 0.8 },
                { participant.attributes.mental.positioning, 0.45 },
                { participant.attributes.mental.offTheBall, 0.75 }
            });
    }

    double dribbleAttributeScore(const ContestParticipant& participant) {
        if (participant.side == ContestSide::Defending) {
            return weightedAverageOrBase(
                participant,
                {
                    { participant.attributes.technical.tackling, 1.2 },
                    { participant.attributes.mental.positioning, 1.0 },
                    { participant.attributes.physical.strength, 0.7 },
                    { participant.attributes.mental.decisions, 0.75 },
                    { participant.attributes.mental.aggression, 0.3 }
                });
        }

        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.dribbling, 1.2 },
                { participant.attributes.technical.technique, 0.9 },
                { participant.attributes.physical.agility, 0.9 },
                { participant.attributes.physical.acceleration, 0.75 },
                { participant.attributes.mental.composure, 0.7 }
            });
    }

    double tackleAttributeScore(const ContestParticipant& participant) {
        if (participant.side == ContestSide::Defending) {
            return weightedAverageOrBase(
                participant,
                {
                    { participant.attributes.technical.tackling, 1.3 },
                    { participant.attributes.physical.strength, 0.9 },
                    { participant.attributes.mental.aggression, 0.45 },
                    { participant.attributes.mental.decisions, 0.75 },
                    { participant.attributes.mental.positioning, 0.75 }
                });
        }

        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.dribbling, 1.05 },
                { participant.attributes.technical.technique, 0.85 },
                { participant.attributes.physical.agility, 0.85 },
                { participant.attributes.physical.strength, 0.55 },
                { participant.attributes.mental.composure, 0.75 }
            });
    }

    double shotBlockAttributeScore(const ContestParticipant& participant) {
        if (participant.side == ContestSide::Defending) {
            return weightedAverageOrBase(
                participant,
                {
                    { participant.attributes.mental.positioning, 1.15 },
                    { participant.attributes.mental.concentration, 1.0 },
                    { participant.attributes.technical.marking, 0.75 },
                    { participant.attributes.technical.tackling, 0.8 },
                    { participant.attributes.physical.agility, 0.45 }
                });
        }

        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.shooting, 1.25 },
                { participant.attributes.technical.technique, 0.85 },
                { participant.attributes.mental.composure, 0.9 },
                { participant.attributes.mental.decisions, 0.45 }
            });
    }

    double goalkeeperSaveAttributeScore(const ContestParticipant& participant) {
        if (participant.side == ContestSide::Defending) {
            return weightedAverageOrBase(
                participant,
                {
                    { participant.attributes.goalkeeper.shotStopping, 1.3 },
                    { participant.attributes.goalkeeper.handling, 0.9 },
                    { participant.attributes.goalkeeper.oneOnOnes, 0.85 },
                    { participant.attributes.goalkeeper.aerialAbility, 0.4 },
                    { participant.attributes.mental.positioning, 0.65 },
                    { participant.attributes.mental.concentration, 0.65 }
                });
        }

        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.shooting, 1.25 },
                { participant.attributes.technical.technique, 0.85 },
                { participant.attributes.mental.composure, 0.9 },
                { participant.attributes.mental.decisions, 0.45 }
            });
    }

    double aerialAttributeScore(const ContestParticipant& participant) {
        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.heading, 1.15 },
                { participant.attributes.physical.jumpingReach, 1.0 },
                { participant.attributes.physical.strength, 0.85 },
                { participant.attributes.mental.positioning, 0.75 },
                { participant.attributes.mental.concentration, 0.65 }
            });
    }

    double looseBallRaceAttributeScore(const ContestParticipant& participant) {
        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.physical.pace, 1.1 },
                { participant.attributes.physical.acceleration, 1.1 },
                { participant.attributes.mental.positioning, 0.7 },
                { participant.attributes.mental.workRate, 0.8 },
                { participant.attributes.mental.decisions, 0.45 }
            });
    }

    double attributeScoreFor(
        const ContestResolverRequest& request,
        const ContestParticipant& participant) {
        switch (request.type) {
        case ContestType::PassingLaneInterception:
        case ContestType::GroundCrossInterception:
            return interceptionAttributeScore(participant);
        case ContestType::ReceptionDuel:
            return receptionAttributeScore(participant);
        case ContestType::DribbleDuel:
            return dribbleAttributeScore(participant);
        case ContestType::TackleAttempt:
            return tackleAttributeScore(participant);
        case ContestType::ShotBlock:
            return shotBlockAttributeScore(participant);
        case ContestType::GoalkeeperSave:
            return goalkeeperSaveAttributeScore(participant);
        case ContestType::AerialDuel:
            return aerialAttributeScore(participant);
        case ContestType::LooseBallRace:
            return looseBallRaceAttributeScore(participant);
        }

        return baseOverallScore(participant);
    }

    double contextScoreFor(
        const ContestResolverRequest& request,
        const ContestParticipant& participant) {
        double score = 0.0;

        if (participant.side == ContestSide::Attacking) {
            score += (executionScore(request.executionQuality) - 50.0) * 0.12;
        }

        if (request.type == ContestType::PassingLaneInterception
            || request.type == ContestType::GroundCrossInterception) {
            score += interceptionCandidateBonus(request, participant);
        }

        score -= fatiguePenalty(participant);
        score -= pressurePenalty(request, participant);
        return score;
    }

    double selectionWeightFor(double finalScore) {
        const double safeScore = std::max(finalScore, 1.0);
        return std::pow(safeScore, SelectionWeightExponent);
    }

    ContestantScore scoreParticipant(
        const ContestResolverRequest& request,
        const ContestParticipant& participant) {
        ContestantScore score;
        score.playerId = participant.playerId;
        score.teamId = participant.teamId;
        score.side = participant.side;
        score.attributeScore = attributeScoreFor(request, participant);
        score.timingScore = timingScoreFor(request, participant);
        score.contextScore = contextScoreFor(request, participant);
        score.finalScore =
            score.attributeScore
            + score.timingScore
            + score.contextScore;
        score.selectionWeight = selectionWeightFor(score.finalScore);
        return score;
    }

    std::uint64_t sideSeed(ContestSide side) {
        return static_cast<std::uint64_t>(side) + 0x75a85ec6d4c53b2bULL;
    }

    std::uint64_t contestRollSeed(
        const ContestResolverRequest& request,
        const std::vector<ScoredParticipant>& scoredParticipants,
        std::uint64_t purpose) {
        std::uint64_t seed = request.seed
            ^ matchEngineMix64(static_cast<std::uint64_t>(request.type) + 0x9e3779b97f4a7c15ULL)
            ^ matchEngineMix64(purpose)
            ^ matchEngineMix64(static_cast<std::uint64_t>(scoredParticipants.size()) + 31ULL);

        std::uint64_t index = 0;
        for (const ScoredParticipant& scored : scoredParticipants) {
            seed ^= matchEngineMix64(static_cast<std::uint64_t>(scored.participant.playerId) + (index * 17ULL));
            seed ^= matchEngineMix64((static_cast<std::uint64_t>(scored.participant.teamId) << 1) + (index * 29ULL));
            seed ^= matchEngineMix64(sideSeed(scored.participant.side) + (index * 43ULL));
            ++index;
        }

        return seed;
    }

    std::vector<ScoredParticipant>::const_iterator selectWeightedWinner(
        const ContestResolverRequest& request,
        const std::vector<ScoredParticipant>& scoredParticipants) {
        double totalWeight = 0.0;
        for (const ScoredParticipant& scored : scoredParticipants) {
            totalWeight += scored.score.selectionWeight;
        }

        if (totalWeight <= 0.0) {
            return std::max_element(
                scoredParticipants.begin(),
                scoredParticipants.end(),
                [](const ScoredParticipant& lhs, const ScoredParticipant& rhs) {
                    return lhs.score.finalScore < rhs.score.finalScore;
                });
        }

        const double roll = matchEngineDeterministicUnitInterval(
            contestRollSeed(request, scoredParticipants, 0x441fd67d2f5d41c3ULL)) * totalWeight;

        double cursor = 0.0;
        for (auto it = scoredParticipants.begin(); it != scoredParticipants.end(); ++it) {
            cursor += it->score.selectionWeight;
            if (roll < cursor) {
                return it;
            }
        }

        return std::prev(scoredParticipants.end());
    }

    double attackingControlSkill(const ContestParticipant& participant) {
        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.firstTouch, 1.1 },
                { participant.attributes.technical.technique, 0.95 },
                { participant.attributes.technical.dribbling, 0.85 },
                { participant.attributes.mental.composure, 0.8 },
                { participant.attributes.mental.decisions, 0.45 }
            });
    }

    double defensiveControlSkill(const ContestParticipant& participant) {
        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.technical.tackling, 1.05 },
                { participant.attributes.technical.marking, 0.85 },
                { participant.attributes.mental.positioning, 1.0 },
                { participant.attributes.mental.concentration, 0.7 },
                { participant.attributes.mental.decisions, 0.55 }
            });
    }

    double keeperControlSkill(const ContestParticipant& participant) {
        return weightedAverageOrBase(
            participant,
            {
                { participant.attributes.goalkeeper.handling, 1.25 },
                { participant.attributes.goalkeeper.shotStopping, 1.0 },
                { participant.attributes.goalkeeper.oneOnOnes, 0.65 },
                { participant.attributes.mental.composure, 0.55 },
                { participant.attributes.mental.concentration, 0.65 }
            });
    }

    double controlScore(
        const ContestResolverRequest& request,
        const ContestParticipant& winner,
        const ContestantScore& winnerScore,
        double winningMargin,
        double controlSkill) {
        double score = 45.0;
        score += std::clamp(winningMargin, -20.0, 25.0) * 1.1;
        score += std::clamp(winnerScore.timingScore, -12.0, 20.0) * 0.75;
        score += (controlSkill - 50.0) * 0.35;
        score -= std::clamp(winner.fatigue, 0.0, 1.0) * 15.0;

        if (winner.side == ContestSide::Attacking) {
            score += (executionScore(request.executionQuality) - 50.0) * 0.12;
            score -= (std::clamp(request.pressure, 0.0, 100.0) / 100.0) * 6.0;
        }

        if (request.interceptionCandidate
            && request.interceptionCandidate->playerId == winner.playerId) {
            score += std::clamp(request.interceptionCandidate->qualityScore, 0.0, 10.0) * 0.8;
            score += std::clamp(1.0 - std::abs(request.interceptionCandidate->arrivalMarginSeconds), 0.0, 1.0) * 5.0;
        }

        switch (request.type) {
        case ContestType::ReceptionDuel:
        case ContestType::DribbleDuel:
        case ContestType::LooseBallRace:
            score += 8.0;
            break;
        case ContestType::AerialDuel:
            score -= 8.0;
            break;
        case ContestType::TackleAttempt:
            if (winner.side == ContestSide::Defending) {
                score -= 5.0;
            }
            break;
        case ContestType::GoalkeeperSave:
            score += 4.0;
            break;
        case ContestType::ShotBlock:
        case ContestType::PassingLaneInterception:
        case ContestType::GroundCrossInterception:
            break;
        }

        return std::clamp(score, 5.0, 95.0);
    }

    double outcomeRoll(
        const ContestResolverRequest& request,
        const ContestParticipant& winner,
        std::uint64_t purpose) {
        const std::uint64_t seed =
            request.seed
            ^ matchEngineMix64(static_cast<std::uint64_t>(request.type) + 0xa24baed4963ee407ULL)
            ^ matchEngineMix64(static_cast<std::uint64_t>(winner.playerId) + 0x9fb21c651e98df25ULL)
            ^ matchEngineMix64(static_cast<std::uint64_t>(winner.teamId) << 1)
            ^ matchEngineMix64(sideSeed(winner.side))
            ^ matchEngineMix64(purpose);

        return matchEngineDeterministicUnitInterval(seed) * 100.0;
    }

    ContestBallOutcome defensiveLooseOrDeflectedOutcome(
        const ContestResolverRequest& request,
        const ContestParticipant& winner,
        const ContestantScore& winnerScore,
        double winningMargin,
        bool allowDeflection) {
        const double cleanScore = controlScore(
            request,
            winner,
            winnerScore,
            winningMargin,
            defensiveControlSkill(winner));
        const double roll = outcomeRoll(request, winner, 0x14f30c31cc2df175ULL);

        if (roll < cleanScore) {
            return ContestBallOutcome::DefenderControls;
        }

        if (allowDeflection) {
            const double deflectionWindow = std::clamp(34.0 - (cleanScore * 0.18), 16.0, 30.0);
            if (roll < cleanScore + deflectionWindow) {
                return ContestBallOutcome::BallDeflected;
            }
        }

        return ContestBallOutcome::BallLoose;
    }

    ContestBallOutcome keeperSaveOutcome(
        const ContestResolverRequest& request,
        const ContestParticipant& winner,
        const ContestantScore& winnerScore,
        double winningMargin) {
        const double holdScore = controlScore(
            request,
            winner,
            winnerScore,
            winningMargin,
            keeperControlSkill(winner));
        const double roll = outcomeRoll(request, winner, 0x9c3df932889d1f9bULL);

        if (roll < holdScore) {
            return ContestBallOutcome::KeeperControls;
        }

        return outcomeRoll(request, winner, 0x38f8ad1f826a3815ULL) < 70.0
            ? ContestBallOutcome::BallDeflected
            : ContestBallOutcome::BallLoose;
    }

    ContestBallOutcome attackingAerialOutcome(
        const ContestResolverRequest& request,
        const ContestParticipant& winner,
        const ContestantScore& winnerScore,
        double winningMargin) {
        const double cleanScore = controlScore(
            request,
            winner,
            winnerScore,
            winningMargin,
            attackingControlSkill(winner));

        return outcomeRoll(request, winner, 0x79d7745f6f75e0d5ULL) < cleanScore
            ? ContestBallOutcome::AttackerKeepsControl
            : ContestBallOutcome::BallLoose;
    }

    ContestBallOutcome resolveBallOutcome(
        const ContestResolverRequest& request,
        const ContestParticipant& winner,
        const ContestantScore& winnerScore,
        double winningMargin) {
        if (winner.side == ContestSide::Neutral) {
            return ContestBallOutcome::BallLoose;
        }

        switch (request.type) {
        case ContestType::PassingLaneInterception:
        case ContestType::GroundCrossInterception:
            if (winner.side == ContestSide::Defending) {
                return defensiveLooseOrDeflectedOutcome(
                    request,
                    winner,
                    winnerScore,
                    winningMargin,
                    true);
            }
            return ContestBallOutcome::AttackerKeepsControl;

        case ContestType::ReceptionDuel:
            if (winner.side == ContestSide::Defending) {
                return defensiveLooseOrDeflectedOutcome(
                    request,
                    winner,
                    winnerScore,
                    winningMargin,
                    false);
            }
            return ContestBallOutcome::AttackerKeepsControl;

        case ContestType::DribbleDuel:
        case ContestType::TackleAttempt:
            if (winner.side == ContestSide::Defending) {
                return defensiveLooseOrDeflectedOutcome(
                    request,
                    winner,
                    winnerScore,
                    winningMargin,
                    true);
            }
            return ContestBallOutcome::AttackerKeepsControl;

        case ContestType::ShotBlock:
            return winner.side == ContestSide::Defending
                ? ContestBallOutcome::BallDeflected
                : ContestBallOutcome::ShotContinues;

        case ContestType::GoalkeeperSave:
            return winner.side == ContestSide::Defending
                ? keeperSaveOutcome(request, winner, winnerScore, winningMargin)
                : ContestBallOutcome::ShotContinues;

        case ContestType::AerialDuel:
            if (winner.side == ContestSide::Defending) {
                return defensiveLooseOrDeflectedOutcome(
                    request,
                    winner,
                    winnerScore,
                    winningMargin,
                    true);
            }
            return attackingAerialOutcome(request, winner, winnerScore, winningMargin);

        case ContestType::LooseBallRace:
            return winner.side == ContestSide::Defending
                ? ContestBallOutcome::DefenderControls
                : ContestBallOutcome::AttackerKeepsControl;
        }

        return ContestBallOutcome::BallLoose;
    }

    void applyCleanController(ContestResolverResult& result) {
        if (!result.winner) {
            return;
        }

        switch (result.ballOutcome) {
        case ContestBallOutcome::AttackerKeepsControl:
            if (result.winner->side == ContestSide::Attacking
                && (result.type == ContestType::ReceptionDuel
                    || result.type == ContestType::DribbleDuel
                    || result.type == ContestType::TackleAttempt
                    || result.type == ContestType::AerialDuel
                    || result.type == ContestType::LooseBallRace)) {
                result.cleanController = result.winner;
            }
            break;
        case ContestBallOutcome::DefenderControls:
        case ContestBallOutcome::KeeperControls:
            result.cleanController = result.winner;
            break;
        case ContestBallOutcome::None:
        case ContestBallOutcome::BallDeflected:
        case ContestBallOutcome::BallLoose:
        case ContestBallOutcome::ShotContinues:
        case ContestBallOutcome::OutOfPlay:
            break;
        }
    }

    void applyResolutionMapping(
        ContestResolverResult& result,
        const ContestResolverRequest& request,
        const ContestantScore& winnerScore) {
        if (!result.winner) {
            result.resolution = ContestResolutionType::NoContest;
            result.winningSide = ContestSide::None;
            return;
        }

        result.winningSide = result.winner->side;
        result.attackingSideSucceeded = result.winningSide == ContestSide::Attacking;
        result.defendingActionSucceeded = result.winningSide == ContestSide::Defending;
        result.ballOutcome = resolveBallOutcome(
            request,
            *result.winner,
            winnerScore,
            result.winningMargin);

        switch (request.type) {
        case ContestType::PassingLaneInterception:
        case ContestType::GroundCrossInterception:
            if (result.winningSide == ContestSide::Defending) {
                if (result.ballOutcome == ContestBallOutcome::DefenderControls) {
                    result.resolution = ContestResolutionType::DefenderWon;
                } else if (result.ballOutcome == ContestBallOutcome::BallDeflected) {
                    result.resolution = ContestResolutionType::BallDeflected;
                } else {
                    result.resolution = ContestResolutionType::BallLoose;
                }
            } else {
                result.resolution = ContestResolutionType::AttackerWon;
            }
            break;

        case ContestType::GoalkeeperSave:
            result.resolution = result.winningSide == ContestSide::Defending
                ? ContestResolutionType::KeeperSaved
                : ContestResolutionType::ShotBeatsKeeper;
            break;

        case ContestType::ShotBlock:
            result.resolution = result.winningSide == ContestSide::Defending
                ? ContestResolutionType::BallDeflected
                : ContestResolutionType::AttackerWon;
            break;

        case ContestType::ReceptionDuel:
        case ContestType::DribbleDuel:
        case ContestType::TackleAttempt:
        case ContestType::AerialDuel:
        case ContestType::LooseBallRace:
            if (result.winningSide == ContestSide::Attacking) {
                result.resolution = ContestResolutionType::AttackerWon;
            } else if (result.winningSide == ContestSide::Defending) {
                result.resolution = ContestResolutionType::DefenderWon;
            } else {
                result.resolution = ContestResolutionType::BallLoose;
            }
            break;
        }

        applyCleanController(result);
        result.ballDeflected = result.ballOutcome == ContestBallOutcome::BallDeflected;
        result.ballBecomesLoose = result.ballOutcome == ContestBallOutcome::BallLoose;
        result.cleanPossessionWon = result.cleanController.has_value();
        result.possessionChanges =
            result.cleanController
            && result.cleanController->side == ContestSide::Defending
            && (result.type == ContestType::PassingLaneInterception
                || result.type == ContestType::GroundCrossInterception
                || result.type == ContestType::ReceptionDuel
                || result.type == ContestType::DribbleDuel
                || result.type == ContestType::TackleAttempt
                || result.type == ContestType::GoalkeeperSave
                || result.type == ContestType::AerialDuel
                || result.type == ContestType::LooseBallRace);
    }
}

ContestResolverResult ContestResolver::resolve(const ContestResolverRequest& request) const {
    ContestResolverResult result;
    result.type = request.type;

    std::vector<ScoredParticipant> scoredParticipants;
    scoredParticipants.reserve(request.participants.size());

    for (const ContestParticipant& participant : request.participants) {
        if (!isValidParticipant(participant)) {
            continue;
        }

        ScoredParticipant scored;
        scored.participant = participant;
        scored.score = scoreParticipant(request, participant);
        scoredParticipants.push_back(scored);
        result.scores.push_back(scored.score);
    }

    if (scoredParticipants.empty()) {
        return result;
    }

    std::sort(
        result.scores.begin(),
        result.scores.end(),
        [](const ContestantScore& lhs, const ContestantScore& rhs) {
            return lhs.finalScore > rhs.finalScore;
        });

    const auto winnerIt = selectWeightedWinner(request, scoredParticipants);

    result.winner = winnerIt->participant;

    double bestOpposingScore = -std::numeric_limits<double>::infinity();
    for (const ScoredParticipant& scored : scoredParticipants) {
        if (scored.participant.playerId == winnerIt->participant.playerId
            || scored.participant.side == winnerIt->participant.side) {
            continue;
        }

        if (scored.score.finalScore > bestOpposingScore) {
            bestOpposingScore = scored.score.finalScore;
            result.loser = scored.participant;
        }
    }

    if (result.loser) {
        result.winningMargin = winnerIt->score.finalScore - bestOpposingScore;
    } else {
        result.winningMargin = winnerIt->score.finalScore;
    }

    applyResolutionMapping(result, request, winnerIt->score);
    return result;
}
