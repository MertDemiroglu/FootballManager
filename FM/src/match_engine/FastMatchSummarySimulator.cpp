#include"fm/match_engine/FastMatchSummarySimulator.h"

#include"DeterministicRandom.h"
#include"fm/match_engine/MatchEngineReportAdapter.h"

#include<algorithm>
#include<cmath>
#include<cstdint>
#include<numeric>
#include<vector>

namespace {
    constexpr int RegulationMatchSeconds = 90 * 60;

    struct StarterProfile {
        const MatchPlayerSnapshot* player = nullptr;
        FormationSlotRole role = FormationSlotRole::Unknown;
        double scoringWeight = 1.0;
        double assistWeight = 1.0;
    };

    struct TeamSummary {
        TeamId teamId = 0;
        double attackStrength = 60.0;
        double chanceCreation = 60.0;
        double finishing = 60.0;
        double defensiveStrength = 60.0;
        double goalkeeperStrength = 60.0;
        double possessionStrength = 60.0;
        double shotVolumeFactor = 1.0;
        double passAccuracy = 0.76;
        double riskFactor = 1.0;
        std::vector<StarterProfile> starters;
    };

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    int roundedClamped(double value, int minimum, int maximum) {
        return std::clamp(static_cast<int>(std::round(value)), minimum, maximum);
    }

    std::uint64_t combineSeed(std::uint64_t seed, std::uint64_t value) {
        return matchEngineMix64(seed ^ matchEngineMix64(value + 0x9e3779b97f4a7c15ULL));
    }

