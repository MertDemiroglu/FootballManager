#include"fm/match_engine/MatchEngine.h"
#include"fm/match_engine/MatchEngineInputBuilder.h"
#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/ball/ShotQualityModel.h"
#include"fm/match_engine/decision/ActionSelector.h"
#include"fm/match_engine/decision/BallCarrierDecisionModel.h"
#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"
#include"fm/match_engine/ball/ShotBlockResolver.h"
#include"fm/match_engine/ball/ShotContextBuilder.h"
#include"fm/match_engine/ball/ShotExecutionModel.h"
#include"fm/match_engine/ball/ShotOutcomeResolver.h"
#include"fm/match_engine/ball/ShotTargetSelector.h"
#include"fm/match_engine/ball/ShotTrajectoryBuilder.h"
#include"fm/competition/League.h"
#include"fm/data/SqliteGameStateRepository.h"
#include"fm/events/DomainEventPublisher.h"
#include"fm/match/PlayMatchCommandHandler.h"
#include"fm/match/TeamSelectionService.h"
#include"fm/roster/FootballerFactory.h"
#include"fm/roster/Team.h"

#include<algorithm>
#include<cstdint>
#include<filesystem>
#include<iostream>
#include<memory>
#include<numeric>
#include<stdexcept>
#include<string>
#include<vector>

