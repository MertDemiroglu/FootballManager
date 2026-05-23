#include"fm/match_engine/ContestResolver.h"

#include"DeterministicRandom.h"
#include"fm/match_engine/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<initializer_list>
#include<limits>
#include<utility>

namespace {
    constexpr double CloseContestMargin = 2.5;
    constexpr double DefaultContestSpeedMetersPerSecond = 6.0;
    constexpr double MinimumContestSpeedMetersPerSecond = 0.5;

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

        return std::clamp(request.pressure, 0.0, 1.0) * 8.0;
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

    double randomScoreFor(
        const ContestResolverRequest& request,
        const ContestParticipant& participant,
        std::uint64_t index) {
        const std::uint64_t typeSeed =
            static_cast<std::uint64_t>(request.type) + 0x9e3779b97f4a7c15ULL;
        const std::uint64_t seed =
            request.seed
            ^ matchEngineMix64(static_cast<std::uint64_t>(participant.playerId))
            ^ matchEngineMix64(static_cast<std::uint64_t>(participant.teamId) << 1)
            ^ matchEngineMix64(typeSeed << 2)
            ^ matchEngineMix64(index + 17ULL);

        return (matchEngineDeterministicUnitInterval(seed) * 10.0) - 5.0;
    }

    ContestantScore scoreParticipant(
        const ContestResolverRequest& request,
        const ContestParticipant& participant,
        std::uint64_t index) {
        ContestantScore score;
        score.playerId = participant.playerId;
        score.teamId = participant.teamId;
        score.side = participant.side;
        score.attributeScore = attributeScoreFor(request, participant);
        score.timingScore = timingScoreFor(request, participant);
        score.contextScore = contextScoreFor(request, participant);
        score.randomScore = randomScoreFor(request, participant, index);
        score.finalScore =
            score.attributeScore
            + score.timingScore
            + score.contextScore
            + score.randomScore;
        return score;
    }

    bool isDeflectionType(ContestType type) {
        return type == ContestType::PassingLaneInterception
            || type == ContestType::GroundCrossInterception
            || type == ContestType::ShotBlock;
    }

    bool defenderWinChangesPossession(ContestType type) {
        return type == ContestType::PassingLaneInterception
            || type == ContestType::GroundCrossInterception
            || type == ContestType::ReceptionDuel
            || type == ContestType::DribbleDuel
            || type == ContestType::TackleAttempt
            || type == ContestType::AerialDuel
            || type == ContestType::LooseBallRace;
    }

    ContestResolutionType closeContestResolution(ContestType type) {
        if (isDeflectionType(type)) {
            return ContestResolutionType::BallDeflected;
        }

        return ContestResolutionType::BallLoose;
    }

    void applyResolutionMapping(ContestResolverResult& result) {
        if (!result.winner) {
            result.resolution = ContestResolutionType::NoContest;
            result.winningSide = ContestSide::None;
            return;
        }

        result.winningSide = result.winner->side;

        if (result.winningMargin <= CloseContestMargin) {
            result.resolution = closeContestResolution(result.type);
            result.attackingSideSucceeded = false;
            result.possessionChanges = false;
            result.ballBecomesLoose = result.resolution == ContestResolutionType::BallLoose;
            return;
        }

        if (result.type == ContestType::GoalkeeperSave) {
            if (result.winningSide == ContestSide::Defending) {
                result.resolution = ContestResolutionType::KeeperSaved;
                result.possessionChanges = true;
            } else if (result.winningSide == ContestSide::Attacking) {
                result.resolution = ContestResolutionType::ShotBeatsKeeper;
                result.attackingSideSucceeded = true;
            } else {
                result.resolution = ContestResolutionType::BallLoose;
                result.ballBecomesLoose = true;
            }
            return;
        }

        if (result.type == ContestType::ShotBlock
            && result.winningSide == ContestSide::Defending) {
            result.resolution = ContestResolutionType::BallDeflected;
            return;
        }

        if (result.winningSide == ContestSide::Attacking) {
            result.resolution = ContestResolutionType::AttackerWon;
            result.attackingSideSucceeded = true;
            return;
        }

        if (result.winningSide == ContestSide::Defending) {
            result.resolution = ContestResolutionType::DefenderWon;
            result.possessionChanges = defenderWinChangesPossession(result.type);
            return;
        }

        result.resolution = ContestResolutionType::BallLoose;
        result.ballBecomesLoose = true;
    }
}

ContestResolverResult ContestResolver::resolve(const ContestResolverRequest& request) const {
    ContestResolverResult result;
    result.type = request.type;

    std::vector<ScoredParticipant> scoredParticipants;
    scoredParticipants.reserve(request.participants.size());

    std::uint64_t validIndex = 0;
    for (const ContestParticipant& participant : request.participants) {
        if (!isValidParticipant(participant)) {
            continue;
        }

        ScoredParticipant scored;
        scored.participant = participant;
        scored.score = scoreParticipant(request, participant, validIndex);
        scoredParticipants.push_back(scored);
        result.scores.push_back(scored.score);
        ++validIndex;
    }

    if (scoredParticipants.empty()) {
        return result;
    }

    const auto winnerIt = std::max_element(
        scoredParticipants.begin(),
        scoredParticipants.end(),
        [](const ScoredParticipant& lhs, const ScoredParticipant& rhs) {
            return lhs.score.finalScore < rhs.score.finalScore;
        });

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

    applyResolutionMapping(result);
    return result;
}