    std::uint64_t fallbackSeedFor(const MatchEngineInput& input) {
        std::uint64_t seed = 0xbacd5eed54197a31ULL;
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.matchId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.leagueId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.homeTeam.teamId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.awayTeam.teamId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.matchDate.getYear()));
        seed = combineSeed(seed, static_cast<std::uint64_t>(static_cast<int>(input.matchDate.getMonth())));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.matchDate.getDay()));
        return seed == 0 ? 0xbacd5eed54197a31ULL : seed;
    }

    std::uint64_t baseSeedFor(const MatchEngineInput& input) {
        return input.options.deterministicSeed != 0
            ? input.options.deterministicSeed
            : fallbackSeedFor(input);
    }

    double unit(std::uint64_t seed, std::uint64_t salt) {
        return matchEngineDeterministicUnitInterval(combineSeed(seed, salt));
    }

    const MatchPlayerSnapshot* findSnapshot(const MatchTeamSnapshot& team, PlayerId playerId) {
        for (const MatchPlayerSnapshot& player : team.players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }
        return nullptr;
    }

    FormationSlotRole assignedRoleFor(const TeamSheet& sheet, PlayerId playerId) {
        for (const TeamSheetSlotAssignment& assignment : sheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }
        return FormationSlotRole::Unknown;
    }

    std::vector<PlayerId> starterIdsFor(const TeamSheet& sheet) {
        std::vector<PlayerId> starters;
        if (!sheet.startingAssignments.empty()) {
            starters.reserve(sheet.startingAssignments.size());
            for (const TeamSheetSlotAssignment& assignment : sheet.startingAssignments) {
                if (assignment.playerId != 0) {
                    starters.push_back(assignment.playerId);
                }
            }
            return starters;
        }
        return sheet.startingPlayerIds;
    }

    bool isGoalkeeperRole(FormationSlotRole role, PlayerPosition position) {
        return role == FormationSlotRole::Goalkeeper || position == PlayerPosition::Goalkeeper;
    }

    bool isDefensiveRole(FormationSlotRole role, PlayerPosition position) {
        return role == FormationSlotRole::CenterBack
            || role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack
            || role == FormationSlotRole::DefensiveMidfielder
            || position == PlayerPosition::CenterBack
            || position == PlayerPosition::LeftBack
            || position == PlayerPosition::RightBack
            || position == PlayerPosition::DefensiveMidfielder;
    }

    bool isMidfieldRole(FormationSlotRole role, PlayerPosition position) {
        return role == FormationSlotRole::DefensiveMidfielder
            || role == FormationSlotRole::CentralMidfielder
            || role == FormationSlotRole::AttackingMidfielder
            || role == FormationSlotRole::LeftMidfielder
            || role == FormationSlotRole::RightMidfielder
            || position == PlayerPosition::DefensiveMidfielder
            || position == PlayerPosition::CentralMidfielder
            || position == PlayerPosition::AttackingMidfielder
            || position == PlayerPosition::LeftMidfielder
            || position == PlayerPosition::RightMidfielder;
    }

    bool isAttackingRole(FormationSlotRole role, PlayerPosition position) {
        return role == FormationSlotRole::Striker
            || role == FormationSlotRole::AttackingMidfielder
            || role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger
            || position == PlayerPosition::Striker
            || position == PlayerPosition::AttackingMidfielder
            || position == PlayerPosition::LeftWinger
            || position == PlayerPosition::RightWinger;
    }

    double playerAttack(const MatchPlayerSnapshot& player) {
        return player.attributes.technical.shooting * 0.30
            + player.attributes.mental.offTheBall * 0.20
            + player.attributes.mental.composure * 0.18
            + player.attributes.technical.technique * 0.14
            + player.attributes.physical.pace * 0.10
            + player.baseOverall * 0.08;
    }

    double playerCreation(const MatchPlayerSnapshot& player) {
        return player.attributes.technical.passing * 0.27
            + player.attributes.mental.vision * 0.23
            + player.attributes.mental.decisions * 0.15
            + player.attributes.technical.crossing * 0.12
            + player.attributes.technical.technique * 0.13
            + player.attributes.mental.teamwork * 0.10;
    }

    double playerDefense(const MatchPlayerSnapshot& player) {
        return player.attributes.technical.tackling * 0.25
            + player.attributes.technical.marking * 0.23
            + player.attributes.mental.positioning * 0.20
            + player.attributes.mental.concentration * 0.16
            + player.attributes.physical.strength * 0.08
            + player.baseOverall * 0.08;
    }

    double playerPossession(const MatchPlayerSnapshot& player) {
        return player.attributes.technical.passing * 0.24
            + player.attributes.technical.firstTouch * 0.16
            + player.attributes.technical.technique * 0.17
            + player.attributes.mental.decisions * 0.20
            + player.attributes.mental.teamwork * 0.15
            + player.baseOverall * 0.08;
    }

    double playerFinishing(const MatchPlayerSnapshot& player) {
        return player.attributes.technical.shooting * 0.42
            + player.attributes.mental.composure * 0.26
            + player.attributes.technical.technique * 0.14
            + player.attributes.mental.offTheBall * 0.14
            + player.baseOverall * 0.04;
    }

    double playerGoalkeeper(const MatchPlayerSnapshot& player) {
        return player.attributes.goalkeeper.shotStopping * 0.34
            + player.attributes.goalkeeper.handling * 0.20
            + player.attributes.goalkeeper.oneOnOnes * 0.20
            + player.attributes.goalkeeper.aerialAbility * 0.13
            + player.attributes.mental.concentration * 0.08
            + player.baseOverall * 0.05;
    }

    double roleWeight(FormationSlotRole role, PlayerPosition position, double defense, double midfield, double attack) {
        if (isGoalkeeperRole(role, position)) {
            return 0.15;
        }
        if (isAttackingRole(role, position)) {
            return attack;
        }
        if (isMidfieldRole(role, position)) {
            return midfield;
        }
        if (isDefensiveRole(role, position)) {
            return defense;
        }
        return 1.0;
    }

    TeamSummary summarizeTeam(const MatchTeamSnapshot& team) {
        TeamSummary summary;
        summary.teamId = team.teamId;

        double attackTotal = 0.0;
        double attackWeight = 0.0;
        double creationTotal = 0.0;
        double creationWeight = 0.0;
        double finishingTotal = 0.0;
        double finishingWeight = 0.0;
        double defenseTotal = 0.0;
        double defenseWeight = 0.0;
        double possessionTotal = 0.0;
        double possessionWeight = 0.0;
        double goalkeeperTotal = 0.0;
        double goalkeeperWeight = 0.0;

        for (PlayerId playerId : starterIdsFor(team.teamSheet)) {
            const MatchPlayerSnapshot* player = findSnapshot(team, playerId);
            if (player == nullptr) {
                continue;
            }

            const FormationSlotRole role = assignedRoleFor(team.teamSheet, playerId);
            StarterProfile profile;
            profile.player = player;
            profile.role = role;
            profile.scoringWeight = clampDouble(
                playerFinishing(*player)
                    * roleWeight(role, player->position, 0.25, 0.65, 1.35),
                1.0,
                140.0);
            profile.assistWeight = clampDouble(
                playerCreation(*player)
                    * roleWeight(role, player->position, 0.30, 1.25, 1.05),
                1.0,
                140.0);
            summary.starters.push_back(profile);

            const double attackW = roleWeight(role, player->position, 0.35, 0.85, 1.45);
            attackTotal += playerAttack(*player) * attackW;
            attackWeight += attackW;

            const double creationW = roleWeight(role, player->position, 0.45, 1.35, 1.10);
            creationTotal += playerCreation(*player) * creationW;
            creationWeight += creationW;

            const double finishingW = roleWeight(role, player->position, 0.20, 0.65, 1.60);
            finishingTotal += playerFinishing(*player) * finishingW;
            finishingWeight += finishingW;

            const double defenseW = roleWeight(role, player->position, 1.55, 1.00, 0.35);
            defenseTotal += playerDefense(*player) * defenseW;
            defenseWeight += defenseW;

            const double possessionW = roleWeight(role, player->position, 0.75, 1.40, 0.85);
            possessionTotal += playerPossession(*player) * possessionW;
            possessionWeight += possessionW;

            if (isGoalkeeperRole(role, player->position)) {
                goalkeeperTotal += playerGoalkeeper(*player);
                goalkeeperWeight += 1.0;
            }
        }

        summary.attackStrength = attackWeight > 0.0 ? attackTotal / attackWeight : 60.0;
        summary.chanceCreation = creationWeight > 0.0 ? creationTotal / creationWeight : 60.0;
        summary.finishing = finishingWeight > 0.0 ? finishingTotal / finishingWeight : 60.0;
        summary.defensiveStrength = defenseWeight > 0.0 ? defenseTotal / defenseWeight : 60.0;
        summary.possessionStrength = possessionWeight > 0.0 ? possessionTotal / possessionWeight : 60.0;
        summary.goalkeeperStrength = goalkeeperWeight > 0.0 ? goalkeeperTotal / goalkeeperWeight : 60.0;

        const TacticalSetup& tactics = team.tacticalSetup;
        if (tactics.mentality == TeamMentality::Attacking) {
            summary.attackStrength *= 1.06;
            summary.chanceCreation *= 1.05;
            summary.defensiveStrength *= 0.98;
            summary.shotVolumeFactor *= 1.10;
            summary.riskFactor *= 1.08;
        } else if (tactics.mentality == TeamMentality::Defensive) {
            summary.attackStrength *= 0.94;
            summary.chanceCreation *= 0.94;
            summary.defensiveStrength *= 1.08;
            summary.shotVolumeFactor *= 0.90;
            summary.riskFactor *= 0.92;
        }

        if (tactics.tempo == TeamTempo::High) {
            summary.shotVolumeFactor *= 1.08;
            summary.possessionStrength *= 0.98;
        } else if (tactics.tempo == TeamTempo::Low) {
            summary.shotVolumeFactor *= 0.92;
            summary.possessionStrength *= 1.03;
        }

        if (tactics.width == TeamWidth::Wide) {
            summary.chanceCreation *= 1.03;
        } else if (tactics.width == TeamWidth::Narrow) {
            summary.possessionStrength *= 1.02;
        }

        if (tactics.passingDirectness == PassingDirectness::Direct) {
            summary.shotVolumeFactor *= 1.04;
            summary.chanceCreation *= 1.02;
            summary.possessionStrength *= 0.97;
        } else if (tactics.passingDirectness == PassingDirectness::Short) {
            summary.shotVolumeFactor *= 0.96;
            summary.possessionStrength *= 1.04;
        }

        if (tactics.defensiveLine == DefensiveLine::High) {
            summary.chanceCreation *= 1.02;
            summary.defensiveStrength *= 0.98;
            summary.riskFactor *= 1.04;
        } else if (tactics.defensiveLine == DefensiveLine::Low) {
            summary.defensiveStrength *= 1.04;
            summary.shotVolumeFactor *= 0.96;
            summary.riskFactor *= 0.96;
        }

        summary.passAccuracy = clampDouble((summary.possessionStrength / 100.0) * 0.25 + 0.62, 0.65, 0.88);
        return summary;
    }

    MatchPlayerSimulationStats& playerStatsFor(
        MatchEngineResult& result,
        PlayerId playerId,
        TeamId teamId) {
        for (MatchPlayerSimulationStats& stats : result.playerStats) {
            if (stats.playerId == playerId) {
                return stats;
            }
        }

        MatchPlayerSimulationStats stats;
        stats.playerId = playerId;
        stats.teamId = teamId;
        stats.minutesPlayed = 90;
        result.playerStats.push_back(stats);
        return result.playerStats.back();
    }

    const StarterProfile* selectWeightedStarter(
        const std::vector<StarterProfile>& starters,
        std::uint64_t seed,
        std::uint64_t salt,
        bool scorer,
        PlayerId excludePlayerId = 0) {
        double total = 0.0;
        for (const StarterProfile& profile : starters) {
            if (profile.player == nullptr || profile.player->playerId == excludePlayerId) {
                continue;
            }
            total += scorer ? profile.scoringWeight : profile.assistWeight;
        }
        if (total <= 0.0) {
            return nullptr;
        }

        const double target = unit(seed, salt) * total;
        double current = 0.0;
        for (const StarterProfile& profile : starters) {
            if (profile.player == nullptr || profile.player->playerId == excludePlayerId) {
                continue;
            }
            current += scorer ? profile.scoringWeight : profile.assistWeight;
            if (current >= target) {
                return &profile;
            }
        }

        return starters.empty() ? nullptr : &starters.back();
    }

    int sampleGoalsFromShots(
        int shotsOnTarget,
        double expectedGoals,
        double finishing,
        double goalkeeper,
        std::uint64_t seed,
        std::uint64_t salt) {
        if (shotsOnTarget <= 0 || expectedGoals <= 0.01) {
            return 0;
        }

        const double qualityPerShot = clampDouble(expectedGoals / static_cast<double>(shotsOnTarget), 0.035, 0.32);
        const double finishingModifier = clampDouble(0.90 + ((finishing - 60.0) / 220.0), 0.78, 1.18);
        const double keeperModifier = clampDouble(1.05 - ((goalkeeper - 60.0) / 210.0), 0.74, 1.22);
        const double probability = clampDouble(qualityPerShot * finishingModifier * keeperModifier, 0.025, 0.34);

        int goals = 0;
        for (int i = 0; i < shotsOnTarget; ++i) {
            if (unit(seed, salt + static_cast<std::uint64_t>(i) * 17ULL) < probability) {
                ++goals;
            }
        }

        const int softCeiling = std::clamp(static_cast<int>(std::ceil(expectedGoals)) + 2, 3, 6);
        while (goals > softCeiling
            && unit(seed, salt ^ (0x55aaULL + static_cast<std::uint64_t>(goals))) < 0.78) {
            --goals;
        }
        return goals;
    }

    void finalizeRatings(MatchEngineResult& result) {
        for (MatchPlayerSimulationStats& stats : result.playerStats) {
            double rating = 6.0
                + static_cast<double>(stats.goals) * 0.7
                + static_cast<double>(stats.assists) * 0.4
                + static_cast<double>(stats.interceptions) * 0.08
                - static_cast<double>(stats.yellowCards) * 0.1
                - static_cast<double>(stats.redCards) * 0.8;
            stats.rating = clampDouble(rating, 4.0, 10.0);
        }
    }
}