namespace {
    void require(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    bool sameAttributes(const PlayerAttributes& lhs, const PlayerAttributes& rhs) {
        return lhs.technical.shooting == rhs.technical.shooting
            && lhs.technical.passing == rhs.technical.passing
            && lhs.technical.firstTouch == rhs.technical.firstTouch
            && lhs.technical.technique == rhs.technical.technique
            && lhs.technical.tackling == rhs.technical.tackling
            && lhs.technical.dribbling == rhs.technical.dribbling
            && lhs.technical.crossing == rhs.technical.crossing
            && lhs.technical.marking == rhs.technical.marking
            && lhs.technical.heading == rhs.technical.heading
            && lhs.technical.setPieces == rhs.technical.setPieces
            && lhs.mental.decisions == rhs.mental.decisions
            && lhs.mental.vision == rhs.mental.vision
            && lhs.mental.positioning == rhs.mental.positioning
            && lhs.mental.offTheBall == rhs.mental.offTheBall
            && lhs.mental.composure == rhs.mental.composure
            && lhs.mental.concentration == rhs.mental.concentration
            && lhs.mental.workRate == rhs.mental.workRate
            && lhs.mental.teamwork == rhs.mental.teamwork
            && lhs.mental.leadership == rhs.mental.leadership
            && lhs.mental.aggression == rhs.mental.aggression
            && lhs.physical.pace == rhs.physical.pace
            && lhs.physical.acceleration == rhs.physical.acceleration
            && lhs.physical.stamina == rhs.physical.stamina
            && lhs.physical.strength == rhs.physical.strength
            && lhs.physical.agility == rhs.physical.agility
            && lhs.physical.jumpingReach == rhs.physical.jumpingReach
            && lhs.goalkeeper.shotStopping == rhs.goalkeeper.shotStopping
            && lhs.goalkeeper.handling == rhs.goalkeeper.handling
            && lhs.goalkeeper.aerialAbility == rhs.goalkeeper.aerialAbility
            && lhs.goalkeeper.oneOnOnes == rhs.goalkeeper.oneOnOnes
            && lhs.goalkeeper.distribution == rhs.goalkeeper.distribution;
    }

    std::unique_ptr<Footballer> makePlayer(
        const std::string& name,
        const std::string& position,
        int rating) {
        auto player = FootballerFactory::create(
            name,
            24,
            position,
            "",
            rating,
            rating,
            rating,
            rating,
            rating);
        require(player != nullptr, "failed to create test player");
        return player;
    }

    Team makeTeam(TeamId teamId, const std::string& name, int ratingOffset) {
        Team team(teamId, name);
        const std::vector<std::string> positions{
            "GK", "LB", "CB", "CB", "RB", "LM", "CM", "CM", "RM", "ST", "ST"
        };

        int index = 0;
        for (const std::string& position : positions) {
            team.addPlayer(makePlayer(
                name + " Player " + std::to_string(index + 1),
                position,
                62 + ratingOffset + (index % 5)));
            ++index;
        }

        return team;
    }

    bool hasStarterReports(const MatchReport& report, const TeamSheet& homeSheet, const TeamSheet& awaySheet) {
        const auto containsStartedReport = [&report](TeamId teamId, PlayerId playerId) {
            return std::any_of(
                report.playerReports.begin(),
                report.playerReports.end(),
                [teamId, playerId](const MatchPlayerReport& playerReport) {
                    return playerReport.teamId == teamId
                        && playerReport.playerId == playerId
                        && playerReport.started;
                });
        };

        for (PlayerId playerId : homeSheet.startingPlayerIds) {
            if (!containsStartedReport(homeSheet.teamId, playerId)) {
                return false;
            }
        }
        for (PlayerId playerId : awaySheet.startingPlayerIds) {
            if (!containsStartedReport(awaySheet.teamId, playerId)) {
                return false;
            }
        }

        return true;
    }

    MatchEngineInput buildInput(
        MatchSimulationDetail detail,
        std::uint64_t deterministicSeed = 0x51f0c0deULL,
        int homeRatingOffset = 4,
        int awayRatingOffset = 0,
        TacticalSetup homeTactics = TacticalSetup{},
        TacticalSetup awayTactics = TacticalSetup{}) {
        Team homeTeam = makeTeam(101, "Home Smoke", homeRatingOffset);
        Team awayTeam = makeTeam(202, "Away Smoke", awayRatingOffset);

        TeamSelectionService selectionService;
        TeamSheet homeSheet = selectionService.buildTeamSheet(homeTeam, FormationId::FourFourTwo);
        TeamSheet awaySheet = selectionService.buildTeamSheet(awayTeam, FormationId::FourFourTwo);
        homeSheet.tacticalSetup = homeTactics;
        awaySheet.tacticalSetup = awayTactics;

        MatchEngineOptions options;
        options.detail = detail;
        options.deterministicSeed = deterministicSeed;

        return MatchEngineInputBuilder{}.build(
            303,
            404,
            2026,
            7,
            Date{ 2026, Month::March, 14 },
            homeTeam,
            awayTeam,
            homeSheet,
            awaySheet,
            options);
    }

    double passAccuracyFor(const MatchTeamSimulationStats& stats) {
        return stats.passesAttempted > 0
            ? static_cast<double>(stats.passesCompleted) / static_cast<double>(stats.passesAttempted)
            : 0.0;
    }

    std::vector<PlayerId> goalkeeperIdsFor(const MatchTeamSnapshot& team) {
        std::vector<PlayerId> ids;
        for (const TeamSheetSlotAssignment& assignment : team.teamSheet.startingAssignments) {
            if (assignment.slotRole == FormationSlotRole::Goalkeeper && assignment.playerId != 0) {
                ids.push_back(assignment.playerId);
            }
        }
        return ids;
    }

    bool containsPlayerId(const std::vector<PlayerId>& ids, PlayerId playerId) {
        return std::find(ids.begin(), ids.end(), playerId) != ids.end();
    }

    PlayerAttributes passDecisionAttributes(int passing, int vision, int decisions) {
        PlayerAttributes attributes;
        attributes.technical.passing = passing;
        attributes.technical.technique = std::max(35, passing - 4);
        attributes.technical.crossing = std::max(30, passing - 6);
        attributes.technical.dribbling = 55;
        attributes.technical.shooting = 50;
        attributes.mental.vision = vision;
        attributes.mental.decisions = decisions;
        attributes.mental.composure = std::max(35, decisions - 2);
        attributes.mental.offTheBall = 58;
        attributes.physical.pace = 62;
        attributes.physical.acceleration = 62;
        attributes.physical.agility = 58;
        return attributes;
    }

    PlayerAttributes carryDecisionAttributes(int dribbling, int technique, int decisions) {
        PlayerAttributes attributes = passDecisionAttributes(58, 56, decisions);
        attributes.technical.dribbling = dribbling;
        attributes.technical.technique = technique;
        attributes.mental.composure = std::max(35, decisions - 1);
        attributes.physical.pace = std::max(35, dribbling - 2);
        attributes.physical.acceleration = std::max(35, dribbling);
        attributes.physical.agility = std::max(35, technique - 1);
        return attributes;
    }

    PlayerAttributes shotDecisionAttributes(int shooting, int technique, int composure) {
        PlayerAttributes attributes = passDecisionAttributes(58, 56, std::max(35, composure - 2));
        attributes.technical.shooting = shooting;
        attributes.technical.technique = technique;
        attributes.mental.composure = composure;
        attributes.mental.offTheBall = std::max(35, shooting - 4);
        attributes.physical.agility = std::max(35, technique - 3);
        return attributes;
    }

    struct PassDecisionFixture {
        MatchTeamSnapshot teamSnapshot;
        MatchTeamSnapshot opponentSnapshot;
        TeamSimState teamState;
        TeamSimState opponentState;
        PlayerSimState carrier;
        TacticalSetup tactics;
        FormationSlotRole carrierRole = FormationSlotRole::CentralMidfielder;
        AttackingDirection direction = AttackingDirection::HomeToAway;
    };

    void addPassDecisionPlayer(
        PassDecisionFixture& fixture,
        PlayerId playerId,
        PitchPoint position,
        FormationSlotRole role,
        const PlayerAttributes& attributes) {
        fixture.teamState.players.push_back(PlayerSimState{
            playerId,
            fixture.teamState.teamId,
            TeamSide::Home,
            position,
            position,
            PlayerIntent{},
            playerId == fixture.carrier.playerId,
            0.0,
            6.8,
            7.1,
            65
        });
        fixture.teamSnapshot.players.push_back(MatchPlayerSnapshot{
            playerId,
            fixture.teamState.teamId,
            PlayerPosition::Unknown,
            attributes,
            PlayerConditionState{},
            65,
            65
        });
        fixture.teamSnapshot.teamSheet.startingAssignments.push_back(TeamSheetSlotAssignment{
            fixture.teamSnapshot.teamSheet.startingAssignments.size(),
            role,
            playerId
        });
    }

    void addPassDecisionOpponent(
        PassDecisionFixture& fixture,
        PlayerId playerId,
        PitchPoint position) {
        fixture.opponentState.players.push_back(PlayerSimState{
            playerId,
            fixture.opponentState.teamId,
            TeamSide::Away,
            position,
            position,
            PlayerIntent{},
            false,
            0.0,
            6.8,
            7.1,
            64
        });
        fixture.opponentSnapshot.players.push_back(MatchPlayerSnapshot{
            playerId,
            fixture.opponentState.teamId,
            PlayerPosition::Unknown,
            passDecisionAttributes(55, 55, 55),
            PlayerConditionState{},
            64,
            64
        });
    }

    PassDecisionFixture buildPassDecisionFixture(
        FormationSlotRole carrierRole,
        PlayerAttributes carrierAttributes,
        TacticalSetup tactics = TacticalSetup{},
        PitchPoint carrierPosition = PitchPoint{ 35.0, 34.0 }) {
        PassDecisionFixture fixture;
        fixture.teamSnapshot.teamId = 501;
        fixture.teamSnapshot.teamName = "Pass Decision Home";
        fixture.teamSnapshot.tacticalSetup = tactics;
        fixture.teamSnapshot.teamSheet.teamId = 501;
        fixture.teamSnapshot.teamSheet.tacticalSetup = tactics;
        fixture.opponentSnapshot.teamId = 502;
        fixture.opponentSnapshot.teamName = "Pass Decision Away";
        fixture.opponentSnapshot.teamSheet.teamId = 502;
        fixture.teamState.teamId = 501;
        fixture.teamState.side = TeamSide::Home;
        fixture.teamState.tacticalSetup = tactics;
        fixture.opponentState.teamId = 502;
        fixture.opponentState.side = TeamSide::Away;
        fixture.carrierRole = carrierRole;
        fixture.tactics = tactics;
        fixture.carrier = PlayerSimState{
            1,
            501,
            TeamSide::Home,
            carrierPosition,
            carrierPosition,
            PlayerIntent{},
            true,
            0.0,
            6.8,
            7.1,
            66
        };

        addPassDecisionPlayer(fixture, 1, carrierPosition, carrierRole, carrierAttributes);
        addPassDecisionPlayer(
            fixture,
            2,
            PitchPoint{ carrierPosition.x - 7.0, carrierPosition.y - 8.0 },
            FormationSlotRole::DefensiveMidfielder,
            passDecisionAttributes(62, 58, 60));
        addPassDecisionPlayer(
            fixture,
            3,
            PitchPoint{ carrierPosition.x + 30.0, carrierPosition.y },
            FormationSlotRole::AttackingMidfielder,
            passDecisionAttributes(66, 64, 63));
        addPassDecisionPlayer(
            fixture,
            4,
            PitchPoint{ carrierPosition.x + 18.0, 60.0 },
            FormationSlotRole::RightWinger,
            passDecisionAttributes(66, 63, 62));
        addPassDecisionPlayer(
            fixture,
            5,
            PitchPoint{ carrierPosition.x + 36.0, 33.0 },
            FormationSlotRole::Striker,
            passDecisionAttributes(60, 58, 60));

        addPassDecisionOpponent(fixture, 101, PitchPoint{ carrierPosition.x + 8.0, carrierPosition.y + 12.0 });
        addPassDecisionOpponent(fixture, 102, PitchPoint{ carrierPosition.x + 19.0, carrierPosition.y - 13.0 });
        addPassDecisionOpponent(fixture, 103, PitchPoint{ carrierPosition.x + 38.0, carrierPosition.y + 10.0 });
        return fixture;
    }

    std::vector<PassOption> evaluatePassFixture(const PassDecisionFixture& fixture) {
        return PassOptionEvaluator{}.evaluate(PassOptionEvaluationContext{
            &fixture.teamSnapshot,
            &fixture.opponentSnapshot,
            &fixture.teamState,
            &fixture.opponentState,
            &fixture.carrier,
            fixture.carrierRole,
            fixture.tactics,
            fixture.carrier.position,
            fixture.direction,
            8.0
        });
    }

    double bestScoreForKind(const std::vector<PassOption>& options, PassOptionKind kind) {
        double best = 0.0;
        for (const PassOption& option : options) {
            if (option.kind == kind) {
                best = std::max(best, option.score);
            }
        }
        return best;
    }

    double bestScoreForKind(const std::vector<CarryOption>& options, CarryOptionKind kind) {
        double best = 0.0;
        for (const CarryOption& option : options) {
            if (option.kind == kind) {
                best = std::max(best, option.score);
            }
        }
        return best;
    }

    double bestZoneRiskForKind(const std::vector<CarryOption>& options, CarryOptionKind kind) {
        double bestRisk = 0.0;
        for (const CarryOption& option : options) {
            if (option.kind == kind) {
                bestRisk = std::max(bestRisk, option.zoneLimitRisk);
            }
        }
        return bestRisk;
    }

    double bestShotScore(const std::vector<ShotOption>& options) {
        double best = 0.0;
        for (const ShotOption& option : options) {
            best = std::max(best, option.score);
        }
        return best;
    }

    double bestCandidateScoreForType(
        const std::vector<ActionCandidate>& candidates,
        BallCarrierActionType type) {
        double best = 0.0;
        for (const ActionCandidate& candidate : candidates) {
            if (candidate.type == type) {
                best = std::max(best, candidate.finalScore);
            }
        }
        return best;
    }

    int traceCountFor(const MatchEngineResult& result, MatchTraceKind kind) {
        return static_cast<int>(std::count_if(
            result.traceFrames.begin(),
            result.traceFrames.end(),
            [kind](const MatchTraceFrame& frame) {
                return frame.kind == kind;
            }));
    }

    int countKind(const std::vector<PassOption>& options, PassOptionKind kind) {
        return static_cast<int>(std::count_if(
            options.begin(),
            options.end(),
            [kind](const PassOption& option) {
                return option.kind == kind;
            }));
    }

    const PassOption* findOptionForTarget(
        const std::vector<PassOption>& options,
        PassOptionKind kind,
        PlayerId targetPlayerId) {
        for (const PassOption& option : options) {
            if (option.kind == kind && option.targetPlayerId == targetPlayerId) {
                return &option;
            }
        }
        return nullptr;
    }

    std::vector<CarryOption> evaluateCarryFixture(
        const PassDecisionFixture& fixture,
        double pressure = 8.0) {
        return CarryOptionEvaluator{}.evaluate(CarryOptionEvaluationContext{
            &fixture.teamSnapshot,
            &fixture.opponentSnapshot,
            &fixture.teamState,
            &fixture.opponentState,
            &fixture.carrier,
            fixture.carrierRole,
            fixture.tactics,
            fixture.carrier.position,
            fixture.direction,
            pressure
        });
    }

    std::vector<ShotOption> evaluateShotFixture(
        const PassDecisionFixture& fixture,
        double pressure = 8.0) {
        return ShotDecisionEvaluator{}.evaluate(ShotOptionEvaluationContext{
            &fixture.teamSnapshot,
            &fixture.opponentSnapshot,
            &fixture.teamState,
            &fixture.opponentState,
            &fixture.carrier,
            fixture.carrierRole,
            fixture.tactics,
            fixture.carrier.position,
            fixture.direction,
            pressure
        });
    }

    void runShotQualityModelSmoke() {
        const double closeCentral = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 97.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            8.0);
        const double farWide = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 70.0, 6.0 },
            AttackingDirection::HomeToAway,
            8.0);
        const double pressured = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 97.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            70.0);
        const double reverseDirection = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 8.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::AwayToHome,
            8.0);

        require(closeCentral > farWide, "close central xG should exceed far wide xG");
        require(closeCentral > pressured, "higher pressure should reduce xG");
        require(reverseDirection > farWide, "away-to-home xG should use the opposite goal");
        for (double xg : { closeCentral, farWide, pressured, reverseDirection }) {
            require(xg >= 0.004 && xg <= 0.28, "open-play xG should stay within clamp range");
        }
    }

    PlayerAttributes goalkeeperAttributes(int shotStopping, int handling) {
        PlayerAttributes attributes = passDecisionAttributes(42, 50, 58);
        attributes.goalkeeper.shotStopping = shotStopping;
        attributes.goalkeeper.handling = handling;
        attributes.goalkeeper.oneOnOnes = std::max(35, shotStopping - 4);
        attributes.goalkeeper.aerialAbility = std::max(35, handling - 6);
        attributes.mental.positioning = std::max(35, shotStopping - 5);
        attributes.mental.concentration = std::max(35, handling - 3);
        return attributes;
    }

    ShotContext buildShotContextForSmoke(
        PitchPoint origin,
        PlayerAttributes shooter,
        double pressure,
        std::vector<PitchPoint> defenderPositions = {},
        PlayerAttributes goalkeeper = goalkeeperAttributes(62, 58),
        std::uint64_t seed = 0x5eedULL) {
        return ShotContextBuilder{}.build(ShotContextBuildRequest{
            origin,
            AttackingDirection::HomeToAway,
            pressure,
            shooter,
            goalkeeper,
            62.0,
            defenderPositions,
            seed
        });
    }

    ShotTargetSelectionResult fixedShotTarget(const ShotContext& context) {
        return ShotTargetSelectionResult{
            ShotTargetZone{ ShotTargetLane::FarPost, ShotTargetHeight::Low },
            shotTargetPointFor(
                context.shotOrigin,
                context.attackingDirection,
                ShotTargetZone{ ShotTargetLane::FarPost, ShotTargetHeight::Low }),
            32.0,
            70.0
        };
    }

    void runShotExecutionModelSmoke() {
        const ShotContext goodContext = buildShotContextForSmoke(
            PitchPoint{ 96.0, PitchGeometry::WidthMeters / 2.0 },
            shotDecisionAttributes(86, 84, 82),
            6.0,
            {},
            goalkeeperAttributes(62, 58),
            0x8801ULL);
        const ShotContext hardContext = buildShotContextForSmoke(
            PitchPoint{ 86.0, 8.0 },
            shotDecisionAttributes(42, 40, 38),
            72.0,
            { PitchPoint{ 88.0, 12.0 }, PitchPoint{ 91.0, 18.0 } },
            goalkeeperAttributes(62, 58),
            0x8801ULL);
        const ShotTargetSelectionResult intended = fixedShotTarget(goodContext);
        const ShotExecutionResult good = ShotExecutionModel{}.execute(ShotExecutionRequest{
            goodContext,
            ShotType::PlacedShot,
            intended
        });
        const ShotExecutionResult poor = ShotExecutionModel{}.execute(ShotExecutionRequest{
            hardContext,
            ShotType::TightAngleShot,
            ShotTargetSelectionResult{
                intended.intendedZone,
                shotTargetPointFor(hardContext.shotOrigin, hardContext.attackingDirection, intended.intendedZone),
                intended.targetDifficulty,
                intended.placementQuality
            }
        });

        require(good.executionQuality > poor.executionQuality,
            "good low-pressure shooter should execute better than poor high-pressure tight-angle shooter");
        require(good.targetDeviationMeters < poor.targetDeviationMeters,
            "good low-pressure shooter should deviate less from intended target");
        require(PitchGeometry::distance(intended.intendedTarget, good.actualTarget) > 0.0
                || PitchGeometry::distance(intended.intendedTarget, poor.actualTarget) > 0.0,
            "intended target and actual executed target should be separate concepts");
    }

    void runShotQualityFoundationSmoke() {
        const PlayerAttributes averageShooter = shotDecisionAttributes(62, 60, 60);
        const PlayerAttributes eliteShooter = shotDecisionAttributes(92, 88, 86);
        const ShotContext closeCentral = buildShotContextForSmoke(
            PitchPoint{ 98.0, PitchGeometry::WidthMeters / 2.0 },
            averageShooter,
            8.0);
        const ShotContext longShot = buildShotContextForSmoke(
            PitchPoint{ 72.0, PitchGeometry::WidthMeters / 2.0 },
            averageShooter,
            8.0);
        const ShotContext tightAngle = buildShotContextForSmoke(
            PitchPoint{ 97.0, 6.0 },
            averageShooter,
            8.0);
        const ShotContext pressured = buildShotContextForSmoke(
            PitchPoint{ 98.0, PitchGeometry::WidthMeters / 2.0 },
            averageShooter,
            76.0);
        const ShotContext blockedLane = buildShotContextForSmoke(
            PitchPoint{ 98.0, PitchGeometry::WidthMeters / 2.0 },
            averageShooter,
            8.0,
            { PitchPoint{ 101.0, PitchGeometry::WidthMeters / 2.0 } });
        const ShotContext eliteSameLocation = buildShotContextForSmoke(
            PitchPoint{ 98.0, PitchGeometry::WidthMeters / 2.0 },
            eliteShooter,
            8.0);

        const ShotTargetSelectionResult intended = fixedShotTarget(closeCentral);
        const ShotExecutionResult execution = ShotExecutionModel{}.execute(ShotExecutionRequest{
            closeCentral,
            ShotType::PlacedShot,
            intended
        });
        const auto qualityFor = [&execution](const ShotContext& context, ShotType type) {
            return ShotQualityModel{}.evaluate(context, type, execution);
        };

        const ShotQualityResult closeQuality = qualityFor(closeCentral, ShotType::PlacedShot);
        const ShotQualityResult longQuality = qualityFor(longShot, ShotType::LongShot);
        const ShotQualityResult tightQuality = qualityFor(tightAngle, ShotType::TightAngleShot);
        const ShotQualityResult pressuredQuality = qualityFor(pressured, ShotType::PlacedShot);
        const ShotQualityResult blockedQuality = qualityFor(blockedLane, ShotType::PlacedShot);
        const ShotQualityResult eliteQuality = qualityFor(eliteSameLocation, ShotType::PlacedShot);

        require(closeQuality.adjustedXG > longQuality.adjustedXG,
            "central close shot should have higher adjusted xG than long shot");
        require(closeQuality.adjustedXG > tightQuality.adjustedXG,
            "tight angle should lower adjusted shot quality");
        require(closeQuality.adjustedXG > pressuredQuality.adjustedXG,
            "pressure should lower adjusted shot quality");
        require(blockedQuality.blockRisk > closeQuality.blockRisk,
            "high blocker risk should raise block risk");
        require(std::abs(eliteQuality.adjustedXG - closeQuality.adjustedXG) < 0.015,
            "player finishing should not massively inflate recorded xG");
    }

    void runShotOutcomeFoundationSmoke() {
        const PlayerAttributes shooter = shotDecisionAttributes(68, 66, 64);
        const ShotContext context = buildShotContextForSmoke(
            PitchPoint{ 96.0, PitchGeometry::WidthMeters / 2.0 },
            shooter,
            18.0,
            { PitchPoint{ 99.0, PitchGeometry::WidthMeters / 2.0 } },
            goalkeeperAttributes(68, 62),
            0x4400ULL);
        const ShotTargetSelectionResult intended = fixedShotTarget(context);
        const ShotExecutionResult execution = ShotExecutionModel{}.execute(ShotExecutionRequest{
            context,
            ShotType::PlacedShot,
            intended
        });
        const ShotQualityResult quality = ShotQualityModel{}.evaluate(context, ShotType::PlacedShot, execution);
        const BallTrajectory trajectory = ShotTrajectoryBuilder{}.build(ShotTrajectoryBuildRequest{
            context,
            intended,
            execution,
            10.0
        });

        bool blockedOccurred = false;
        for (std::uint64_t seed = 1; seed <= 160 && !blockedOccurred; ++seed) {
            const ShotBlockResult block = ShotBlockResolver{}.resolve(ShotBlockRequest{
                trajectory,
                context,
                quality,
                execution,
                { ShotBlocker{
                    9001,
                    502,
                    PitchPoint{ 99.0, PitchGeometry::WidthMeters / 2.0 },
                    passDecisionAttributes(58, 58, 62),
                    68
                } },
                seed
            });
            blockedOccurred = block.blocked;
        }
        require(blockedOccurred, "blocked shots should be able to occur");

        bool offTargetOccurred = false;
        bool savedHeldOccurred = false;
        bool savedReboundOccurred = false;
        int highKeeperGoals = 0;
        int lowKeeperGoals = 0;
        int highHandlingHeld = 0;
        int lowHandlingHeld = 0;
        for (std::uint64_t seed = 1; seed <= 260; ++seed) {
            ShotContext poorContext = buildShotContextForSmoke(
                PitchPoint{ 84.0, 8.0 },
                shotDecisionAttributes(38, 36, 35),
                82.0,
                {},
                goalkeeperAttributes(64, 58),
                seed);
            ShotExecutionResult poorExecution = ShotExecutionModel{}.execute(ShotExecutionRequest{
                poorContext,
                ShotType::DesperationShot,
                ShotTargetSelector{}.select(poorContext, ShotType::DesperationShot)
            });
            ShotQualityResult poorQuality =
                ShotQualityModel{}.evaluate(poorContext, ShotType::DesperationShot, poorExecution);
            ShotOutcomeResult poorOutcome = ShotOutcomeResolver{}.resolve(ShotOutcomeContext{
                poorContext,
                ShotType::DesperationShot,
                poorExecution,
                poorQuality,
                seed
            });
            offTargetOccurred = offTargetOccurred || poorOutcome.kind == ShotOutcomeKind::OffTarget;

            ShotContext highKeeperContext = buildShotContextForSmoke(
                PitchPoint{ 95.0, PitchGeometry::WidthMeters / 2.0 },
                shooter,
                12.0,
                {},
                goalkeeperAttributes(88, 88),
                seed);
            ShotContext lowKeeperContext = buildShotContextForSmoke(
                PitchPoint{ 95.0, PitchGeometry::WidthMeters / 2.0 },
                shooter,
                12.0,
                {},
                goalkeeperAttributes(42, 34),
                seed);
            ShotExecutionResult onTargetExecution = ShotExecutionModel{}.execute(ShotExecutionRequest{
                highKeeperContext,
                ShotType::PlacedShot,
                ShotTargetSelector{}.select(highKeeperContext, ShotType::PlacedShot)
            });
            ShotQualityResult onTargetQuality =
                ShotQualityModel{}.evaluate(highKeeperContext, ShotType::PlacedShot, onTargetExecution);
            const ShotOutcomeResult highOutcome = GoalkeeperSaveResolver{}.resolveOnTarget(ShotOutcomeContext{
                highKeeperContext,
                ShotType::PlacedShot,
                onTargetExecution,
                onTargetQuality,
                seed
            });
            const ShotOutcomeResult lowOutcome = GoalkeeperSaveResolver{}.resolveOnTarget(ShotOutcomeContext{
                lowKeeperContext,
                ShotType::PlacedShot,
                onTargetExecution,
                ShotQualityModel{}.evaluate(lowKeeperContext, ShotType::PlacedShot, onTargetExecution),
                seed
            });
            savedHeldOccurred = savedHeldOccurred || highOutcome.kind == ShotOutcomeKind::SavedHeld;
            savedReboundOccurred = savedReboundOccurred || highOutcome.kind == ShotOutcomeKind::SavedRebound;
            highKeeperGoals += highOutcome.goal ? 1 : 0;
            lowKeeperGoals += lowOutcome.goal ? 1 : 0;
            highHandlingHeld += highOutcome.kind == ShotOutcomeKind::SavedHeld ? 1 : 0;
            lowHandlingHeld += lowOutcome.kind == ShotOutcomeKind::SavedHeld ? 1 : 0;
        }

        require(offTargetOccurred, "off-target shots should be able to occur");
        require(savedHeldOccurred, "saved-held outcomes should be able to occur");
        require(savedReboundOccurred, "saved-rebound outcomes should be able to occur");
        require(highHandlingHeld > lowHandlingHeld,
            "goalkeeper handling should increase held-save tendency");
        require(highKeeperGoals < lowKeeperGoals,
            "goalkeeper quality should affect save chance");
    }

    void assertResultReportConsistency(
        const MatchEngineInput& input,
        const MatchEngineResult& result) {
        require(result.report.has_value(), "valid match input should produce a report");

        const MatchReport& report = *result.report;
        require(report.homeGoals == result.homeStats.goals, "home report goals should match result stats");
        require(report.awayGoals == result.awayStats.goals, "away report goals should match result stats");
        require(report.homeStats.goals == report.homeGoals, "home report team stats should match score");
        require(report.awayStats.goals == report.awayGoals, "away report team stats should match score");
        require(report.homeId == input.homeTeam.teamId, "home report team id should match input");
        require(report.awayId == input.awayTeam.teamId, "away report team id should match input");
        require(report.matchId == input.matchId, "report match id should match input");
        require(report.leagueId == input.leagueId, "report league id should match input");
        require(report.seasonYear == input.seasonYear, "report season year should match input");
        require(report.matchweek == input.matchweek, "report matchweek should match input");
        require(hasStarterReports(report, input.homeTeam.teamSheet, input.awayTeam.teamSheet),
            "report should contain started player reports for starters");
        require(result.simulatedSeconds >= 5400, "coordinate match should simulate regulation time");
        require(report.homeStats.shots >= report.homeStats.shotsOnTarget,
            "home shots should cover shots on target");
        require(report.awayStats.shots >= report.awayStats.shotsOnTarget,
            "away shots should cover shots on target");
        require(report.homeStats.passesAttempted >= report.homeStats.passesCompleted,
            "home passes attempted should cover completed passes");
        require(report.awayStats.passesAttempted >= report.awayStats.passesCompleted,
            "away passes attempted should cover completed passes");
        require(report.homeStats.possessionShare >= 0.0 && report.awayStats.possessionShare >= 0.0,
            "possession shares should be populated");
        require(report.homeStats.possessionShare + report.awayStats.possessionShare > 99.0,
            "possession shares should represent the match");
        for (const MatchPlayerReport& playerReport : report.playerReports) {
            require(playerReport.rating >= 0.0 && playerReport.rating <= 10.0,
                "player report ratings should be present and bounded");
        }
    }

    void runDeterministicRegression() {
        const MatchEngineInput input = buildInput(MatchSimulationDetail::DebugFullTrace);
        const MatchEngine engine;

        const MatchEngineResult first = engine.simulate(input);
        const MatchEngineResult second = engine.simulate(input);

        assertResultReportConsistency(input, first);
        assertResultReportConsistency(input, second);

        require(first.homeStats.goals == second.homeStats.goals, "home goals should be deterministic");
        require(first.awayStats.goals == second.awayStats.goals, "away goals should be deterministic");
        require(first.report->homeGoals == second.report->homeGoals, "home report goals should be deterministic");
        require(first.report->awayGoals == second.report->awayGoals, "away report goals should be deterministic");
        require(first.traceFrames.size() == second.traceFrames.size(), "debug trace frame count should be deterministic");
    }

    void runDetailSmoke(MatchSimulationDetail detail) {
        const MatchEngineInput input = buildInput(detail);
        const MatchEngineResult result = MatchEngine{}.simulate(input);
        assertResultReportConsistency(input, result);
        if (detail == MatchSimulationDetail::BackgroundSummary) {
            require(result.traceFrames.empty(), "background summary should route to fast no-trace simulation");
        } else {
            require(!result.traceFrames.empty(), "watched/debug detail should route to detailed coordinate simulation");
        }
    }

    void runOfficialGoalEventSmoke() {
        for (std::uint64_t seed = 1; seed <= 40; ++seed) {
            const MatchEngineInput input = buildInput(MatchSimulationDetail::BackgroundSummary, seed);
            const MatchEngineResult result = MatchEngine{}.simulate(input);
            assertResultReportConsistency(input, result);

            const int totalGoals = result.homeStats.goals + result.awayStats.goals;
            if (totalGoals <= 0) {
                continue;
            }

            const auto goalEventCount = [](const std::vector<MatchEventRecord>& events) {
                return static_cast<int>(std::count_if(
                    events.begin(),
                    events.end(),
                    [](const MatchEventRecord& event) {
                        return event.kind == MatchEventKind::Goal;
                    }));
            };
            const int playerReportGoals = std::accumulate(
                result.report->playerReports.begin(),
                result.report->playerReports.end(),
                0,
                [](int total, const MatchPlayerReport& report) {
                    return total + report.goals;
                });

            require(goalEventCount(result.events) == totalGoals,
                "official result goal events should match score in background mode");
            require(goalEventCount(result.report->events) == totalGoals,
                "report goal events should match score in background mode");
            require(playerReportGoals == totalGoals,
                "player report goals should match score when goals are scored");
            require(result.traceFrames.empty(), "background summary should not need debug trace for goal events");
            return;
        }

        throw std::runtime_error("goal event smoke could not find a deterministic goal sample");
    }

    void runBackgroundSummaryCalibrationSmoke() {
        const MatchEngineInput input = buildInput(MatchSimulationDetail::BackgroundSummary, 0x778899ULL);
        const MatchEngineResult first = MatchEngine{}.simulate(input);
        const MatchEngineResult second = MatchEngine{}.simulate(input);

        assertResultReportConsistency(input, first);
        assertResultReportConsistency(input, second);
        require(first.traceFrames.empty(), "background summary should not emit marker trace");
        require(first.homeStats.goals == second.homeStats.goals
            && first.awayStats.goals == second.awayStats.goals,
            "background summary score should be deterministic");
        require(first.homeStats.shots == second.homeStats.shots
            && first.awayStats.shots == second.awayStats.shots,
            "background summary shot volume should be deterministic");
        require(first.events.size() == second.events.size(),
            "background summary official events should be deterministic");

        int extremeGoalMatches = 0;
        int assistedGoalSamples = 0;
        for (std::uint64_t seed = 1; seed <= 24; ++seed) {
            const MatchEngineInput sampleInput =
                buildInput(MatchSimulationDetail::BackgroundSummary, seed + 0x9000ULL);
            const MatchEngineResult sample = MatchEngine{}.simulate(sampleInput);
            assertResultReportConsistency(sampleInput, sample);
            const int combinedGoals = sample.homeStats.goals + sample.awayStats.goals;
            const int combinedShots = sample.homeStats.shots + sample.awayStats.shots;
            const double combinedXg =
                sample.homeStats.expectedGoals + sample.awayStats.expectedGoals;
            require(combinedShots >= 10 && combinedShots <= 46,
                "background summary combined shots should stay in a playable range");
            require(combinedXg >= 0.3 && combinedXg <= 7.0,
                "background summary xG should stay in a playable range");
            require(combinedGoals <= static_cast<int>(std::ceil(combinedXg)) + 5,
                "background summary goals should stay directionally consistent with xG");
            if (combinedGoals >= 8) {
                ++extremeGoalMatches;
            }
            assistedGoalSamples += static_cast<int>(std::count_if(
                sample.report->events.begin(),
                sample.report->events.end(),
                [](const MatchEventRecord& event) {
                    return event.kind == MatchEventKind::Goal && event.secondaryPlayerId != 0;
                }));
        }
        require(extremeGoalMatches <= 1,
            "background summary should not regularly produce extreme scorelines");
        require(assistedGoalSamples > 0,
            "background summary should produce assisted goals across deterministic samples");
    }

    void runGoalkeeperScorerSafetySmoke() {
        for (std::uint64_t seed = 1; seed <= 80; ++seed) {
            const MatchEngineInput input =
                buildInput(MatchSimulationDetail::BackgroundSummary, seed + 0x6600ULL);
            const std::vector<PlayerId> homeGoalkeepers = goalkeeperIdsFor(input.homeTeam);
            const std::vector<PlayerId> awayGoalkeepers = goalkeeperIdsFor(input.awayTeam);
            const MatchEngineResult result = MatchEngine{}.simulate(input);
            assertResultReportConsistency(input, result);

            for (const MatchEventRecord& event : result.report->events) {
                if (event.kind != MatchEventKind::Goal) {
                    continue;
                }
                require(!containsPlayerId(homeGoalkeepers, event.primaryPlayerId)
                    && !containsPlayerId(awayGoalkeepers, event.primaryPlayerId),
                    "fast summary should not assign open-play goals to goalkeepers");
            }

            for (const MatchPlayerReport& playerReport : result.report->playerReports) {
                if (containsPlayerId(homeGoalkeepers, playerReport.playerId)
                    || containsPlayerId(awayGoalkeepers, playerReport.playerId)) {
                    require(playerReport.goals == 0,
                        "fast summary goalkeeper player reports should not contain open-play goals");
                }
            }
        }
    }

    void runFastSummaryTacticalSensitivitySmoke() {
        TacticalSetup attacking;
        attacking.mentality = TeamMentality::Attacking;
        TacticalSetup defensive;
        defensive.mentality = TeamMentality::Defensive;
        const MatchEngineResult attackResult = MatchEngine{}.simulate(
            buildInput(MatchSimulationDetail::BackgroundSummary, 0x7100ULL, 4, 0, attacking));
        const MatchEngineResult defensiveResult = MatchEngine{}.simulate(
            buildInput(MatchSimulationDetail::BackgroundSummary, 0x7100ULL, 4, 0, defensive));
        require(attackResult.homeStats.shots > defensiveResult.homeStats.shots,
            "attacking mentality should increase own shot volume");
        require(attackResult.homeStats.expectedGoals > defensiveResult.homeStats.expectedGoals,
            "attacking mentality should increase own xG");
        require(attackResult.awayStats.expectedGoals >= defensiveResult.awayStats.expectedGoals,
            "attacking mentality should carry more defensive risk");

        TacticalSetup highTempo;
        highTempo.tempo = TeamTempo::High;
        TacticalSetup lowTempo;
        lowTempo.tempo = TeamTempo::Low;
        const MatchEngineResult highTempoResult = MatchEngine{}.simulate(
            buildInput(MatchSimulationDetail::BackgroundSummary, 0x7200ULL, 4, 0, highTempo));
        const MatchEngineResult lowTempoResult = MatchEngine{}.simulate(
            buildInput(MatchSimulationDetail::BackgroundSummary, 0x7200ULL, 4, 0, lowTempo));
        require(highTempoResult.homeStats.shots > lowTempoResult.homeStats.shots,
            "high tempo should increase shot volume");
        require(passAccuracyFor(highTempoResult.homeStats) < passAccuracyFor(lowTempoResult.homeStats),
            "high tempo should reduce pass accuracy compared with low tempo");

        TacticalSetup direct;
        direct.passingDirectness = PassingDirectness::Direct;
        TacticalSetup shortPassing;
        shortPassing.passingDirectness = PassingDirectness::Short;
        const MatchEngineResult directResult = MatchEngine{}.simulate(
            buildInput(MatchSimulationDetail::BackgroundSummary, 0x7300ULL, 4, 0, direct));
        const MatchEngineResult shortResult = MatchEngine{}.simulate(
            buildInput(MatchSimulationDetail::BackgroundSummary, 0x7300ULL, 4, 0, shortPassing));
        require(directResult.homeStats.shots > shortResult.homeStats.shots,
            "direct passing should increase direct shot volume");
        require(shortResult.homeStats.passesAttempted > directResult.homeStats.passesAttempted,
            "short passing should increase pass volume");
        require(passAccuracyFor(shortResult.homeStats) > passAccuracyFor(directResult.homeStats),
            "short passing should increase pass accuracy");
    }

    void runFastSummaryQualitySensitivitySmoke() {
        int betterXgSamples = 0;
        int betterShotSamples = 0;
        int betterControlSamples = 0;
        for (std::uint64_t seed = 1; seed <= 16; ++seed) {
            const MatchEngineInput input =
                buildInput(
                    MatchSimulationDetail::BackgroundSummary,
                    0x8100ULL + seed,
                    16,
                    -6);
            const MatchEngineResult result = MatchEngine{}.simulate(input);
            assertResultReportConsistency(input, result);
            if (result.homeStats.expectedGoals > result.awayStats.expectedGoals) {
                ++betterXgSamples;
            }
            if (result.homeStats.shots >= result.awayStats.shots) {
                ++betterShotSamples;
            }
            if (result.homeStats.possessionShare >= result.awayStats.possessionShare
                || result.homeStats.expectedGoals > result.awayStats.expectedGoals + 0.3) {
                ++betterControlSamples;
            }
        }

        require(betterXgSamples >= 12,
            "stronger team should usually produce more xG in fast summary");
        require(betterShotSamples >= 11,
            "stronger team should usually produce at least as many shots in fast summary");
        require(betterControlSamples >= 11,
            "stronger team should usually show better possession or chance control");
    }

    void runPassOptionEvaluatorSmoke() {
        TacticalSetup defensive;
        defensive.mentality = TeamMentality::Defensive;
        defensive.passingDirectness = PassingDirectness::Short;
        defensive.tempo = TeamTempo::Low;
        const PassDecisionFixture defenderFixture = buildPassDecisionFixture(
            FormationSlotRole::CenterBack,
            passDecisionAttributes(68, 64, 66),
            defensive);
        const std::vector<PassOption> defenderOptions = evaluatePassFixture(defenderFixture);
        require(!defenderOptions.empty(), "pass evaluator should create options for a center back");
        const double defenderSafe =
            std::max(bestScoreForKind(defenderOptions, PassOptionKind::SafePass),
                bestScoreForKind(defenderOptions, PassOptionKind::BackPass));
        const double defenderProgressive =
            std::max(bestScoreForKind(defenderOptions, PassOptionKind::ProgressivePass),
                bestScoreForKind(defenderOptions, PassOptionKind::ThroughBall));
        require(defenderSafe > defenderProgressive,
            "defensive center back should prefer safe/support pass options");
        require(defenderProgressive > 0.0,
            "defensive center back should still retain a progressive pass chance");

        const PassDecisionFixture goodMidfielder = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            passDecisionAttributes(82, 84, 80));
        const PassDecisionFixture poorMidfielder = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            passDecisionAttributes(28, 26, 30));
        const std::vector<PassOption> goodOptions = evaluatePassFixture(goodMidfielder);
        const std::vector<PassOption> poorOptions = evaluatePassFixture(poorMidfielder);
        require(countKind(goodOptions, PassOptionKind::ProgressivePass)
                >= countKind(poorOptions, PassOptionKind::ProgressivePass),
            "good passer should identify at least as many progressive options as poor passer");
        require(bestScoreForKind(goodOptions, PassOptionKind::ProgressivePass)
                > bestScoreForKind(poorOptions, PassOptionKind::ProgressivePass),
            "good passer should score progressive options above poor passer");

        TacticalSetup shortLow;
        shortLow.passingDirectness = PassingDirectness::Short;
        shortLow.tempo = TeamTempo::Low;
        TacticalSetup directHigh;
        directHigh.passingDirectness = PassingDirectness::Direct;
        directHigh.tempo = TeamTempo::High;
        const std::vector<PassOption> shortLowOptions = evaluatePassFixture(buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            passDecisionAttributes(74, 76, 74),
            shortLow));
        const std::vector<PassOption> directHighOptions = evaluatePassFixture(buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            passDecisionAttributes(74, 76, 74),
            directHigh));
        const double shortSafe =
            bestScoreForKind(shortLowOptions, PassOptionKind::SafePass)
            + bestScoreForKind(shortLowOptions, PassOptionKind::BackPass);
        const double directSafe =
            bestScoreForKind(directHighOptions, PassOptionKind::SafePass)
            + bestScoreForKind(directHighOptions, PassOptionKind::BackPass);
        const double shortProgressive =
            bestScoreForKind(shortLowOptions, PassOptionKind::ProgressivePass)
            + bestScoreForKind(shortLowOptions, PassOptionKind::ThroughBall)
            + bestScoreForKind(shortLowOptions, PassOptionKind::SwitchPlay);
        const double directProgressive =
            bestScoreForKind(directHighOptions, PassOptionKind::ProgressivePass)
            + bestScoreForKind(directHighOptions, PassOptionKind::ThroughBall)
            + bestScoreForKind(directHighOptions, PassOptionKind::SwitchPlay);
        require(shortSafe > directSafe,
            "short low-tempo tactics should increase safe/support pass preference");
        require(directProgressive > shortProgressive,
            "direct high-tempo tactics should increase progressive/through/switch preference");

        TacticalSetup attackingWide;
        attackingWide.mentality = TeamMentality::Attacking;
        attackingWide.width = TeamWidth::Wide;
        TacticalSetup defensiveWide = attackingWide;
        defensiveWide.mentality = TeamMentality::Defensive;
        defensiveWide.passingDirectness = PassingDirectness::Short;
        PassDecisionFixture attackingFinalThird = buildPassDecisionFixture(
            FormationSlotRole::RightWinger,
            passDecisionAttributes(76, 74, 72),
            attackingWide,
            PitchPoint{ 82.0, 8.0 });
        PassDecisionFixture defensiveFinalThird = buildPassDecisionFixture(
            FormationSlotRole::RightWinger,
            passDecisionAttributes(76, 74, 72),
            defensiveWide,
            PitchPoint{ 82.0, 8.0 });
        addPassDecisionPlayer(
            attackingFinalThird,
            6,
            PitchPoint{ 94.0, 34.0 },
            FormationSlotRole::Striker,
            passDecisionAttributes(66, 60, 62));
        addPassDecisionPlayer(
            attackingFinalThird,
            7,
            PitchPoint{ 76.0, 35.0 },
            FormationSlotRole::AttackingMidfielder,
            passDecisionAttributes(70, 72, 70));
        addPassDecisionPlayer(
            defensiveFinalThird,
            6,
            PitchPoint{ 94.0, 34.0 },
            FormationSlotRole::Striker,
            passDecisionAttributes(66, 60, 62));
        addPassDecisionPlayer(
            defensiveFinalThird,
            7,
            PitchPoint{ 76.0, 35.0 },
            FormationSlotRole::AttackingMidfielder,
            passDecisionAttributes(70, 72, 70));
        const double attackingWideRisk =
            bestScoreForKind(evaluatePassFixture(attackingFinalThird), PassOptionKind::Cross)
            + bestScoreForKind(evaluatePassFixture(attackingFinalThird), PassOptionKind::Cutback);
        const double defensiveWideRisk =
            bestScoreForKind(evaluatePassFixture(defensiveFinalThird), PassOptionKind::Cross)
            + bestScoreForKind(evaluatePassFixture(defensiveFinalThird), PassOptionKind::Cutback);
        require(attackingWideRisk > defensiveWideRisk,
            "attacking wide tactics should increase cross/cutback desire");
    }

    void runPassLaneRiskSmoke() {
        PassDecisionFixture openLane = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            passDecisionAttributes(78, 80, 76));
        PassDecisionFixture blockedLane = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            passDecisionAttributes(78, 80, 76));
        addPassDecisionOpponent(blockedLane, 199, PitchPoint{ 50.0, 34.0 });

        const std::vector<PassOption> openOptions = evaluatePassFixture(openLane);
        const std::vector<PassOption> blockedOptions = evaluatePassFixture(blockedLane);
        const PassOption* openProgressive =
            findOptionForTarget(openOptions, PassOptionKind::ProgressivePass, 3);
        const PassOption* blockedProgressive =
            findOptionForTarget(blockedOptions, PassOptionKind::ProgressivePass, 3);
        require(openProgressive != nullptr && blockedProgressive != nullptr,
            "lane risk smoke should have a comparable progressive pass");
        require(blockedProgressive->laneRisk > openProgressive->laneRisk,
            "blocked lane should report higher lane risk");
        require(blockedProgressive->score < openProgressive->score,
            "blocked lane should score lower than open lane");
        require(bestScoreForKind(blockedOptions, PassOptionKind::SafePass) > 0.0,
            "safe short support option should remain viable when one lane is blocked");
    }

    void runCarryOptionEvaluatorSmoke() {
        TacticalSetup defensive;
        defensive.mentality = TeamMentality::Defensive;
        defensive.tempo = TeamTempo::Low;
        defensive.passingDirectness = PassingDirectness::Short;
        const PassDecisionFixture centerBack = buildPassDecisionFixture(
            FormationSlotRole::CenterBack,
            carryDecisionAttributes(58, 60, 66),
            defensive,
            PitchPoint{ 32.0, 34.0 });
        const std::vector<CarryOption> centerBackOptions = evaluateCarryFixture(centerBack, 5.0);
        require(bestScoreForKind(centerBackOptions, CarryOptionKind::SafeCarry)
                > bestScoreForKind(centerBackOptions, CarryOptionKind::Dribble),
            "center back should prefer safe carry over aggressive dribble");
        require(bestScoreForKind(centerBackOptions, CarryOptionKind::ProgressiveCarry) > 0.0,
            "center back should still have a low-probability progressive carry when space exists");

        TacticalSetup wideAttack;
        wideAttack.mentality = TeamMentality::Attacking;
        wideAttack.tempo = TeamTempo::High;
        wideAttack.width = TeamWidth::Wide;
        const PassDecisionFixture wideCenterBack = buildPassDecisionFixture(
            FormationSlotRole::CenterBack,
            carryDecisionAttributes(68, 68, 68),
            wideAttack,
            PitchPoint{ 40.0, 8.0 });
        const PassDecisionFixture fullback = buildPassDecisionFixture(
            FormationSlotRole::RightBack,
            carryDecisionAttributes(68, 68, 68),
            wideAttack,
            PitchPoint{ 40.0, 8.0 });
        const PassDecisionFixture winger = buildPassDecisionFixture(
            FormationSlotRole::RightWinger,
            carryDecisionAttributes(68, 68, 68),
            wideAttack,
            PitchPoint{ 40.0, 8.0 });
        require(bestScoreForKind(evaluateCarryFixture(fullback), CarryOptionKind::ProgressiveCarry)
                > bestScoreForKind(evaluateCarryFixture(wideCenterBack), CarryOptionKind::ProgressiveCarry),
            "fullback should value wide progressive carry above center back");
        require(bestScoreForKind(evaluateCarryFixture(winger), CarryOptionKind::ProgressiveCarry)
                > bestScoreForKind(evaluateCarryFixture(wideCenterBack), CarryOptionKind::ProgressiveCarry),
            "winger should value wide progressive carry above center back");

        const PassDecisionFixture goodDribbler = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            carryDecisionAttributes(86, 84, 80),
            TacticalSetup{},
            PitchPoint{ 45.0, 34.0 });
        const PassDecisionFixture poorDribbler = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            carryDecisionAttributes(28, 32, 34),
            TacticalSetup{},
            PitchPoint{ 45.0, 34.0 });
        const double goodDribbleScore =
            bestScoreForKind(evaluateCarryFixture(goodDribbler), CarryOptionKind::Dribble);
        const double poorDribbleScore =
            bestScoreForKind(evaluateCarryFixture(poorDribbler), CarryOptionKind::Dribble);
        require(goodDribbleScore > poorDribbleScore,
            "good dribbler should score dribble options above poor dribbler");

        TacticalSetup directHigh;
        directHigh.mentality = TeamMentality::Attacking;
        directHigh.tempo = TeamTempo::High;
        directHigh.passingDirectness = PassingDirectness::Direct;
        TacticalSetup shortLow;
        shortLow.mentality = TeamMentality::Defensive;
        shortLow.tempo = TeamTempo::Low;
        shortLow.passingDirectness = PassingDirectness::Short;
        const PassDecisionFixture aggressiveMid = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            carryDecisionAttributes(74, 74, 72),
            directHigh,
            PitchPoint{ 45.0, 34.0 });
        const PassDecisionFixture cautiousMid = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            carryDecisionAttributes(74, 74, 72),
            shortLow,
            PitchPoint{ 45.0, 34.0 });
        require(bestScoreForKind(evaluateCarryFixture(aggressiveMid), CarryOptionKind::ProgressiveCarry)
                > bestScoreForKind(evaluateCarryFixture(cautiousMid), CarryOptionKind::ProgressiveCarry),
            "attacking direct high-tempo tactics should increase progressive carry desire");
        require(bestScoreForKind(evaluateCarryFixture(cautiousMid), CarryOptionKind::SafeCarry) > 0.0,
            "defensive low-tempo tactics should not make carry impossible");

        const PassDecisionFixture advancedDefender = buildPassDecisionFixture(
            FormationSlotRole::CenterBack,
            carryDecisionAttributes(70, 70, 70),
            directHigh,
            PitchPoint{ 63.0, 34.0 });
        const PassDecisionFixture advancedWinger = buildPassDecisionFixture(
            FormationSlotRole::LeftWinger,
            carryDecisionAttributes(70, 70, 70),
            directHigh,
            PitchPoint{ 63.0, 34.0 });
        const std::vector<CarryOption> defenderAdvancedOptions =
            evaluateCarryFixture(advancedDefender);
        const std::vector<CarryOption> wingerAdvancedOptions =
            evaluateCarryFixture(advancedWinger);
        require(bestZoneRiskForKind(defenderAdvancedOptions, CarryOptionKind::ProgressiveCarry)
                > bestZoneRiskForKind(wingerAdvancedOptions, CarryOptionKind::ProgressiveCarry),
            "defender advanced carry should receive a higher zone-limit risk than winger");
        require(bestScoreForKind(wingerAdvancedOptions, CarryOptionKind::ProgressiveCarry)
                > bestScoreForKind(defenderAdvancedOptions, CarryOptionKind::ProgressiveCarry),
            "winger advanced carry should be less penalized than defender advanced carry");
    }

    void runShotDecisionEvaluatorSmoke() {
        const PassDecisionFixture closeStriker = buildPassDecisionFixture(
            FormationSlotRole::Striker,
            shotDecisionAttributes(82, 78, 80),
            TacticalSetup{},
            PitchPoint{ 97.0, PitchGeometry::WidthMeters / 2.0 });
        const PassDecisionFixture farWideStriker = buildPassDecisionFixture(
            FormationSlotRole::Striker,
            shotDecisionAttributes(82, 78, 80),
            TacticalSetup{},
            PitchPoint{ 70.0, 6.0 });
        require(bestShotScore(evaluateShotFixture(closeStriker))
                > bestShotScore(evaluateShotFixture(farWideStriker)),
            "close central high-xG shot should score above far wide low-xG shot");

        const PassDecisionFixture attackingMid = buildPassDecisionFixture(
            FormationSlotRole::AttackingMidfielder,
            shotDecisionAttributes(76, 74, 76),
            TacticalSetup{},
            PitchPoint{ 99.0, 34.0 });
        const PassDecisionFixture centerBack = buildPassDecisionFixture(
            FormationSlotRole::CenterBack,
            shotDecisionAttributes(76, 74, 76),
            TacticalSetup{},
            PitchPoint{ 99.0, 34.0 });
        const double attackingMidShotScore = bestShotScore(evaluateShotFixture(attackingMid));
        const double centerBackShotScore = bestShotScore(evaluateShotFixture(centerBack));
        require(attackingMidShotScore > 0.0,
            "attacking midfielder should generate a shot option from a reasonable shooting position");
        require(centerBackShotScore == 0.0 || attackingMidShotScore > centerBackShotScore,
            "attacking midfielder shot desire should exceed or outlive center back from same position");

        TacticalSetup defensive;
        defensive.mentality = TeamMentality::Defensive;
        defensive.tempo = TeamTempo::Low;
        TacticalSetup attacking;
        attacking.mentality = TeamMentality::Attacking;
        attacking.tempo = TeamTempo::High;
        const PassDecisionFixture weakDefensiveShot = buildPassDecisionFixture(
            FormationSlotRole::LeftWinger,
            shotDecisionAttributes(74, 72, 72),
            defensive,
            PitchPoint{ 100.0, 24.0 });
        const PassDecisionFixture weakAttackingShot = buildPassDecisionFixture(
            FormationSlotRole::LeftWinger,
            shotDecisionAttributes(74, 72, 72),
            attacking,
            PitchPoint{ 100.0, 24.0 });
        const double weakAttackingScore = bestShotScore(evaluateShotFixture(weakAttackingShot));
        const double weakDefensiveScore = bestShotScore(evaluateShotFixture(weakDefensiveShot));
        require(weakAttackingScore > 0.0,
            "attacking mentality should still be able to generate a weak shot option");
        require(weakDefensiveScore == 0.0 || weakAttackingScore > weakDefensiveScore,
            "defensive mentality should lower or suppress weak shot desire");

        const PassDecisionFixture clearDefensiveShot = buildPassDecisionFixture(
            FormationSlotRole::Striker,
            shotDecisionAttributes(80, 78, 80),
            defensive,
            PitchPoint{ 99.0, 34.0 });
        require(bestShotScore(evaluateShotFixture(clearDefensiveShot)) > 0.0,
            "defensive mentality should not eliminate a clear high-xG shot");

        const PassDecisionFixture lowXGCenterBack = buildPassDecisionFixture(
            FormationSlotRole::CenterBack,
            shotDecisionAttributes(82, 82, 82),
            attacking,
            PitchPoint{ 70.0, 6.0 });
        require(evaluateShotFixture(lowXGCenterBack).empty(),
            "low-xG defensive long shots should not become common shot candidates");
    }

    PlayerDecisionContext playerDecisionContextFor(
        const PassDecisionFixture& fixture,
        const MatchSimulationState& state,
        double pressure = 8.0) {
        const double progress = fixture.direction == AttackingDirection::HomeToAway
            ? fixture.carrier.position.x / PitchGeometry::LengthMeters
            : (PitchGeometry::LengthMeters - fixture.carrier.position.x) / PitchGeometry::LengthMeters;
        DecisionMatchPhase phase = DecisionMatchPhase::BuildUp;
        if (progress >= 0.88) {
            phase = DecisionMatchPhase::ChanceCreation;
        } else if (progress >= 0.78) {
            phase = DecisionMatchPhase::BoxEntry;
        } else if (progress >= 0.66) {
            phase = DecisionMatchPhase::FinalThird;
        } else if (progress >= 0.38) {
            phase = DecisionMatchPhase::MiddleThirdCirculation;
        }

        PossessionContext possession;
        possession.teamInPossession = fixture.carrier.teamId;
        possession.ballCarrierId = fixture.carrier.playerId;
        possession.possessionActionCount = state.possession.actionDepth;
        possession.currentPhase = phase;
        possession.ballPosition = fixture.carrier.position;
        possession.ballProgression = std::clamp(progress, 0.0, 1.0);
        possession.progressionAvailable = progress < 0.86;
        possession.safeCirculationAvailable = true;
        possession.finalThirdEntryAvailable = progress >= 0.55 && progress < 0.70;
        possession.boxEntryAvailable = progress >= 0.68 && progress < 0.84;
        possession.localPressure = pressure;

        DefensiveContext defensive;
        defensive.defendingTeamId = fixture.carrier.teamId;
        defensive.opponentTeamId = fixture.opponentState.teamId;
        defensive.opponentPossessionActionCount = state.possession.actionDepth;
        defensive.opponentPhase = phase;
        defensive.ballPosition = fixture.carrier.position;

        return PlayerDecisionContext{
            fixture.carrier.playerId,
            fixture.carrier.teamId,
            fixture.carrierRole,
            fixture.carrier.position,
            fixture.carrier.position,
            fixture.tactics,
            phase,
            possession,
            defensive,
            pressure
        };
    }

    void runBallCarrierDecisionModelSmoke() {
        TacticalSetup tactics;
        tactics.mentality = TeamMentality::Attacking;
        tactics.tempo = TeamTempo::High;
        tactics.passingDirectness = PassingDirectness::Direct;
        const PassDecisionFixture fixture = buildPassDecisionFixture(
            FormationSlotRole::AttackingMidfielder,
            shotDecisionAttributes(80, 78, 78),
            tactics,
            PitchPoint{ 102.0, 34.0 });

        MatchSimulationState state;
        state.homeTeam = fixture.teamState;
        state.awayTeam = fixture.opponentState;
        state.ball.controlState = BallControlState::Controlled;
        state.ball.carrierPlayerId = fixture.carrier.playerId;
        state.ball.carrierTeamId = fixture.carrier.teamId;
        state.ball.position = fixture.carrier.position;
        state.possession.teamInPossession = fixture.carrier.teamId;
        state.possession.ballCarrierId = fixture.carrier.playerId;

        TeamShapeContext shapeContext;
        shapeContext.teamId = fixture.teamState.teamId;
        shapeContext.isHomeTeam = true;
        shapeContext.hasPossession = true;
        shapeContext.phase = TeamShapePhase::InPossession;
        shapeContext.attackingDirection = AttackingDirection::HomeToAway;
        shapeContext.tacticalSetup = tactics;
        shapeContext.ballPosition = fixture.carrier.position;

        (void)shapeContext;
        const std::vector<ActionCandidate> modelCandidates = BallCarrierDecisionModel{}.evaluate(
            BallCarrierDecisionModelContext{
                &fixture.teamSnapshot,
                &fixture.opponentSnapshot,
                &fixture.teamState,
                &fixture.opponentState,
                &fixture.carrier,
                AttackingDirection::HomeToAway,
                playerDecisionContextFor(fixture, state)
            });
        const auto modelHasCandidate = [&modelCandidates](BallCarrierActionType type) {
            return std::any_of(
                modelCandidates.begin(),
                modelCandidates.end(),
                [type](const ActionCandidate& candidate) {
                    return candidate.type == type;
                });
        };
        require(modelHasCandidate(BallCarrierActionType::ShortPass)
                || modelHasCandidate(BallCarrierActionType::ThroughBall)
                || modelHasCandidate(BallCarrierActionType::LowCross)
                || modelHasCandidate(BallCarrierActionType::Cutback),
            "BallCarrierDecisionModel should combine pass evaluator options");
        require(modelHasCandidate(BallCarrierActionType::Carry)
                || modelHasCandidate(BallCarrierActionType::Dribble),
            "BallCarrierDecisionModel should combine carry evaluator options");
        require(modelHasCandidate(BallCarrierActionType::Shoot),
            "BallCarrierDecisionModel should combine shot evaluator options");
    }

    void runPassSelectionDeterminismSmoke() {
        const PassDecisionFixture fixture = buildPassDecisionFixture(
            FormationSlotRole::CentralMidfielder,
            passDecisionAttributes(76, 78, 74));
        MatchSimulationState state;
        state.homeTeam = fixture.teamState;
        state.awayTeam = fixture.opponentState;
        state.ball.controlState = BallControlState::Controlled;
        state.ball.carrierPlayerId = fixture.carrier.playerId;
        state.ball.carrierTeamId = fixture.carrier.teamId;
        state.ball.position = fixture.carrier.position;
        state.possession.teamInPossession = fixture.carrier.teamId;
        state.possession.ballCarrierId = fixture.carrier.playerId;

        const BallCarrierDecisionModel model;
        const std::vector<ActionCandidate> firstCandidates = model.evaluate(
            BallCarrierDecisionModelContext{
                &fixture.teamSnapshot,
                &fixture.opponentSnapshot,
                &fixture.teamState,
                &fixture.opponentState,
                &fixture.carrier,
                fixture.direction,
                playerDecisionContextFor(fixture, state)
            });
        const std::vector<ActionCandidate> secondCandidates = model.evaluate(
            BallCarrierDecisionModelContext{
                &fixture.teamSnapshot,
                &fixture.opponentSnapshot,
                &fixture.teamState,
                &fixture.opponentState,
                &fixture.carrier,
                fixture.direction,
                playerDecisionContextFor(fixture, state)
            });
        const ActionSelector selector;
        const ActionSelectionResult first = selector.select(ActionSelectionRequest{
            firstCandidates,
            passDecisionAttributes(76, 78, 74),
            0x55aa11ULL
        });
        const ActionSelectionResult second = selector.select(ActionSelectionRequest{
            secondCandidates,
            passDecisionAttributes(76, 78, 74),
            0x55aa11ULL
        });
        require(first.selected.has_value() && second.selected.has_value(),
            "pass selection determinism smoke should select an action");
        require(first.selected->type == second.selected->type
                && first.selected->targetPlayerId == second.selected->targetPlayerId,
            "same pass candidates and seed should select the same behavior");
    }

    void runDetailedCoordinateBalanceSmoke() {
        int assistedGoalSamples = 0;
        int goalScorerRatingSamples = 0;
        int totalShots = 0;
        int totalPasses = 0;
        int totalCarryTraces = 0;
        int totalShotTraces = 0;
        int hundredShotSamples = 0;
        int explodedXGSamples = 0;
        for (std::uint64_t seed = 1; seed <= 8; ++seed) {
            const MatchEngineInput input =
                buildInput(MatchSimulationDetail::WatchedMatch, 0x9300ULL + seed);
            const MatchEngineResult result = MatchEngine{}.simulate(input);
            assertResultReportConsistency(input, result);

            const double maxPossession = std::max(
                result.homeStats.possessionShare,
                result.awayStats.possessionShare);
            const int combinedPasses =
                result.homeStats.passesAttempted + result.awayStats.passesAttempted;
            const int combinedShots = result.homeStats.shots + result.awayStats.shots;
            const double combinedXG =
                result.homeStats.expectedGoals + result.awayStats.expectedGoals;
            if (combinedShots >= 100) {
                ++hundredShotSamples;
                std::cerr << "shot balance guardrail seed=" << (0x9300ULL + seed)
                    << " combinedShots=" << combinedShots
                    << " homeShots=" << result.homeStats.shots
                    << " awayShots=" << result.awayStats.shots
                    << " homeSOT=" << result.homeStats.shotsOnTarget
                    << " awaySOT=" << result.awayStats.shotsOnTarget
                    << " combinedXG=" << combinedXG
                    << " homeXG=" << result.homeStats.expectedGoals
                    << " awayXG=" << result.awayStats.expectedGoals
                    << " shotTraces=" << traceCountFor(result, MatchTraceKind::Shot)
                    << " passAttempts=" << combinedPasses
                    << '\n';
            }
            if (combinedXG >= 30.0) {
                ++explodedXGSamples;
            }
            const bool extremeLoop =
                maxPossession > 95.0
                || result.homeStats.passesAttempted > 2000
                || result.awayStats.passesAttempted > 2000
                || (result.homeStats.passesAttempted > 1500 && result.awayStats.passesAttempted < 100)
                || (result.awayStats.passesAttempted > 1500 && result.homeStats.passesAttempted < 100)
                || (maxPossession > 90.0
                    && std::min(result.homeStats.passesAttempted, result.awayStats.passesAttempted) < 100)
                || (maxPossession > 90.0 && combinedPasses > 1500 && combinedShots == 0);
            if (extremeLoop) {
                std::cerr << "anti-loop guardrail seed=" << (0x9300ULL + seed)
                    << " maxPossession=" << maxPossession
                    << " combinedPasses=" << combinedPasses
                    << " combinedShots=" << combinedShots
                    << " homePossession=" << result.homeStats.possessionShare
                    << " awayPossession=" << result.awayStats.possessionShare
                    << " homePasses=" << result.homeStats.passesCompleted
                    << "/" << result.homeStats.passesAttempted
                    << " awayPasses=" << result.awayStats.passesCompleted
                    << "/" << result.awayStats.passesAttempted
                    << " homeShots=" << result.homeStats.shots
                    << " awayShots=" << result.awayStats.shots
                    << " homeSOT=" << result.homeStats.shotsOnTarget
                    << " awaySOT=" << result.awayStats.shotsOnTarget
                    << " homeXg=" << result.homeStats.expectedGoals
                    << " awayXg=" << result.awayStats.expectedGoals
                    << " interceptions="
                    << (result.homeStats.interceptions + result.awayStats.interceptions)
                    << " turnovers=" << traceCountFor(result, MatchTraceKind::Turnover)
                    << " loosePasses="
                    << (result.homeStats.passesLoose + result.awayStats.passesLoose)
                    << " deflectedPasses="
                    << (result.homeStats.passesDeflected + result.awayStats.passesDeflected)
                    << " receiverOutOfRange="
                    << (result.homeStats.passesReceiverOutOfRange
                        + result.awayStats.passesReceiverOutOfRange)
                    << '\n';
            }
            require(!extremeLoop,
                "detailed coordinate match should not fall into extreme short-pass possession loop");

            totalCarryTraces += traceCountFor(result, MatchTraceKind::Carry);
            totalShotTraces += traceCountFor(result, MatchTraceKind::Shot);

            for (const MatchTeamSimulationStats* stats : { &result.homeStats, &result.awayStats }) {
                require(stats->passesAttempted > 0, "detailed coordinate match should attempt passes");
                totalShots += stats->shots;
                totalPasses += stats->passesAttempted;
                require(stats->fouls == 0 && stats->corners == 0,
                    "detailed coordinate fouls/corners should remain placeholders until modeled");
            }

            const int homeGoals = result.homeStats.goals;
            const int awayGoals = result.awayStats.goals;
            for (const MatchPlayerSimulationStats& stats : result.playerStats) {
                const bool losingTeam =
                    (stats.teamId == result.homeStats.teamId && homeGoals < awayGoals)
                    || (stats.teamId == result.awayStats.teamId && awayGoals < homeGoals);
                if (losingTeam && stats.goals == 0 && stats.assists == 0) {
                    require(stats.rating < 9.8,
                        "losing players without goals or assists should not inflate to 10.0 ratings");
                }
                if (stats.goals > 0 && stats.rating > 6.5) {
                    ++goalScorerRatingSamples;
                }
            }

            assistedGoalSamples += static_cast<int>(std::count_if(
                result.events.begin(),
                result.events.end(),
                [](const MatchEventRecord& event) {
                    return event.kind == MatchEventKind::Goal && event.secondaryPlayerId != 0;
                }));
        }

        require(assistedGoalSamples > 0,
            "detailed coordinate possession-created goals should be able to record assists");
        require(goalScorerRatingSamples > 0,
            "detailed coordinate goal scorers should receive a visible rating lift");
        require(totalShots > 0 && totalShotTraces > 0,
            "detailed coordinate samples should include terminal shot actions");
        require(totalCarryTraces > 0,
            "detailed coordinate samples should include carry actions without suppressing shots");
        require(totalPasses > totalShots,
            "detailed coordinate samples should retain passing without becoming only shots");
        require(hundredShotSamples <= 1,
            "balanced 4-4-2 coordinate matches should not regularly produce 100+ shots");
        require(explodedXGSamples == 0,
            "balanced 4-4-2 coordinate matches should not explode into 30+ total xG");
    }

    void runInvalidInputSmoke() {
        const MatchEngineResult result = MatchEngine{}.simulate(MatchEngineInput{});
        require(!result.report.has_value(), "invalid input should not produce a report");
        require(result.homeStats.goals == 0, "invalid input home goals should default to zero");
        require(result.awayStats.goals == 0, "invalid input away goals should default to zero");
        require(result.traceFrames.empty(), "invalid input trace frames should default empty");
    }

    PlayMatchCommand buildCommand(const League& league, const FixtureMatch& match, const Date& date) {
        return PlayMatchCommand{
            match.matchId,
            league.getId(),
            league.getCurrentSeasonYear(),
            date,
            match.homeId,
            match.awayId,
            match.matchweek
        };
    }

    void runHandlerSmoke(
        MatchSimulationEngineMode engineMode,
        MatchSimulationDetail coordinateDetail = MatchSimulationDetail::BackgroundSummary) {
        const Date matchDate{ 2026, Month::March, 14 };
        League league("Handler Smoke", 909);
        league.addTeam(std::make_unique<Team>(makeTeam(501, "Handler Home", 3)));
        league.addTeam(std::make_unique<Team>(makeTeam(502, "Handler Away", 0)));
        league.resetForNewSeason(2026);
        league.initializeMatchdayTracking(1);
        league.addFixtureMatch(707, 1, matchDate, 501, 502);

        FixtureMatch* match = league.findFixtureMatchById(707);
        require(match != nullptr, "handler smoke fixture should exist");
        match->eventEnqueued = true;

        int publishedEvents = 0;
        MatchId publishedMatchId = 0;
        DomainEventPublisher publisher;
        publisher.subscribeMatchPlayed([&](const MatchPlayedEvent& event) {
            ++publishedEvents;
            publishedMatchId = event.matchId;
        });

        PlayMatchCommandHandlerOptions options;
        options.engineMode = engineMode;
        options.coordinateDetail = coordinateDetail;
        PlayMatchCommandHandler handler(publisher, options);
        handler.handle(league, buildCommand(league, *match, matchDate));

        const FixtureMatch* playedMatch = league.findFixtureMatchById(707);
        require(playedMatch != nullptr && playedMatch->played, "handler should mark fixture played");
        require(!playedMatch->eventEnqueued, "handler should clear fixture event enqueue guard");
        require(playedMatch->homeGoals >= 0 && playedMatch->awayGoals >= 0, "handler should apply non-negative goals");

        const auto& standings = league.getStandings();
        require(standings.at(501).played == 1, "home standings should update through League");
        require(standings.at(502).played == 1, "away standings should update through League");
        require(league.findCurrentSeasonMatchReportById(707) != nullptr, "applied match report should be stored");
        require(publishedEvents == 1, "handler should publish one MatchPlayedEvent");
        require(publishedMatchId == 707, "published event should reference played match");
    }

    void runHandlerIntegrationSmoke() {
        PlayMatchCommandHandlerOptions defaultOptions;
        require(defaultOptions.engineMode == MatchSimulationEngineMode::Coordinate,
            "default handler engine mode should be coordinate");
        require(defaultOptions.coordinateDetail == MatchSimulationDetail::BackgroundSummary,
            "default coordinate handler detail should remain scalable background summary");
        runHandlerSmoke(MatchSimulationEngineMode::Coordinate);
        runHandlerSmoke(MatchSimulationEngineMode::Coordinate, MatchSimulationDetail::WatchedMatch);
        runHandlerSmoke(MatchSimulationEngineMode::Lightweight);
    }

    void runPlayerAttributesPersistenceSmoke() {
        const std::filesystem::path dbPath =
            std::filesystem::temp_directory_path() / "fm_match_engine_attributes_smoke.db";
        std::error_code removeError;
        std::filesystem::remove(dbPath, removeError);

        PlayerAttributes attributes =
            buildAttributesFromLegacySkills(PlayerPosition::CentralMidfielder, 71, 68, 74, 69, 73, 24);
        attributes.technical.setPieces = 77;
        attributes.goalkeeper.distribution = 12;

        {
            SqliteGameStateRepository repository(dbPath.string());
            repository.saveRuntimeState(
                Date{ 2026, Month::March, 14 },
                0,
                {},
                {},
                {},
                {},
                {},
                { PersistedPlayerAttributesState{ 12345, attributes } },
                {},
                {},
                {},
                {});
        }

        SqliteGameStateRepository repository(dbPath.string(), GameStateRepositoryMode::ReadExisting);
        const std::vector<PersistedPlayerAttributesState> loadedAttributes =
            repository.loadPlayerAttributesStates();
        require(loadedAttributes.size() == 1, "runtime save should persist one player attribute row");
        require(loadedAttributes.front().playerId == 12345, "loaded player attribute row should preserve player id");
        require(sameAttributes(loadedAttributes.front().attributes, attributes),
            "loaded player attributes should match saved detailed attributes");

        std::filesystem::remove(dbPath, removeError);
    }

    void runMatchReportStatsPersistenceSmoke() {
        const std::filesystem::path dbPath =
            std::filesystem::temp_directory_path() / "fm_match_engine_report_stats_smoke.db";
        std::error_code removeError;
        std::filesystem::remove(dbPath, removeError);

        MatchReport report;
        report.matchId = 3030;
        report.leagueId = 4040;
        report.seasonYear = 2026;
        report.date = Date{ 2026, Month::March, 14 };
        report.homeId = 7001;
        report.awayId = 7002;
        report.matchweek = 3;
        report.homeGoals = 2;
        report.awayGoals = 1;
        report.homeLineup.teamId = report.homeId;
        report.awayLineup.teamId = report.awayId;
        report.homeStats.goals = report.homeGoals;
        report.homeStats.shots = 11;
        report.homeStats.shotsOnTarget = 5;
        report.homeStats.passesAttempted = 420;
        report.homeStats.passesCompleted = 338;
        report.homeStats.possessionShare = 56.0;
        report.homeStats.expectedGoals = 1.42;
        report.awayStats.goals = report.awayGoals;
        report.awayStats.shots = 7;
        report.awayStats.shotsOnTarget = 2;
        report.awayStats.passesAttempted = 310;
        report.awayStats.passesCompleted = 231;
        report.awayStats.possessionShare = 44.0;
        report.awayStats.expectedGoals = 0.81;
        report.playerReports.push_back(MatchPlayerReport{
            8001,
            report.homeId,
            true,
            90,
            1,
            1,
            0,
            0,
            8.2
        });

        {
            SqliteGameStateRepository repository(dbPath.string());
            repository.saveRuntimeState(
                Date{ 2026, Month::March, 14 },
                0,
                {},
                {},
                { report },
                {},
                {},
                {},
                {},
                {},
                {},
                {});
        }

        SqliteGameStateRepository repository(dbPath.string(), GameStateRepositoryMode::ReadExisting);
        const std::vector<MatchReport> loadedReports = repository.loadMatchReports();
        require(loadedReports.size() == 1, "runtime save should persist one match report");
        require(loadedReports.front().homeStats.shots == 11,
            "loaded match report should preserve home team shots");
        require(loadedReports.front().awayStats.passesCompleted == 231,
            "loaded match report should preserve away team passes");
        require(loadedReports.front().homeStats.goals == loadedReports.front().homeGoals,
            "loaded home report stats should match score");
        require(loadedReports.front().awayStats.goals == loadedReports.front().awayGoals,
            "loaded away report stats should match score");
        require(!loadedReports.front().playerReports.empty()
            && loadedReports.front().playerReports.front().rating == 8.2,
            "loaded match player report should preserve rating");

        std::filesystem::remove(dbPath, removeError);
    }
}

int main() {
    try {
        runDeterministicRegression();
        runInvalidInputSmoke();
        runShotQualityModelSmoke();
        runShotExecutionModelSmoke();
        runShotQualityFoundationSmoke();
        runShotOutcomeFoundationSmoke();
        runDetailSmoke(MatchSimulationDetail::BackgroundSummary);
        runDetailSmoke(MatchSimulationDetail::WatchedMatch);
        runDetailSmoke(MatchSimulationDetail::DebugFullTrace);
        runOfficialGoalEventSmoke();
        runBackgroundSummaryCalibrationSmoke();
        runGoalkeeperScorerSafetySmoke();
        runFastSummaryTacticalSensitivitySmoke();
        runFastSummaryQualitySensitivitySmoke();
        runPassOptionEvaluatorSmoke();
        runPassLaneRiskSmoke();
        runCarryOptionEvaluatorSmoke();
        runShotDecisionEvaluatorSmoke();
        runBallCarrierDecisionModelSmoke();
        runPassSelectionDeterminismSmoke();
        runDetailedCoordinateBalanceSmoke();
        runHandlerIntegrationSmoke();
        runPlayerAttributesPersistenceSmoke();
        runMatchReportStatsPersistenceSmoke();
    }
    catch (const std::exception& ex) {
        std::cerr << "fm_match_engine_smoke failed: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "fm_match_engine_smoke passed\n";
    return 0;
}