MatchEngineResult FastMatchSummarySimulator::run(const MatchEngineInput& input) const {
    MatchEngineResult result;
    result.homeStats.teamId = input.homeTeam.teamId;
    result.awayStats.teamId = input.awayTeam.teamId;
    result.simulatedSeconds = RegulationMatchSeconds;

    TeamSummary home = summarizeTeam(input.homeTeam);
    TeamSummary away = summarizeTeam(input.awayTeam);
    const std::uint64_t seed = baseSeedFor(input);

    for (const StarterProfile& profile : home.starters) {
        if (profile.player != nullptr) {
            playerStatsFor(result, profile.player->playerId, input.homeTeam.teamId);
        }
    }
    for (const StarterProfile& profile : away.starters) {
        if (profile.player != nullptr) {
            playerStatsFor(result, profile.player->playerId, input.awayTeam.teamId);
        }
    }

    result.homeStats.possessionShare = clampDouble(
        50.0
            + (home.possessionStrength - away.possessionStrength) * 0.18
            + 1.5
            + (unit(seed, 0x101ULL) - 0.5) * 2.0,
        38.0,
        62.0);
    result.awayStats.possessionShare = 100.0 - result.homeStats.possessionShare;

    const double homeChanceRatio = clampDouble(
        (home.chanceCreation / std::max(35.0, away.defensiveStrength)) * away.riskFactor,
        0.70,
        1.35);
    const double awayChanceRatio = clampDouble(
        (away.chanceCreation / std::max(35.0, home.defensiveStrength)) * home.riskFactor,
        0.70,
        1.35);
    const double homeShotMean =
        11.0 * home.shotVolumeFactor * homeChanceRatio * (result.homeStats.possessionShare / 50.0) + 0.8;
    const double awayShotMean =
        10.5 * away.shotVolumeFactor * awayChanceRatio * (result.awayStats.possessionShare / 50.0);

    result.homeStats.shots = roundedClamped(
        homeShotMean + (unit(seed, 0x201ULL) - 0.5) * 4.0,
        5,
        19);
    result.awayStats.shots = roundedClamped(
        awayShotMean + (unit(seed, 0x202ULL) - 0.5) * 4.0,
        4,
        18);

    const double homeSotRate = clampDouble(
        0.32 + (home.finishing - away.goalkeeperStrength) * 0.0012,
        0.24,
        0.45);
    const double awaySotRate = clampDouble(
        0.31 + (away.finishing - home.goalkeeperStrength) * 0.0012,
        0.23,
        0.44);
    result.homeStats.shotsOnTarget = roundedClamped(
        result.homeStats.shots * homeSotRate + (unit(seed, 0x301ULL) - 0.5) * 1.5,
        0,
        result.homeStats.shots);
    result.awayStats.shotsOnTarget = roundedClamped(
        result.awayStats.shots * awaySotRate + (unit(seed, 0x302ULL) - 0.5) * 1.5,
        0,
        result.awayStats.shots);

    const double homeShotQuality = clampDouble(
        0.078
            * clampDouble(home.attackStrength / std::max(40.0, away.defensiveStrength), 0.78, 1.25)
            * clampDouble(home.finishing / std::max(40.0, away.goalkeeperStrength), 0.78, 1.22),
        0.045,
        0.135);
    const double awayShotQuality = clampDouble(
        0.075
            * clampDouble(away.attackStrength / std::max(40.0, home.defensiveStrength), 0.78, 1.25)
            * clampDouble(away.finishing / std::max(40.0, home.goalkeeperStrength), 0.78, 1.22),
        0.043,
        0.130);
    result.homeStats.expectedGoals = clampDouble(
        result.homeStats.shots * homeShotQuality,
        0.25,
        3.20);
    result.awayStats.expectedGoals = clampDouble(
        result.awayStats.shots * awayShotQuality,
        0.20,
        3.00);

    result.homeStats.goals = sampleGoalsFromShots(
        result.homeStats.shotsOnTarget,
        result.homeStats.expectedGoals,
        home.finishing,
        away.goalkeeperStrength,
        seed,
        0x401ULL);
    result.awayStats.goals = sampleGoalsFromShots(
        result.awayStats.shotsOnTarget,
        result.awayStats.expectedGoals,
        away.finishing,
        home.goalkeeperStrength,
        seed,
        0x402ULL);

    const auto fillPassing = [&](MatchTeamSimulationStats& stats, const TeamSummary& summary, std::uint64_t salt) {
        const double tempoPassFactor =
            summary.shotVolumeFactor >= 1.05 ? 0.96 : summary.shotVolumeFactor <= 0.94 ? 1.06 : 1.0;
        stats.passesAttempted = roundedClamped(
            300.0 + stats.possessionShare * 3.2 * tempoPassFactor + (unit(seed, salt) - 0.5) * 36.0,
            220,
            560);
        stats.passesCompleted = roundedClamped(
            stats.passesAttempted * summary.passAccuracy,
            0,
            stats.passesAttempted);
        stats.tacklesAttempted = roundedClamped(15.0 + (50.0 - stats.possessionShare) * 0.16, 8, 26);
        stats.tacklesWon = roundedClamped(stats.tacklesAttempted * 0.58, 0, stats.tacklesAttempted);
        stats.interceptions = roundedClamped(7.0 + (50.0 - stats.possessionShare) * 0.09, 3, 14);
        stats.fouls = roundedClamped(8.0 + (unit(seed, salt ^ 0xf0f0ULL) - 0.5) * 5.0, 4, 15);
        stats.corners = roundedClamped(stats.shots * 0.18 + unit(seed, salt ^ 0xc0c0ULL) * 2.0, 0, 8);
    };
    fillPassing(result.homeStats, home, 0x501ULL);
    fillPassing(result.awayStats, away, 0x502ULL);

    const auto assignGoals = [&](
        const TeamSummary& summary,
        MatchTeamSimulationStats& stats,
        TeamId teamId,
        std::uint64_t salt) {
        for (int i = 0; i < stats.goals; ++i) {
            const StarterProfile* scorer = selectWeightedStarter(
                summary.starters,
                seed,
                salt + static_cast<std::uint64_t>(i) * 41ULL,
                true);
            if (scorer == nullptr || scorer->player == nullptr) {
                continue;
            }

            MatchPlayerSimulationStats& scorerStats =
                playerStatsFor(result, scorer->player->playerId, teamId);
            ++scorerStats.goals;

            PlayerId assistId = 0;
            const bool hasAssist = unit(seed, salt ^ (0xa551ULL + static_cast<std::uint64_t>(i))) < 0.72;
            if (hasAssist) {
                const StarterProfile* assister = selectWeightedStarter(
                    summary.starters,
                    seed,
                    salt + 0x222ULL + static_cast<std::uint64_t>(i) * 53ULL,
                    false,
                    scorer->player->playerId);
                if (assister != nullptr && assister->player != nullptr) {
                    assistId = assister->player->playerId;
                    ++playerStatsFor(result, assistId, teamId).assists;
                }
            }

            result.events.push_back(MatchEventRecord{
                roundedClamped(3.0 + unit(seed, salt + 0x333ULL + static_cast<std::uint64_t>(i) * 61ULL) * 87.0, 1, 90),
                MatchEventKind::Goal,
                teamId,
                scorer->player->playerId,
                assistId
            });
        }
    };
    assignGoals(home, result.homeStats, input.homeTeam.teamId, 0x601ULL);
    assignGoals(away, result.awayStats, input.awayTeam.teamId, 0x602ULL);

    finalizeRatings(result);
    result.report = MatchEngineReportAdapter{}.buildReport(input, result);
    return result;
}
