#include"fm/match_engine/MatchEngine.h"
#include"fm/match_engine/MatchEngineInputBuilder.h"
#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/ball/ShotQualityModel.h"
#include"fm/match_engine/decision/ActionSelector.h"
#include"fm/match_engine/decision/BallCarrierDecisionModel.h"
#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"
#include"fm/match_engine/ball/ReboundTrajectoryBuilder.h"
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
#include<cmath>
#include<cstdint>
#include<filesystem>
#include<iostream>
#include<memory>
#include<numeric>
#include<stdexcept>
#include<string>
#include<unordered_map>
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
            "Fixture Player " + std::to_string(playerId),
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
            "Fixture Opponent " + std::to_string(playerId),
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

    struct GoalSourceDiagnostic {
        int assistedGoals = 0;
        int unassistedOpenPlayGoals = 0;
        int reboundGoals = 0;
        int transitionGoals = 0;
    };

    struct DefensiveEventDiagnostic {
        int pressures = 0;
        int duels = 0;
        int tackleAttempts = 0;
        int tacklesWon = 0;
        int tacklesLost = 0;
        int passInterceptions = 0;
        int passBlocks = 0;
        int shotBlocks = 0;
        int clearances = 0;
        int looseBallRecoveries = 0;
        int keeperClaims = 0;
        int dispossessionsForced = 0;
        int dribblesAttempted = 0;
        int dribblesWon = 0;
        int dribblesLost = 0;
    };

    struct ShotOutcomeDiagnostic {
        int offTarget = 0;
        int savedHeld = 0;
        int savedParried = 0;
        int woodwork = 0;
        int blockedShots = 0;
        int reboundShots = 0;
        double totalShotDistance = 0.0;
        int distanceSamples = 0;
        int shotsInsideFourMeters = 0;
        int shotsFourToEightMeters = 0;
        int shotsEightToTwelveMeters = 0;
        int shotsTwelveToEighteenMeters = 0;
        int shotsBeyondEighteenMeters = 0;
        int preShotUnder003 = 0;
        int preShot003To008 = 0;
        int preShot008To015 = 0;
        int preShot015To030 = 0;
        int preShotAbove030 = 0;
        int carryShots = 0;
        int simplePassShots = 0;
        int finalBallShots = 0;
        int throughBallShots = 0;
        int lowCrossShots = 0;
        int cutbackShots = 0;
        int reboundOrLooseShots = 0;
        int turnoverShots = 0;
        int assistedShots = 0;
        int soloShots = 0;
    };

    enum class RoleBucket {
        Goalkeeper,
        CenterBack,
        FullbackWingback,
        CentralMidfield,
        Wide,
        Striker,
        Unknown
    };

    struct RoleBucketDiagnostic {
        int playerSamples = 0;
        int passesAttempted = 0;
        int passesCompleted = 0;
        int shortPassesAttempted = 0;
        int shortPassesCompleted = 0;
        int throughBalls = 0;
        int lowCrosses = 0;
        int cutbacks = 0;
        int finalBalls = 0;
        int shots = 0;
        int shotsOnTarget = 0;
        int goals = 0;
        int assistedShotsReceived = 0;
        int assists = 0;
        int carryTraces = 0;
        int dribbleTraces = 0;
        int progressiveCarries = 0;
        DefensiveEventDiagnostic defensive;
        int tackles = 0;
        double preShotXG = 0.0;
        double effectiveXG = 0.0;
        double totalDistance = 0.0;
        double onBallDistance = 0.0;
        double offBallDistance = 0.0;
    };

    struct PlayerLeaderDiagnostic {
        PlayerId playerId = 0;
        TeamId teamId = 0;
        int playerSamples = 0;
        std::string playerName;
        std::string teamName;
        RoleBucket roleBucket = RoleBucket::Unknown;
        int passesAttempted = 0;
        int passesCompleted = 0;
        int finalBalls = 0;
        int carryTraces = 0;
        int dribbleTraces = 0;
        int shots = 0;
        DefensiveEventDiagnostic defensive;
        int tackles = 0;
        double expectedGoals = 0.0;
        double totalDistance = 0.0;
    };

    struct TraceMixDiagnostic {
        int carryTraces = 0;
        int dribbleTraces = 0;
        int shotTraces = 0;
    };

    struct SmokeAggregateDiagnostic {
        std::unordered_map<int, RoleBucketDiagnostic> roleBuckets;
        std::unordered_map<std::string, PlayerLeaderDiagnostic> playerLeaders;
    };

    const char* roleBucketName(RoleBucket bucket) {
        switch (bucket) {
        case RoleBucket::Goalkeeper:
            return "GK";
        case RoleBucket::CenterBack:
            return "CB";
        case RoleBucket::FullbackWingback:
            return "FB/WB";
        case RoleBucket::CentralMidfield:
            return "DM/CM/AM";
        case RoleBucket::Wide:
            return "Wide Mid/Winger";
        case RoleBucket::Striker:
            return "ST";
        case RoleBucket::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    RoleBucket bucketForRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            return RoleBucket::Goalkeeper;
        case FormationSlotRole::CenterBack:
            return RoleBucket::CenterBack;
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            return RoleBucket::FullbackWingback;
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::AttackingMidfielder:
            return RoleBucket::CentralMidfield;
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            return RoleBucket::Wide;
        case FormationSlotRole::Striker:
            return RoleBucket::Striker;
        case FormationSlotRole::Unknown:
            return RoleBucket::Unknown;
        }
        return RoleBucket::Unknown;
    }

    const char* actionTypeName(BallCarrierActionType type) {
        switch (type) {
        case BallCarrierActionType::ShortPass:
            return "ShortPass";
        case BallCarrierActionType::BackPass:
            return "BackPass";
        case BallCarrierActionType::ThroughBall:
            return "ThroughBall";
        case BallCarrierActionType::SwitchPlay:
            return "SwitchPlay";
        case BallCarrierActionType::Carry:
            return "Carry";
        case BallCarrierActionType::Dribble:
            return "Dribble";
        case BallCarrierActionType::CutInside:
            return "CutInside";
        case BallCarrierActionType::LowCross:
            return "LowCross";
        case BallCarrierActionType::HighCross:
            return "HighCross";
        case BallCarrierActionType::Cutback:
            return "Cutback";
        case BallCarrierActionType::Shoot:
            return "Shoot";
        case BallCarrierActionType::Hold:
            return "Hold";
        case BallCarrierActionType::Clear:
            return "Clear";
        }
        return "Unknown";
    }

    const char* goalSourceCategoryName(MatchGoalSourceCategory category) {
        switch (category) {
        case MatchGoalSourceCategory::AssistedFinalBall:
            return "AssistedFinalBall";
        case MatchGoalSourceCategory::AssistedSimplePass:
            return "AssistedSimplePass";
        case MatchGoalSourceCategory::CarryAfterReceive:
            return "CarryAfterReceive";
        case MatchGoalSourceCategory::SoloCarry:
            return "SoloCarry";
        case MatchGoalSourceCategory::Rebound:
            return "Rebound";
        case MatchGoalSourceCategory::Turnover:
            return "Turnover";
        case MatchGoalSourceCategory::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    MatchTraceKind previousSignificantTraceKind(
        const MatchEngineResult& result,
        std::size_t index) {
        for (std::size_t previous = index; previous > 0; --previous) {
            const MatchTraceFrame& frame = result.traceFrames[previous - 1];
            if (frame.kind != MatchTraceKind::PossessionStart) {
                return frame.kind;
            }
        }

        return MatchTraceKind::PossessionStart;
    }

    PitchPoint goalCenterForTraceTeam(
        const MatchEngineInput& input,
        TeamId teamId) {
        return teamId == input.homeTeam.teamId
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    TeamId teamIdForPlayerInInput(
        const MatchEngineInput& input,
        PlayerId playerId) {
        for (const MatchPlayerSnapshot& player : input.homeTeam.players) {
            if (player.playerId == playerId) {
                return input.homeTeam.teamId;
            }
        }
        for (const MatchPlayerSnapshot& player : input.awayTeam.players) {
            if (player.playerId == playerId) {
                return input.awayTeam.teamId;
            }
        }
        return 0;
    }

    TeamId shotOutcomeTeamIdForFrame(
        const MatchEngineInput& input,
        const MatchTraceFrame& frame) {
        switch (frame.kind) {
        case MatchTraceKind::Save:
        case MatchTraceKind::SaveHeld:
        case MatchTraceKind::SaveParried:
        case MatchTraceKind::BlockedShot:
            return teamIdForPlayerInInput(input, frame.secondaryPlayerId);
        case MatchTraceKind::ShotOffTarget:
        case MatchTraceKind::Woodwork:
        case MatchTraceKind::Shot:
            return frame.teamId;
        default:
            return frame.teamId;
        }
    }

    AttackingDirection attackingDirectionForTraceTeam(
        const MatchEngineInput& input,
        TeamId teamId) {
        return teamId == input.homeTeam.teamId
            ? AttackingDirection::HomeToAway
            : AttackingDirection::AwayToHome;
    }

    ShotOutcomeDiagnostic shotOutcomeDiagnosticFor(
        const MatchEngineInput& input,
        const MatchEngineResult& result,
        TeamId teamFilter = 0) {
        ShotOutcomeDiagnostic diagnostic;

        for (std::size_t i = 0; i < result.traceFrames.size(); ++i) {
            const MatchTraceFrame& frame = result.traceFrames[i];

            switch (frame.kind) {
            case MatchTraceKind::ShotOffTarget:
                if (teamFilter != 0 && shotOutcomeTeamIdForFrame(input, frame) != teamFilter) {
                    break;
                }
                ++diagnostic.offTarget;
                break;
            case MatchTraceKind::Save:
            case MatchTraceKind::SaveHeld:
                if (teamFilter != 0 && shotOutcomeTeamIdForFrame(input, frame) != teamFilter) {
                    break;
                }
                ++diagnostic.savedHeld;
                break;
            case MatchTraceKind::SaveParried:
                if (teamFilter != 0 && shotOutcomeTeamIdForFrame(input, frame) != teamFilter) {
                    break;
                }
                ++diagnostic.savedParried;
                break;
            case MatchTraceKind::Woodwork:
                if (teamFilter != 0 && shotOutcomeTeamIdForFrame(input, frame) != teamFilter) {
                    break;
                }
                ++diagnostic.woodwork;
                break;
            case MatchTraceKind::BlockedShot:
                if (teamFilter != 0 && shotOutcomeTeamIdForFrame(input, frame) != teamFilter) {
                    break;
                }
                ++diagnostic.blockedShots;
                break;
            default:
                break;
            }

            if (frame.kind != MatchTraceKind::Shot) {
                continue;
            }
            if (teamFilter != 0 && frame.teamId != teamFilter) {
                continue;
            }

            const double shotDistance =
                PitchGeometry::distance(frame.ballPosition, goalCenterForTraceTeam(input, frame.teamId));
            diagnostic.totalShotDistance += shotDistance;
            if (shotDistance <= 4.0) {
                ++diagnostic.shotsInsideFourMeters;
            } else if (shotDistance <= 8.0) {
                ++diagnostic.shotsFourToEightMeters;
            } else if (shotDistance <= 12.0) {
                ++diagnostic.shotsEightToTwelveMeters;
            } else if (shotDistance <= 18.0) {
                ++diagnostic.shotsTwelveToEighteenMeters;
            } else {
                ++diagnostic.shotsBeyondEighteenMeters;
            }
            ++diagnostic.distanceSamples;

            const double approximatePreShotXG = ShotQualityModel::calculateOpenPlayXG(
                frame.ballPosition,
                attackingDirectionForTraceTeam(input, frame.teamId),
                0.0);
            if (approximatePreShotXG < 0.03) {
                ++diagnostic.preShotUnder003;
            } else if (approximatePreShotXG < 0.08) {
                ++diagnostic.preShot003To008;
            } else if (approximatePreShotXG < 0.15) {
                ++diagnostic.preShot008To015;
            } else if (approximatePreShotXG < 0.30) {
                ++diagnostic.preShot015To030;
            } else {
                ++diagnostic.preShotAbove030;
            }

            const MatchTraceKind previousKind = previousSignificantTraceKind(result, i);
            if (previousKind == MatchTraceKind::SaveParried
                || previousKind == MatchTraceKind::LooseBall
                || previousKind == MatchTraceKind::BlockedShot) {
                ++diagnostic.reboundShots;
            }
        }

        return diagnostic;
    }

    GoalSourceDiagnostic goalSourceDiagnosticFor(const MatchEngineResult& result) {
        GoalSourceDiagnostic diagnostic;
        diagnostic.assistedGoals = result.homeStats.assistedGoals + result.awayStats.assistedGoals;
        diagnostic.unassistedOpenPlayGoals =
            result.homeStats.unassistedOpenPlayGoals + result.awayStats.unassistedOpenPlayGoals;
        diagnostic.reboundGoals = result.homeStats.reboundGoals + result.awayStats.reboundGoals;
        diagnostic.transitionGoals = result.homeStats.transitionGoals + result.awayStats.transitionGoals;
        return diagnostic;
    }

    DefensiveEventDiagnostic defensiveEventDiagnosticFor(
        const MatchEngineResult& result,
        TeamId teamFilter = 0) {
        DefensiveEventDiagnostic diagnostic;
        for (const MatchTeamSimulationStats* stats : { &result.homeStats, &result.awayStats }) {
            if (teamFilter != 0 && stats->teamId != teamFilter) {
                continue;
            }
            diagnostic.pressures += stats->pressures;
            diagnostic.duels += stats->duels;
            diagnostic.tackleAttempts += stats->tacklesAttempted;
            diagnostic.tacklesWon += stats->tacklesWon;
            diagnostic.tacklesLost += stats->tacklesLost;
            diagnostic.passInterceptions += stats->passInterceptions;
            diagnostic.passBlocks += stats->passBlocks;
            diagnostic.shotBlocks += stats->shotBlocks;
            diagnostic.clearances += stats->clearances;
            diagnostic.looseBallRecoveries += stats->looseBallRecoveries;
            diagnostic.keeperClaims += stats->keeperClaims;
            diagnostic.dispossessionsForced += stats->dispossessionsForced;
            diagnostic.dribblesAttempted += stats->dribblesAttempted;
            diagnostic.dribblesWon += stats->dribblesWon;
            diagnostic.dribblesLost += stats->dribblesLost;
        }
        return diagnostic;
    }

    void addDefensiveEvents(
        DefensiveEventDiagnostic& total,
        const DefensiveEventDiagnostic& sample) {
        total.pressures += sample.pressures;
        total.duels += sample.duels;
        total.tackleAttempts += sample.tackleAttempts;
        total.tacklesWon += sample.tacklesWon;
        total.tacklesLost += sample.tacklesLost;
        total.passInterceptions += sample.passInterceptions;
        total.passBlocks += sample.passBlocks;
        total.shotBlocks += sample.shotBlocks;
        total.clearances += sample.clearances;
        total.looseBallRecoveries += sample.looseBallRecoveries;
        total.keeperClaims += sample.keeperClaims;
        total.dispossessionsForced += sample.dispossessionsForced;
        total.dribblesAttempted += sample.dribblesAttempted;
        total.dribblesWon += sample.dribblesWon;
        total.dribblesLost += sample.dribblesLost;
    }

    double ratio(int numerator, int denominator);

    void printDefensiveEvents(const DefensiveEventDiagnostic& stats) {
        std::cerr << "[Defensive events]\n"
            << "  pressures=" << stats.pressures
            << " duels=" << stats.duels
            << " duelWinRate=" << ratio(stats.tacklesWon, stats.duels)
            << " tackleAttempts=" << stats.tackleAttempts
            << " tacklesWon=" << stats.tacklesWon
            << " tacklesLost=" << stats.tacklesLost
            << " tackleWinRate=" << ratio(stats.tacklesWon, stats.tackleAttempts)
            << " passInterceptions=" << stats.passInterceptions
            << " passBlocks=" << stats.passBlocks
            << " shotBlocks=" << stats.shotBlocks
            << " clearances=" << stats.clearances
            << " looseBallRecoveries=" << stats.looseBallRecoveries
            << " keeperClaims=" << stats.keeperClaims
            << " dispossessionsForced=" << stats.dispossessionsForced
            << " dribblesAttempted=" << stats.dribblesAttempted
            << " dribblesWon=" << stats.dribblesWon
            << " dribblesLost=" << stats.dribblesLost
            << '\n';
    }

    void requireShotOutcomeInvariants(
        const std::string& label,
        int shots,
        int shotsOnTarget,
        int goals,
        const ShotOutcomeDiagnostic& outcomes) {
        const int outcomeShotTotal =
            goals
            + outcomes.offTarget
            + outcomes.savedHeld
            + outcomes.savedParried
            + outcomes.woodwork
            + outcomes.blockedShots;
        const int onTargetTotal = goals + outcomes.savedHeld + outcomes.savedParried;
        if (shots != outcomeShotTotal || shotsOnTarget != onTargetTotal) {
            std::cerr << "shot-outcome invariant failed label=" << label
                << " shots=" << shots
                << " outcomeShotTotal=" << outcomeShotTotal
                << " shotsOnTarget=" << shotsOnTarget
                << " onTargetTotal=" << onTargetTotal
                << " goals=" << goals
                << " offTarget=" << outcomes.offTarget
                << " savedHeld=" << outcomes.savedHeld
                << " savedParried=" << outcomes.savedParried
                << " woodwork=" << outcomes.woodwork
                << " blockedShots=" << outcomes.blockedShots
                << '\n';
        }
        require(shots == outcomeShotTotal,
            label + " shots should equal goals + off target + saves + woodwork + blocked shots");
        require(shotsOnTarget == onTargetTotal,
            label + " shots on target should equal goals + saves");
    }

    const MatchTeamSnapshot* teamSnapshotFor(
        const MatchEngineInput& input,
        TeamId teamId) {
        if (input.homeTeam.teamId == teamId) {
            return &input.homeTeam;
        }
        if (input.awayTeam.teamId == teamId) {
            return &input.awayTeam;
        }
        return nullptr;
    }

    const MatchPlayerSnapshot* playerSnapshotFor(
        const MatchEngineInput& input,
        PlayerId playerId) {
        for (const MatchPlayerSnapshot& player : input.homeTeam.players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }
        for (const MatchPlayerSnapshot& player : input.awayTeam.players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }
        return nullptr;
    }

    FormationSlotRole roleForPlayerInInput(
        const MatchEngineInput& input,
        TeamId teamId,
        PlayerId playerId) {
        const MatchTeamSnapshot* team = teamSnapshotFor(input, teamId);
        if (team == nullptr) {
            return FormationSlotRole::Unknown;
        }

        for (const TeamSheetSlotAssignment& assignment : team->teamSheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }
        return FormationSlotRole::Unknown;
    }

    std::string playerDisplayName(
        const MatchEngineInput& input,
        PlayerId playerId) {
        const MatchPlayerSnapshot* player = playerSnapshotFor(input, playerId);
        if (player != nullptr && !player->playerName.empty()) {
            return player->playerName;
        }
        return "Player " + std::to_string(playerId);
    }

    std::string teamDisplayName(
        const MatchEngineInput& input,
        TeamId teamId) {
        const MatchTeamSnapshot* team = teamSnapshotFor(input, teamId);
        if (team != nullptr && !team->teamName.empty()) {
            return team->teamName;
        }
        return "Team " + std::to_string(teamId);
    }

    void addResultToAggregate(
        SmokeAggregateDiagnostic& aggregate,
        const MatchEngineInput& input,
        const MatchEngineResult& result,
        TeamId teamFilter = 0) {
        for (const MatchPlayerSimulationStats& stats : result.playerStats) {
            if (stats.playerId == 0 || (teamFilter != 0 && stats.teamId != teamFilter)) {
                continue;
            }

            const RoleBucket bucket =
                bucketForRole(roleForPlayerInInput(input, stats.teamId, stats.playerId));
            RoleBucketDiagnostic& role = aggregate.roleBuckets[static_cast<int>(bucket)];
            ++role.playerSamples;
            role.passesAttempted += stats.passesAttempted;
            role.passesCompleted += stats.passesCompleted;
            role.shortPassesAttempted += stats.shortPassesAttempted;
            role.shortPassesCompleted += stats.shortPassesCompleted;
            role.throughBalls += stats.throughBalls;
            role.lowCrosses += stats.lowCrosses;
            role.cutbacks += stats.cutbacks;
            role.finalBalls += stats.finalBalls;
            role.shots += stats.shots;
            role.shotsOnTarget += stats.shotsOnTarget;
            role.goals += stats.goals;
            role.assistedShotsReceived += stats.assistedShotsReceived;
            role.assists += stats.assists;
            role.carryTraces += stats.carryTraces;
            role.dribbleTraces += stats.dribbleTraces;
            role.progressiveCarries += stats.progressiveCarries;
            role.defensive.pressures += stats.pressures;
            role.defensive.duels += stats.duels;
            role.defensive.tackleAttempts += stats.tackleAttempts;
            role.defensive.tacklesWon += stats.tacklesWon;
            role.defensive.tacklesLost += stats.tacklesLost;
            role.defensive.passInterceptions += stats.passInterceptions;
            role.defensive.passBlocks += stats.passBlocks;
            role.defensive.shotBlocks += stats.shotBlocks;
            role.defensive.clearances += stats.clearances;
            role.defensive.looseBallRecoveries += stats.looseBallRecoveries;
            role.defensive.keeperClaims += stats.keeperClaims;
            role.defensive.dispossessionsForced += stats.dispossessionsForced;
            role.defensive.dribblesAttempted += stats.dribblesAttempted;
            role.defensive.dribblesWon += stats.dribblesWon;
            role.defensive.dribblesLost += stats.dribblesLost;
            role.tackles += stats.tackles;
            role.preShotXG += stats.preShotExpectedGoals;
            role.effectiveXG += stats.expectedGoals;
            role.totalDistance += stats.totalDistanceCoveredMeters;
            role.onBallDistance += stats.onBallCarryDistanceMeters;
            role.offBallDistance += stats.offBallMovementDistanceMeters;

            const std::string playerName = playerDisplayName(input, stats.playerId);
            const std::string teamName = teamDisplayName(input, stats.teamId);
            const std::string leaderKey =
                teamName + "#" + playerName + "#" + roleBucketName(bucket);
            PlayerLeaderDiagnostic& leader = aggregate.playerLeaders[leaderKey];
            leader.playerId = stats.playerId;
            leader.teamId = stats.teamId;
            leader.playerName = playerName;
            leader.teamName = teamName;
            leader.roleBucket = bucket;
            ++leader.playerSamples;
            leader.passesAttempted += stats.passesAttempted;
            leader.passesCompleted += stats.passesCompleted;
            leader.finalBalls += stats.finalBalls;
            leader.carryTraces += stats.carryTraces;
            leader.dribbleTraces += stats.dribbleTraces;
            leader.shots += stats.shots;
            leader.defensive.pressures += stats.pressures;
            leader.defensive.duels += stats.duels;
            leader.defensive.tackleAttempts += stats.tackleAttempts;
            leader.defensive.tacklesWon += stats.tacklesWon;
            leader.defensive.tacklesLost += stats.tacklesLost;
            leader.defensive.passInterceptions += stats.passInterceptions;
            leader.defensive.passBlocks += stats.passBlocks;
            leader.defensive.shotBlocks += stats.shotBlocks;
            leader.defensive.clearances += stats.clearances;
            leader.defensive.looseBallRecoveries += stats.looseBallRecoveries;
            leader.defensive.keeperClaims += stats.keeperClaims;
            leader.defensive.dispossessionsForced += stats.dispossessionsForced;
            leader.defensive.dribblesAttempted += stats.dribblesAttempted;
            leader.defensive.dribblesWon += stats.dribblesWon;
            leader.defensive.dribblesLost += stats.dribblesLost;
            leader.tackles += stats.tackles;
            leader.expectedGoals += stats.expectedGoals;
            leader.totalDistance += stats.totalDistanceCoveredMeters;
        }
    }

    TraceMixDiagnostic traceMixFor(const MatchEngineResult& result, TeamId teamFilter = 0) {
        TraceMixDiagnostic mix;
        for (const MatchTraceFrame& frame : result.traceFrames) {
            if (teamFilter != 0 && frame.teamId != teamFilter) {
                continue;
            }
            if (frame.kind == MatchTraceKind::Carry) {
                ++mix.carryTraces;
            } else if (frame.kind == MatchTraceKind::Dribble) {
                ++mix.dribbleTraces;
            } else if (frame.kind == MatchTraceKind::Shot) {
                ++mix.shotTraces;
            }
        }
        return mix;
    }

    double ratio(int numerator, int denominator) {
        return denominator > 0
            ? static_cast<double>(numerator) / static_cast<double>(denominator)
            : 0.0;
    }

    void printRoleBuckets(
        const SmokeAggregateDiagnostic& aggregate,
        int sampleCount,
        const std::string& label) {
        std::cerr << "[Role buckets] " << label << '\n';
        for (RoleBucket bucket : {
            RoleBucket::Goalkeeper,
            RoleBucket::CenterBack,
            RoleBucket::FullbackWingback,
            RoleBucket::CentralMidfield,
            RoleBucket::Wide,
            RoleBucket::Striker,
            RoleBucket::Unknown }) {
            const auto it = aggregate.roleBuckets.find(static_cast<int>(bucket));
            if (it == aggregate.roleBuckets.end()) {
                continue;
            }
            const RoleBucketDiagnostic& stats = it->second;
            const double perMatch = 1.0 / static_cast<double>(std::max(sampleCount, 1));
            std::cerr << "  " << roleBucketName(bucket)
                << " playersSampled=" << stats.playerSamples
                << " passes=" << stats.passesCompleted << "/" << stats.passesAttempted
                << " passAcc=" << ratio(stats.passesCompleted, stats.passesAttempted)
                << " shortPasses=" << stats.shortPassesCompleted << "/" << stats.shortPassesAttempted
                << " throughBalls=" << stats.throughBalls
                << " lowCrosses=" << stats.lowCrosses
                << " cutbacks=" << stats.cutbacks
                << " finalBalls=" << stats.finalBalls
                << " shots=" << stats.shots
                << " shotsOnTarget=" << stats.shotsOnTarget
                << " goals=" << stats.goals
                << " preShotXG=" << stats.preShotXG
                << " effectiveXG=" << stats.effectiveXG
                << " assistedShotsReceived=" << stats.assistedShotsReceived
                << " assists=" << stats.assists
                << " carryTraces=" << stats.carryTraces
                << " dribbleTraces=" << stats.dribbleTraces
                << " progressiveCarries=" << stats.progressiveCarries
                << " pressures=" << stats.defensive.pressures
                << " duels=" << stats.defensive.duels
                << " tackleAttempts=" << stats.defensive.tackleAttempts
                << " tacklesWon=" << stats.defensive.tacklesWon
                << " tacklesLost=" << stats.defensive.tacklesLost
                << " passInterceptions=" << stats.defensive.passInterceptions
                << " passBlocks=" << stats.defensive.passBlocks
                << " shotBlocks=" << stats.defensive.shotBlocks
                << " clearances=" << stats.defensive.clearances
                << " looseBallRecoveries=" << stats.defensive.looseBallRecoveries
                << " keeperClaims=" << stats.defensive.keeperClaims
                << " dispossessionsForced=" << stats.defensive.dispossessionsForced
                << " dribbleAttempts=" << stats.defensive.dribblesAttempted
                << " dribblesWon=" << stats.defensive.dribblesWon
                << " dribblesLost=" << stats.defensive.dribblesLost
                << " totalDistanceMeters=" << stats.totalDistance
                << " distancePerMatch=" << stats.totalDistance * perMatch
                << " avgDistancePerPlayerMatch="
                << (stats.playerSamples > 0
                    ? stats.totalDistance / static_cast<double>(stats.playerSamples)
                    : 0.0)
                << " totalOnBallCarryMeters=" << stats.onBallDistance
                << " avgOnBallCarryMetersPerPlayerMatch="
                << (stats.playerSamples > 0
                    ? stats.onBallDistance / static_cast<double>(stats.playerSamples)
                    : 0.0)
                << " totalOffBallMeters=" << stats.offBallDistance
                << " avgOffBallMetersPerPlayerMatch="
                << (stats.playerSamples > 0
                    ? stats.offBallDistance / static_cast<double>(stats.playerSamples)
                    : 0.0)
                << '\n';
        }
        std::cerr << "  TODO: per-player touches are not tracked yet.\n";
    }

    void printMovementSummary(
        const SmokeAggregateDiagnostic& aggregate,
        int sampleCount) {
        double totalDistance = 0.0;
        double onBallDistance = 0.0;
        double offBallDistance = 0.0;
        int playerSamples = 0;
        for (const auto& [_, role] : aggregate.roleBuckets) {
            totalDistance += role.totalDistance;
            onBallDistance += role.onBallDistance;
            offBallDistance += role.offBallDistance;
            playerSamples += role.playerSamples;
        }
        std::cerr << "[Movement]\n"
            << "  totalDistanceMeters=" << totalDistance
            << " distancePerMatch=" << (totalDistance / static_cast<double>(std::max(sampleCount, 1)))
            << " avgDistancePerPlayerMatch="
            << (playerSamples > 0 ? totalDistance / static_cast<double>(playerSamples) : 0.0)
            << " totalOnBallCarryMeters=" << onBallDistance
            << " avgOnBallCarryMetersPerPlayerMatch="
            << (playerSamples > 0 ? onBallDistance / static_cast<double>(playerSamples) : 0.0)
            << " totalOffBallMeters=" << offBallDistance
            << " avgOffBallMetersPerPlayerMatch="
            << (playerSamples > 0 ? offBallDistance / static_cast<double>(playerSamples) : 0.0)
            << " resetMovementIgnoredMeters=0\n";
    }

    int roleBucketShotTotal(const SmokeAggregateDiagnostic& aggregate) {
        int total = 0;
        for (const auto& [_, role] : aggregate.roleBuckets) {
            total += role.shots;
        }
        return total;
    }

    int roleBucketPassAttemptTotal(const SmokeAggregateDiagnostic& aggregate) {
        int total = 0;
        for (const auto& [_, role] : aggregate.roleBuckets) {
            total += role.passesAttempted;
        }
        return total;
    }

    template<typename ValueFn>
    void printTopPlayers(
        const SmokeAggregateDiagnostic& aggregate,
        const std::string& title,
        ValueFn valueFn,
        int limit = 5) {
        std::vector<PlayerLeaderDiagnostic> players;
        players.reserve(aggregate.playerLeaders.size());
        for (const auto& [_, player] : aggregate.playerLeaders) {
            if (valueFn(player) > 0.0) {
                players.push_back(player);
            }
        }
        std::sort(
            players.begin(),
            players.end(),
            [&](const PlayerLeaderDiagnostic& lhs, const PlayerLeaderDiagnostic& rhs) {
                return valueFn(lhs) > valueFn(rhs);
            });

        std::cerr << "  " << title << ':';
        if (players.empty()) {
            std::cerr << " none\n";
            return;
        }
        std::cerr << '\n';
        const int count = std::min(limit, static_cast<int>(players.size()));
        for (int i = 0; i < count; ++i) {
            const PlayerLeaderDiagnostic& player = players[static_cast<std::size_t>(i)];
            std::cerr << "    " << player.playerName
                << " (" << player.teamName
                << ", " << roleBucketName(player.roleBucket)
                << ") value=" << valueFn(player)
                << '\n';
        }
    }

    void printPlayerLeaders(
        const SmokeAggregateDiagnostic& aggregate,
        const std::string& label) {
        std::cerr << "[Player leaders] " << label << '\n';
        printTopPlayers(aggregate, "passAttempts", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.passesAttempted);
        });
        printTopPlayers(aggregate, "passCompletions", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.passesCompleted);
        });
        printTopPlayers(aggregate, "finalBalls", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.finalBalls);
        });
        printTopPlayers(aggregate, "carryTraces", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.carryTraces);
        });
        printTopPlayers(aggregate, "dribbleTraces", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.dribbleTraces);
        });
        printTopPlayers(aggregate, "shots", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.shots);
        });
        printTopPlayers(aggregate, "xG", [](const PlayerLeaderDiagnostic& p) {
            return p.expectedGoals;
        });
        printTopPlayers(aggregate, "passInterceptions", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.defensive.passInterceptions);
        });
        printTopPlayers(aggregate, "tackleAttempts", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.defensive.tackleAttempts);
        });
        printTopPlayers(aggregate, "tacklesWon", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.defensive.tacklesWon);
        });
        printTopPlayers(aggregate, "pressures", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.defensive.pressures);
        });
        printTopPlayers(aggregate, "looseBallRecoveries", [](const PlayerLeaderDiagnostic& p) {
            return static_cast<double>(p.defensive.looseBallRecoveries);
        });
        std::vector<PlayerLeaderDiagnostic> distancePlayers;
        distancePlayers.reserve(aggregate.playerLeaders.size());
        for (const auto& [_, player] : aggregate.playerLeaders) {
            if (player.totalDistance > 0.0) {
                distancePlayers.push_back(player);
            }
        }
        std::sort(
            distancePlayers.begin(),
            distancePlayers.end(),
            [](const PlayerLeaderDiagnostic& lhs, const PlayerLeaderDiagnostic& rhs) {
                return lhs.totalDistance > rhs.totalDistance;
            });
        std::cerr << "  distanceCoveredMeters:\n";
        const int distanceCount = std::min(5, static_cast<int>(distancePlayers.size()));
        for (int i = 0; i < distanceCount; ++i) {
            const PlayerLeaderDiagnostic& player = distancePlayers[static_cast<std::size_t>(i)];
            std::cerr << "    " << player.playerName
                << " (" << player.teamName
                << ", " << roleBucketName(player.roleBucket)
                << ") totalDistanceMeters=" << player.totalDistance
                << " avgDistancePerMatch="
                << (player.playerSamples > 0
                    ? player.totalDistance / static_cast<double>(player.playerSamples)
                    : 0.0)
                << '\n';
        }
    }

    const char* shotZoneName(double shotDistanceMeters) {
        if (shotDistanceMeters <= 4.0) {
            return "0-4m";
        }
        if (shotDistanceMeters <= 8.0) {
            return "4-8m";
        }
        if (shotDistanceMeters <= 12.0) {
            return "8-12m";
        }
        if (shotDistanceMeters <= 18.0) {
            return "12-18m";
        }
        return "18m+";
    }

    void printGoalChains(
        const MatchEngineInput& input,
        const MatchEngineResult& result,
        const std::string& label) {
        if (result.goalChains.empty()) {
            return;
        }

        std::cerr << "[Goal chains] " << label << '\n';
        for (const MatchGoalChainDiagnostic& chain : result.goalChains) {
            std::cerr << "  minute=" << chain.minute
                << " team=" << teamDisplayName(input, chain.teamId)
                << " scorer=" << playerDisplayName(input, chain.scorerPlayerId)
                << " assist=" << (chain.assistPlayerId != 0
                    ? playerDisplayName(input, chain.assistPlayerId)
                    : "none")
                << " assistRetained=" << chain.assistRetained
                << " sourceAction=" << actionTypeName(chain.sourceActionType)
                << " sourceCategory=" << goalSourceCategoryName(chain.sourceCategory)
                << " finalPassType=" << actionTypeName(chain.sourceActionType)
                << " shotDistance=" << chain.shotDistanceMeters
                << " shotZone=" << shotZoneName(chain.shotDistanceMeters)
                << " preShotXG=" << chain.preShotXG
                << " keeperFacingXG=" << chain.keeperFacingXG
                << " effectiveXG=" << chain.effectiveXG
                << " shotPressure=" << chain.shotPressure
                << " closestDefenderDistance=" << chain.closestDefenderDistance
                << " carryAfterReceiveMeters=" << chain.carryAfterReceiveMeters
                << " touchesAfterReceive=" << chain.touchesAfterReceive
                << " defendersBeatenAfterReceive=" << chain.defendersBeatenAfterReceive
                << " carriedAfterReceive=" << chain.followedControlledCarryAfterReceive
                << " throughBall=" << chain.finalBallThroughBall
                << " lowCross=" << chain.finalBallLowCross
                << " cutback=" << chain.finalBallCutback
                << " highCross=" << chain.finalBallHighCross
                << " simplePass=" << chain.finalBallSimplePass
                << " rebound=" << chain.rebound
                << " transition=" << chain.transition
                << " turnover=" << chain.turnover
                << " assistNoneReason=" << chain.assistNoneReason
                << '\n';
        }
    }

    void printUnassistedGoalChains(const MatchEngineResult& result, int maxGoals = 4) {
        int printed = 0;
        for (std::size_t i = 0; i < result.traceFrames.size() && printed < maxGoals; ++i) {
            const MatchTraceFrame& frame = result.traceFrames[i];
            if (frame.kind != MatchTraceKind::Goal) {
                continue;
            }

            const std::size_t begin = i > 8 ? i - 8 : 0;
            std::cerr << "unassisted-goal-chain goalPlayer=" << frame.primaryPlayerId
                << " team=" << frame.teamId;
            for (std::size_t j = begin; j <= i; ++j) {
                const MatchTraceFrame& chainFrame = result.traceFrames[j];
                std::cerr << " [kind=" << static_cast<int>(chainFrame.kind)
                    << " p=" << chainFrame.primaryPlayerId
                    << " s=" << chainFrame.secondaryPlayerId
                    << " x=" << chainFrame.ballPosition.x
                    << " y=" << chainFrame.ballPosition.y << "]";
            }
            std::cerr << '\n';
            ++printed;
        }
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

    const PassOption* findOptionForKind(
        const std::vector<PassOption>& options,
        PassOptionKind kind) {
        for (const PassOption& option : options) {
            if (option.kind == kind) {
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
            pressure,
            phase,
            3,
            std::clamp(progress, 0.0, 1.0),
            true
        });
    }

    void runShotQualityModelSmoke() {
        const double closeCentral = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 102.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            6.0);
        const double centralEightMeters = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 97.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            6.0);
        const double centralTwelveMeters = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 93.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            6.0);
        const double centralEighteenMeters = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 87.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            6.0);
        const double wideAngle = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 97.0, 21.0 },
            AttackingDirection::HomeToAway,
            6.0);
        const double longShot = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 78.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            8.0);
        const double pressured = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 102.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::HomeToAway,
            70.0);
        const double reverseDirection = ShotQualityModel::calculateOpenPlayXG(
            PitchPoint{ 3.0, PitchGeometry::WidthMeters / 2.0 },
            AttackingDirection::AwayToHome,
            6.0);

        require(closeCentral > centralEightMeters, "2-4m central xG should exceed 7-9m central xG");
        require(centralEightMeters > centralTwelveMeters, "7-9m central xG should exceed 11-14m central xG");
        require(centralTwelveMeters > centralEighteenMeters, "11-14m central xG should exceed 16-20m central xG");
        require(centralTwelveMeters > wideAngle, "central box xG should exceed wide/tight angle xG");
        require(wideAngle > longShot, "wide box xG should still exceed long shot xG");
        require(closeCentral > pressured, "higher pressure should reduce xG");
        require(reverseDirection > centralTwelveMeters, "away-to-home xG should use the opposite goal");
        require(closeCentral >= 0.60 && closeCentral <= 0.90,
            "2-4m central clear chance should be very high xG");
        require(centralEightMeters >= 0.25 && centralEightMeters <= 0.50,
            "7-9m central chance should be good but not automatic");
        require(centralTwelveMeters >= 0.12 && centralTwelveMeters <= 0.30,
            "11-14m central chance should be medium to good");
        require(centralEighteenMeters >= 0.04 && centralEighteenMeters <= 0.12,
            "16-20m central chance should be low to medium-low");
        require(wideAngle <= centralTwelveMeters * 0.75,
            "wide/tight angle chance should be clearly lower than central chance");
        require(longShot <= 0.035, "long shot should remain low xG");
        for (double xg : {
            closeCentral,
            centralEightMeters,
            centralTwelveMeters,
            centralEighteenMeters,
            wideAngle,
            longShot,
            pressured,
            reverseDirection }) {
            require(xg > 0.0 && xg < 1.0, "open-play xG should remain a probability without an upper clamp");
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
        const ShotTargetZone zone{ ShotTargetLane::FarPost, ShotTargetHeight::Low };
        return ShotTargetSelectionResult{
            zone,
            shotTargetPointFor(
                context.shotOrigin,
                context.attackingDirection,
                zone),
            shotTargetFramePointFor(context.shotOrigin, zone),
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
                shotTargetFramePointFor(hardContext.shotOrigin, intended.intendedZone),
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

        require(closeQuality.rawXG > longQuality.rawXG,
            "central close shot should have higher raw xG than long shot");
        require(closeQuality.rawXG + 0.0001 >= closeQuality.preShotXG,
            "pre-shot xG should not exceed raw location xG");
        require(closeQuality.preShotXG + 0.0001 >= closeQuality.keeperFacingXG,
            "keeper-facing xG should not exceed pre-shot chance quality");
        require(closeQuality.preShotXG > longQuality.preShotXG,
            "central close shot should have higher pre-shot xG than long shot");
        require(closeQuality.keeperFacingXG > tightQuality.keeperFacingXG,
            "tight angle should lower keeper-facing shot quality");
        require(closeQuality.preShotXG > pressuredQuality.preShotXG,
            "pressure should lower pre-shot chance quality");
        require(closeQuality.keeperFacingXG > pressuredQuality.keeperFacingXG,
            "pressure should lower keeper-facing shot quality");
        require(blockedQuality.blockRisk > closeQuality.blockRisk,
            "high blocker risk should raise block risk");
        require(blockedQuality.preShotXG < closeQuality.preShotXG,
            "crowded lanes should lower pre-shot chance quality");
        require(blockedQuality.rawXG > blockedQuality.effectiveXG,
            "crowded lane should lower effective xG below raw xG");
        require(closeQuality.effectiveXG > longQuality.effectiveXG,
            "central close shot should keep higher effective xG than long shot");
        require(std::abs(eliteQuality.effectiveXG - closeQuality.effectiveXG) < 0.015,
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
        require(
            ShotAccuracyResolver{}.isOnTarget(ShotOutcomeContext{
                context,
                ShotType::PlacedShot,
                execution,
                quality,
                0x4401ULL
            }) == GoalFrame{}.contains(execution.actualFrameTarget),
            "shot accuracy should be resolved from actual frame target geometry");

        ShotExecutionResult woodworkExecution = execution;
        woodworkExecution.actualFrameTarget =
            ShotTargetPoint{ GoalFrame{}.width / 2.0 + 0.05, GoalFrame{}.height - 0.10 };
        const ShotOutcomeResult woodworkOutcome = ShotOutcomeResolver{}.resolve(ShotOutcomeContext{
            context,
            ShotType::PlacedShot,
            woodworkExecution,
            quality,
            0x4402ULL
        });
        require(woodworkOutcome.kind == ShotOutcomeKind::Woodwork,
            "actual target near the frame should resolve as woodwork");

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
        int highKeeperSaves = 0;
        int lowKeeperSaves = 0;
        int highHandlingHeld = 0;
        int lowHandlingHeld = 0;
        int highHandlingRebounds = 0;
        int lowHandlingRebounds = 0;
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
            highKeeperSaves += highOutcome.goal ? 0 : 1;
            lowKeeperSaves += lowOutcome.goal ? 0 : 1;
            highHandlingHeld += highOutcome.kind == ShotOutcomeKind::SavedHeld ? 1 : 0;
            lowHandlingHeld += lowOutcome.kind == ShotOutcomeKind::SavedHeld ? 1 : 0;
            highHandlingRebounds += highOutcome.kind == ShotOutcomeKind::SavedRebound ? 1 : 0;
            lowHandlingRebounds += lowOutcome.kind == ShotOutcomeKind::SavedRebound ? 1 : 0;
        }

        require(offTargetOccurred, "off-target shots should be able to occur");
        require(savedHeldOccurred, "saved-held outcomes should be able to occur");
        require(savedReboundOccurred, "saved-rebound outcomes should be able to occur");
        require(highHandlingHeld > lowHandlingHeld,
            "goalkeeper handling should increase held-save tendency");
        require(highKeeperSaves > lowKeeperSaves,
            "strong goalkeeper should produce more saves in aggregate");
        require(highHandlingHeld + highHandlingRebounds > 0
                && lowHandlingHeld + lowHandlingRebounds > 0,
            "keeper outcome smoke should exercise held and rebound save accounting");
        require(highKeeperGoals < lowKeeperGoals,
            "goalkeeper quality should affect save chance");
    }

    void runReboundTrajectoryModelSmoke() {
        const PitchPoint contactPoint{ 70.0, PitchGeometry::WidthMeters / 2.0 };
        BallTrajectory incoming;
        incoming.start = PitchPoint{ 50.0, PitchGeometry::WidthMeters / 2.0 };
        incoming.intendedTarget = PitchPoint{ 105.0, PitchGeometry::WidthMeters / 2.0 };
        incoming.actualTarget = PitchPoint{ 105.0, PitchGeometry::WidthMeters / 2.0 };
        incoming.startSecond = 12.0;
        incoming.arrivalSecond = 12.45;
        incoming.speedMetersPerSecond = 28.0;
        incoming.type = BallTrajectoryType::Shot;
        incoming.flightProfile = BallFlightProfile::Shot;
        incoming.apexHeightMeters = 0.9;

        ShotExecutionResult lowPowerExecution;
        lowPowerExecution.actualZone = ShotTargetZone{ ShotTargetLane::Center, ShotTargetHeight::Low };
        lowPowerExecution.actualTarget = incoming.actualTarget;
        lowPowerExecution.executionQuality = 62.0;
        lowPowerExecution.shotPower = 18.0;
        lowPowerExecution.placementQuality = 58.0;

        ShotExecutionResult highPowerExecution = lowPowerExecution;
        highPowerExecution.shotPower = 92.0;

        const ReboundTrajectoryBuilder builder;
        const std::uint64_t seed = 0xabc123ULL;
        const BallTrajectory first = builder.build(ReboundTrajectoryRequest{
            contactPoint,
            incoming,
            highPowerExecution,
            ShotOutcomeKind::SavedRebound,
            42.0,
            0.0,
            incoming.arrivalSecond,
            seed
        });
        const BallTrajectory second = builder.build(ReboundTrajectoryRequest{
            contactPoint,
            incoming,
            highPowerExecution,
            ShotOutcomeKind::SavedRebound,
            42.0,
            0.0,
            incoming.arrivalSecond,
            seed
        });
        require(PitchGeometry::distance(first.actualTarget, second.actualTarget) < 0.0001,
            "same rebound input and seed should produce the same target");
        require(first.arrivalSecond > first.startSecond,
            "rebound trajectory should arrive after it starts");

        const BallTrajectory lowPowerSaved = builder.build(ReboundTrajectoryRequest{
            contactPoint,
            incoming,
            lowPowerExecution,
            ShotOutcomeKind::SavedRebound,
            42.0,
            0.0,
            incoming.arrivalSecond,
            seed
        });
        require(
            PitchGeometry::distance(contactPoint, first.actualTarget)
                > PitchGeometry::distance(contactPoint, lowPowerSaved.actualTarget),
            "higher shot power should generally create a longer saved rebound");

        const BallTrajectory lowHandlingSaved = builder.build(ReboundTrajectoryRequest{
            contactPoint,
            incoming,
            highPowerExecution,
            ShotOutcomeKind::SavedRebound,
            28.0,
            0.0,
            incoming.arrivalSecond,
            seed
        });
        const BallTrajectory highHandlingSaved = builder.build(ReboundTrajectoryRequest{
            contactPoint,
            incoming,
            highPowerExecution,
            ShotOutcomeKind::SavedRebound,
            88.0,
            0.0,
            incoming.arrivalSecond,
            seed
        });
        require(
            PitchGeometry::distance(contactPoint, highHandlingSaved.actualTarget)
                < PitchGeometry::distance(contactPoint, lowHandlingSaved.actualTarget),
            "higher goalkeeper handling should reduce saved rebound distance");

        const BallTrajectory blockedDeflection = builder.build(ReboundTrajectoryRequest{
            contactPoint,
            incoming,
            highPowerExecution,
            ShotOutcomeKind::Blocked,
            50.0,
            0.85,
            incoming.arrivalSecond,
            seed
        });
        require(blockedDeflection.type == BallTrajectoryType::Deflection,
            "blocked shot rebound builder should create a deflection trajectory");
        require(first.type == BallTrajectoryType::Rebound,
            "saved shot rebound builder should create a rebound trajectory");
        require(blockedDeflection.flightProfile != first.flightProfile
                || PitchGeometry::distance(blockedDeflection.actualTarget, first.actualTarget) > 0.001,
            "blocked deflection and saved rebound should have different trajectory profiles");
        require(blockedDeflection.actualTarget.x >= 0.0
                && blockedDeflection.actualTarget.x <= PitchGeometry::LengthMeters
                && blockedDeflection.actualTarget.y >= 0.0
                && blockedDeflection.actualTarget.y <= PitchGeometry::WidthMeters,
            "rebound target should be clamped to the pitch");
        require(blockedDeflection.arrivalSecond > blockedDeflection.startSecond,
            "blocked deflection should arrive after it starts");
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
            PitchPoint{ 100.0, 8.0 });
        PassDecisionFixture defensiveFinalThird = buildPassDecisionFixture(
            FormationSlotRole::RightWinger,
            passDecisionAttributes(76, 74, 72),
            defensiveWide,
            PitchPoint{ 100.0, 8.0 });
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

        const std::vector<PassOption> finalBallOptions = evaluatePassFixture(attackingFinalThird);
        const PassOption* strikerCross = findOptionForKind(finalBallOptions, PassOptionKind::Cross);
        const PassOption* midfielderCutback = findOptionForKind(finalBallOptions, PassOptionKind::Cutback);
        const PassOption* strikerSimple =
            findOptionForTarget(finalBallOptions, PassOptionKind::ProgressivePass, 6);
        require(strikerCross != nullptr,
            "final-third wide fixture should expose a striker low-cross target");
        require(midfielderCutback != nullptr,
            "final-third wide fixture should expose a trailing cutback target");
        if (strikerCross != nullptr) {
            const double crossGoalDistance =
                PitchGeometry::distance(strikerCross->targetPoint, PitchGeometry::awayGoalCenter());
            require(crossGoalDistance >= 7.0 && crossGoalDistance <= 10.0,
                "low-cross target should be allowed into a realistic 7-10m chance corridor");
        }
        if (midfielderCutback != nullptr) {
            const double cutbackGoalDistance =
                PitchGeometry::distance(midfielderCutback->targetPoint, PitchGeometry::awayGoalCenter());
            require(cutbackGoalDistance >= 7.0 && cutbackGoalDistance <= 10.0,
                "cutback target should be allowed into a realistic 7-10m chance corridor");
        }
        if (strikerSimple != nullptr) {
            const double simpleGoalDistance =
                PitchGeometry::distance(strikerSimple->targetPoint, PitchGeometry::awayGoalCenter());
            require(simpleGoalDistance >= 11.0,
                "ordinary central pass target should keep static reception depth safer than final balls");
        }
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
            PitchPoint{ 102.0, PitchGeometry::WidthMeters / 2.0 });
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
        require(weakAttackingScore == 0.0 || weakAttackingScore > weakDefensiveScore,
            "attacking mentality should not make weak shot desire worse than defensive mentality");
        require(weakDefensiveScore == 0.0 || weakAttackingScore > weakDefensiveScore,
            "defensive mentality should lower or suppress weak shot desire");

        const PassDecisionFixture clearDefensiveShot = buildPassDecisionFixture(
            FormationSlotRole::Striker,
            shotDecisionAttributes(80, 78, 80),
            defensive,
            PitchPoint{ 102.0, 34.0 });
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

    TeamSheet makeShapeSmokeSheet(FormationId formationId = FormationId::FourThreeThree) {
        TeamSheet sheet;
        sheet.teamId = 6001;
        sheet.formation = formationId;
        const std::vector<FormationSlotRole>& roles = getFormationSlotTemplate(formationId);
        for (std::size_t i = 0; i < roles.size(); ++i) {
            sheet.startingAssignments.push_back(TeamSheetSlotAssignment{
                i,
                roles[i],
                static_cast<PlayerId>(7001 + i)
            });
        }

        return sheet;
    }

    TeamShapeContext makeShapeSmokeContext(
        TeamShapePhase phase,
        PitchPoint ballPosition,
        TeamMentality mentality = TeamMentality::Balanced) {
        TeamShapeContext context;
        context.teamId = 6001;
        context.isHomeTeam = true;
        context.hasPossession = phase == TeamShapePhase::InPossession
            || phase == TeamShapePhase::AttackingTransition;
        context.phase = phase;
        context.attackingDirection = AttackingDirection::HomeToAway;
        context.tacticalSetup.mentality = mentality;
        context.ballPosition = ballPosition;
        return context;
    }

    const PlayerShapeTarget& firstTargetForRole(
        const std::vector<PlayerShapeTarget>& targets,
        FormationSlotRole role) {
        const auto it = std::find_if(
            targets.begin(),
            targets.end(),
            [role](const PlayerShapeTarget& target) {
                return target.slotRole == role;
            });
        require(it != targets.end(), "shape smoke expected role target");
        return *it;
    }

    double averageTargetYExcludingGoalkeeper(const std::vector<PlayerShapeTarget>& targets) {
        double totalY = 0.0;
        int count = 0;
        for (const PlayerShapeTarget& target : targets) {
            if (target.slotRole == FormationSlotRole::Goalkeeper) {
                continue;
            }

            totalY += target.finalTarget.y;
            ++count;
        }

        require(count > 0, "shape smoke expected non-goalkeeper targets");
        return totalY / static_cast<double>(count);
    }

    double averageSignedProgressForRoles(
        const std::vector<PlayerShapeTarget>& targets,
        const std::vector<FormationSlotRole>& roles) {
        double totalX = 0.0;
        int count = 0;
        for (const PlayerShapeTarget& target : targets) {
            if (std::find(roles.begin(), roles.end(), target.slotRole) == roles.end()) {
                continue;
            }

            totalX += target.finalTarget.x;
            ++count;
        }

        require(count > 0, "shape smoke expected role-group targets");
        return totalX / static_cast<double>(count);
    }

    void requireTargetsInsidePitch(const std::vector<PlayerShapeTarget>& targets) {
        for (const PlayerShapeTarget& target : targets) {
            require(PitchGeometry::isInsidePitch(target.finalTarget),
                "team shape smoke finalTarget should remain inside pitch");
        }
    }

    void runTeamShapeDynamicTargetsSmoke() {
        const TeamSheet sheet = makeShapeSmokeSheet();
        const TeamShapeModel model;

        const std::vector<PlayerShapeTarget> inPossession = model.buildTargets(
            makeShapeSmokeContext(TeamShapePhase::InPossession, PitchPoint{ 62.0, 18.0 }),
            sheet);
        const std::vector<PlayerShapeTarget> outOfPossession = model.buildTargets(
            makeShapeSmokeContext(TeamShapePhase::OutOfPossession, PitchPoint{ 62.0, 18.0 }),
            sheet);

        int changedNonGoalkeeperTargets = 0;
        for (std::size_t i = 0; i < inPossession.size() && i < outOfPossession.size(); ++i) {
            if (inPossession[i].slotRole == FormationSlotRole::Goalkeeper) {
                continue;
            }

            if (PitchGeometry::distance(inPossession[i].finalTarget, outOfPossession[i].finalTarget) > 0.01) {
                ++changedNonGoalkeeperTargets;
            }
        }
        require(changedNonGoalkeeperTargets >= 5,
            "in-possession and out-of-possession shapes should differ for several non-GK players");

        const std::vector<PlayerShapeTarget> leftBall = model.buildTargets(
            makeShapeSmokeContext(TeamShapePhase::InPossession, PitchPoint{ 58.0, 10.0 }),
            sheet);
        const std::vector<PlayerShapeTarget> rightBall = model.buildTargets(
            makeShapeSmokeContext(TeamShapePhase::InPossession, PitchPoint{ 58.0, 58.0 }),
            sheet);
        require(averageTargetYExcludingGoalkeeper(rightBall)
                > averageTargetYExcludingGoalkeeper(leftBall) + 0.02,
            "ball side should shift team shape laterally");

        const std::vector<PlayerShapeTarget> attackingShape = model.buildTargets(
            makeShapeSmokeContext(TeamShapePhase::InPossession, PitchPoint{ 68.0, 34.0 }, TeamMentality::Attacking),
            sheet);
        const std::vector<PlayerShapeTarget> defensiveShape = model.buildTargets(
            makeShapeSmokeContext(TeamShapePhase::InPossession, PitchPoint{ 68.0, 34.0 }, TeamMentality::Defensive),
            sheet);
        const std::vector<FormationSlotRole> supportRoles{
            FormationSlotRole::CentralMidfielder,
            FormationSlotRole::LeftWinger,
            FormationSlotRole::RightWinger,
            FormationSlotRole::Striker
        };
        require(averageSignedProgressForRoles(attackingShape, supportRoles)
                > averageSignedProgressForRoles(defensiveShape, supportRoles) + 0.25,
            "attacking mentality should create more forward support than defensive mentality");
        require(firstTargetForRole(attackingShape, FormationSlotRole::CenterBack).finalTarget.x
                < firstTargetForRole(attackingShape, FormationSlotRole::Striker).finalTarget.x,
            "center backs should remain behind attackers in attacking possession shape");

        const std::vector<PlayerShapeTarget> defensiveTransition = model.buildTargets(
            makeShapeSmokeContext(TeamShapePhase::DefensiveTransition, PitchPoint{ 56.0, 9.0 }),
            sheet);
        const PlayerShapeTarget& nearSideWinger =
            firstTargetForRole(defensiveTransition, FormationSlotRole::LeftWinger);
        const PlayerShapeTarget& farSideWinger =
            firstTargetForRole(defensiveTransition, FormationSlotRole::RightWinger);
        require(PitchGeometry::distance(nearSideWinger.finalTarget, PitchPoint{ 56.0, 9.0 })
                < PitchGeometry::distance(farSideWinger.finalTarget, PitchPoint{ 56.0, 9.0 }),
            "defensive transition should pull nearby players closer to the ball than far-side players");

        int playersNotCollapsedToBall = 0;
        for (const PlayerShapeTarget& target : defensiveTransition) {
            if (PitchGeometry::distance(target.finalTarget, PitchPoint{ 56.0, 9.0 }) > 8.0) {
                ++playersNotCollapsedToBall;
            }
        }
        require(playersNotCollapsedToBall >= 6,
            "defensive transition should not move every player to the ball");

        requireTargetsInsidePitch(inPossession);
        requireTargetsInsidePitch(outOfPossession);
        requireTargetsInsidePitch(leftBall);
        requireTargetsInsidePitch(rightBall);
        requireTargetsInsidePitch(attackingShape);
        requireTargetsInsidePitch(defensiveShape);
        requireTargetsInsidePitch(defensiveTransition);
    }

    void runDetailedCoordinateBalanceSmoke() {
        int assistedGoalSamples = 0;
        int goalScorerRatingSamples = 0;
        int totalShots = 0;
        int totalShotsOnTarget = 0;
        int totalGoals = 0;
        int totalPasses = 0;
        int strongPassesAttempted = 0;
        int strongPassesCompleted = 0;
        int weakPassesAttempted = 0;
        int weakPassesCompleted = 0;
        int totalCarryTraces = 0;
        int totalDribbleTraces = 0;
        int totalShotTraces = 0;
        int hundredShotSamples = 0;
        int explodedXGSamples = 0;
        int extremePerfectConversionSamples = 0;
        int threeOpenPlayGoalZeroAssistSamples = 0;
        int maxSampleShots = 0;
        int maxSampleGoals = 0;
        std::uint64_t maxShotSeed = 0;
        double maxSampleXg = 0.0;
        double totalExpectedGoals = 0.0;
        double totalRawExpectedGoals = 0.0;
        double totalPreShotExpectedGoals = 0.0;
        double totalKeeperFacingExpectedGoals = 0.0;
        double totalBlockedExpectedGoals = 0.0;
        GoalSourceDiagnostic totalGoalSources;
        DefensiveEventDiagnostic totalDefensiveEvents;
        ShotOutcomeDiagnostic totalShotOutcomes;
        SmokeAggregateDiagnostic watchedAggregate;
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
            const int combinedShotsOnTarget =
                result.homeStats.shotsOnTarget + result.awayStats.shotsOnTarget;
            const int combinedGoals = result.homeStats.goals + result.awayStats.goals;
            const double combinedXG = result.homeStats.expectedGoals + result.awayStats.expectedGoals;
            const double combinedRawXG =
                result.homeStats.rawExpectedGoals + result.awayStats.rawExpectedGoals;
            const double combinedPreShotXG =
                result.homeStats.preShotExpectedGoals + result.awayStats.preShotExpectedGoals;
            const double combinedKeeperFacingXG =
                result.homeStats.keeperFacingExpectedGoals + result.awayStats.keeperFacingExpectedGoals;
            const double combinedBlockedXG =
                result.homeStats.blockedExpectedGoals + result.awayStats.blockedExpectedGoals;
            const GoalSourceDiagnostic goalSources = goalSourceDiagnosticFor(result);
            const DefensiveEventDiagnostic defensiveEvents = defensiveEventDiagnosticFor(result);
            const ShotOutcomeDiagnostic shotOutcomes = shotOutcomeDiagnosticFor(input, result);
            const int openPlayGoals =
                goalSources.assistedGoals + goalSources.unassistedOpenPlayGoals;
            if (openPlayGoals >= 3 && goalSources.assistedGoals == 0) {
                ++threeOpenPlayGoalZeroAssistSamples;
                printUnassistedGoalChains(result, 3);
            }
            if (combinedShots > maxSampleShots || combinedXG > maxSampleXg || combinedGoals > maxSampleGoals) {
                maxSampleShots = std::max(maxSampleShots, combinedShots);
                maxSampleGoals = std::max(maxSampleGoals, combinedGoals);
                maxSampleXg = std::max(maxSampleXg, combinedXG);
                maxShotSeed = 0x9300ULL + seed;
            }
            if (combinedShots >= 100) {
                ++hundredShotSamples;
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
                double homeMaxProgress = 0.0;
                double awayMaxProgress = 0.0;
                MatchTraceKind homeMaxKind = MatchTraceKind::PossessionStart;
                MatchTraceKind awayMaxKind = MatchTraceKind::PossessionStart;
                PitchPoint homeMaxPoint{};
                PitchPoint awayMaxPoint{};
                for (const MatchTraceFrame& frame : result.traceFrames) {
                    if (frame.teamId == result.homeStats.teamId) {
                        const double progress = frame.ballPosition.x / PitchGeometry::LengthMeters;
                        if (progress > homeMaxProgress) {
                            homeMaxProgress = progress;
                            homeMaxKind = frame.kind;
                            homeMaxPoint = frame.ballPosition;
                        }
                    } else if (frame.teamId == result.awayStats.teamId) {
                        const double progress =
                            (PitchGeometry::LengthMeters - frame.ballPosition.x) / PitchGeometry::LengthMeters;
                        if (progress > awayMaxProgress) {
                            awayMaxProgress = progress;
                            awayMaxKind = frame.kind;
                            awayMaxPoint = frame.ballPosition;
                        }
                    }
                }
                std::cerr << "anti-loop guardrail seed=" << (0x9300ULL + seed)
                    << " maxPossession=" << maxPossession
                    << " combinedPasses=" << combinedPasses
                    << " combinedShots=" << combinedShots
                    << " homeMaxProgress=" << homeMaxProgress
                    << " homeMaxKind=" << static_cast<int>(homeMaxKind)
                    << " homeMaxPoint=" << homeMaxPoint.x << "," << homeMaxPoint.y
                    << " awayMaxProgress=" << awayMaxProgress
                    << " awayMaxKind=" << static_cast<int>(awayMaxKind)
                    << " awayMaxPoint=" << awayMaxPoint.x << "," << awayMaxPoint.y
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
                    << " homePreShotXg=" << result.homeStats.preShotExpectedGoals
                    << " awayPreShotXg=" << result.awayStats.preShotExpectedGoals
                    << " passInterceptions="
                    << (result.homeStats.passInterceptions + result.awayStats.passInterceptions)
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

            const TraceMixDiagnostic traceMix = traceMixFor(result);
            totalCarryTraces += traceMix.carryTraces;
            totalDribbleTraces += traceMix.dribbleTraces;
            totalShotTraces += traceMix.shotTraces;
            addResultToAggregate(watchedAggregate, input, result);
            printGoalChains(input, result, "watched balance seed=" + std::to_string(0x9300ULL + seed));
            totalShots += combinedShots;
            totalShotsOnTarget += combinedShotsOnTarget;
            totalGoals += combinedGoals;
            totalPasses += combinedPasses;
            strongPassesAttempted += result.homeStats.passesAttempted;
            strongPassesCompleted += result.homeStats.passesCompleted;
            weakPassesAttempted += result.awayStats.passesAttempted;
            weakPassesCompleted += result.awayStats.passesCompleted;
            totalExpectedGoals += combinedXG;
            totalRawExpectedGoals += combinedRawXG;
            totalPreShotExpectedGoals += combinedPreShotXG;
            totalKeeperFacingExpectedGoals += combinedKeeperFacingXG;
            totalBlockedExpectedGoals += combinedBlockedXG;
            totalGoalSources.assistedGoals += goalSources.assistedGoals;
            totalGoalSources.unassistedOpenPlayGoals += goalSources.unassistedOpenPlayGoals;
            totalGoalSources.reboundGoals += goalSources.reboundGoals;
            totalGoalSources.transitionGoals += goalSources.transitionGoals;
            addDefensiveEvents(totalDefensiveEvents, defensiveEvents);
            totalShotOutcomes.offTarget += shotOutcomes.offTarget;
            totalShotOutcomes.savedHeld += shotOutcomes.savedHeld;
            totalShotOutcomes.savedParried += shotOutcomes.savedParried;
            totalShotOutcomes.woodwork += shotOutcomes.woodwork;
            totalShotOutcomes.blockedShots += shotOutcomes.blockedShots;
            totalShotOutcomes.reboundShots +=
                result.homeStats.reboundShots + result.awayStats.reboundShots;
            totalShotOutcomes.totalShotDistance += shotOutcomes.totalShotDistance;
            totalShotOutcomes.distanceSamples += shotOutcomes.distanceSamples;
            totalShotOutcomes.shotsInsideFourMeters += shotOutcomes.shotsInsideFourMeters;
            totalShotOutcomes.shotsFourToEightMeters += shotOutcomes.shotsFourToEightMeters;
            totalShotOutcomes.shotsEightToTwelveMeters += shotOutcomes.shotsEightToTwelveMeters;
            totalShotOutcomes.shotsTwelveToEighteenMeters += shotOutcomes.shotsTwelveToEighteenMeters;
            totalShotOutcomes.shotsBeyondEighteenMeters += shotOutcomes.shotsBeyondEighteenMeters;
            totalShotOutcomes.preShotUnder003 += shotOutcomes.preShotUnder003;
            totalShotOutcomes.preShot003To008 += shotOutcomes.preShot003To008;
            totalShotOutcomes.preShot008To015 += shotOutcomes.preShot008To015;
            totalShotOutcomes.preShot015To030 += shotOutcomes.preShot015To030;
            totalShotOutcomes.preShotAbove030 += shotOutcomes.preShotAbove030;
            const int statAssistedShots =
                result.homeStats.assistedShots + result.awayStats.assistedShots;
            const int statFinalBallShots =
                result.homeStats.finalBallShots + result.awayStats.finalBallShots;
            totalShotOutcomes.carryShots +=
                result.homeStats.soloCarryShots + result.awayStats.soloCarryShots;
            totalShotOutcomes.simplePassShots += std::max(0, statAssistedShots - statFinalBallShots);
            totalShotOutcomes.finalBallShots += statFinalBallShots;
            totalShotOutcomes.throughBallShots +=
                result.homeStats.throughBallShots + result.awayStats.throughBallShots;
            totalShotOutcomes.lowCrossShots +=
                result.homeStats.lowCrossShots + result.awayStats.lowCrossShots;
            totalShotOutcomes.cutbackShots +=
                result.homeStats.cutbackShots + result.awayStats.cutbackShots;
            totalShotOutcomes.reboundOrLooseShots +=
                result.homeStats.reboundShots + result.awayStats.reboundShots;
            totalShotOutcomes.turnoverShots +=
                result.homeStats.turnoverShots + result.awayStats.turnoverShots;
            totalShotOutcomes.assistedShots += statAssistedShots;
            totalShotOutcomes.soloShots +=
                result.homeStats.soloCarryShots + result.awayStats.soloCarryShots;

            for (const MatchTeamSimulationStats* stats : { &result.homeStats, &result.awayStats }) {
                require(stats->passesAttempted > 0, "detailed coordinate match should attempt passes");
                require(stats->shotsOnTarget <= stats->shots,
                    "shots on target should never exceed total shots");
                require(stats->goals <= stats->shotsOnTarget,
                    "goals should not exceed shots on target while own goals are not modeled");
                require(stats->rawExpectedGoals + 0.0001 >= stats->keeperFacingExpectedGoals,
                    "raw xG should be at least keeper-facing xG");
                require(stats->rawExpectedGoals + 0.0001 >= stats->preShotExpectedGoals,
                    "raw xG should be at least pre-shot xG");
                require(stats->preShotExpectedGoals + 0.0001 >= stats->keeperFacingExpectedGoals,
                    "pre-shot xG should be at least keeper-facing xG");
                require(stats->keeperFacingExpectedGoals + 0.0001 >= stats->expectedGoals,
                    "keeper-facing xG should be at least effective xG");
                if (stats->shotsOnTarget >= 8 && stats->goals == stats->shotsOnTarget) {
                    ++extremePerfectConversionSamples;
                }
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

        if (hundredShotSamples > 0 || explodedXGSamples > 0 || totalGoals >= 15) {
            std::cerr << "shot-volume diagnostic samples=" << hundredShotSamples
                << " maxSeed=" << maxShotSeed
                << " maxShots=" << maxSampleShots
                << " maxXg=" << maxSampleXg
                << " maxGoals=" << maxSampleGoals
                << " assistedGoals=" << totalGoalSources.assistedGoals
                << " unassistedOpenPlayGoals=" << totalGoalSources.unassistedOpenPlayGoals
                << " reboundGoals=" << totalGoalSources.reboundGoals
                << " transitionGoals=" << totalGoalSources.transitionGoals
                << '\n';
        }
        if (totalShots == 0) {
            std::cerr << "[Smoke config] watchedBalanceMatches=8 dominantMatches=3 seeds=0x9301-0x9308 scope=bothTeamsAggregate\n"
                << "[Action mix]\n"
                << "  totalPasses=" << totalPasses
                << " carryTraces=" << totalCarryTraces
                << " dribbleTraces=" << totalDribbleTraces
                << " totalShots=0"
                << " shotTraces=" << totalShotTraces
                << '\n'
                << "[Possession/Passing]\n"
                << "  strongPasses=" << strongPassesCompleted << "/" << strongPassesAttempted
                << " weakPasses=" << weakPassesCompleted << "/" << weakPassesAttempted
                << '\n'
                << "[Score]\n"
                << "  goals=" << totalGoals
                << '\n';
        }
        if (totalShots > 0) {
            const double xgPerShot =
                totalRawExpectedGoals / static_cast<double>(std::max(totalShots, 1));
            const double preShotXgPerShot =
                totalPreShotExpectedGoals / static_cast<double>(std::max(totalShots, 1));
            const double effectiveXgPerShot =
                totalExpectedGoals / static_cast<double>(std::max(totalShots, 1));
            const double averageShotDistance =
                totalShotOutcomes.distanceSamples > 0
                    ? totalShotOutcomes.totalShotDistance
                        / static_cast<double>(totalShotOutcomes.distanceSamples)
                    : 0.0;
            const double shotsInsideFourRate =
                static_cast<double>(totalShotOutcomes.shotsInsideFourMeters)
                / static_cast<double>(std::max(totalShots, 1));
            const double shotsFourToEightRate =
                static_cast<double>(totalShotOutcomes.shotsFourToEightMeters)
                / static_cast<double>(std::max(totalShots, 1));
            const double shotOnTargetRate =
                static_cast<double>(totalShotsOnTarget)
                / static_cast<double>(std::max(totalShots, 1));
            const double strongPassAccuracy =
                static_cast<double>(strongPassesCompleted)
                / static_cast<double>(std::max(strongPassesAttempted, 1));
            const double weakPassAccuracy =
                static_cast<double>(weakPassesCompleted)
                / static_cast<double>(std::max(weakPassesAttempted, 1));
            std::cerr << "[Smoke config]\n"
                << "  watchedBalanceMatches=8 dominantMatches=3 seeds=0x9301-0x9308 scope=bothTeamsAggregate\n"
                << "[Score]\n"
                << "  goals=" << totalGoals
                << " shots=" << totalShots
                << " shotsOnTarget=" << totalShotsOnTarget
                << " shotOnTargetRate=" << shotOnTargetRate
                << '\n'
                << "[Possession/Passing]\n"
                << "  totalPasses=" << totalPasses
                << " strongPasses=" << strongPassesCompleted << "/" << strongPassesAttempted
                << " strongPassAccuracy=" << strongPassAccuracy
                << " weakPasses=" << weakPassesCompleted << "/" << weakPassesAttempted
                << " weakPassAccuracy=" << weakPassAccuracy
                << '\n'
                << "[Pass outcome]\n"
                << "  passInterceptions=" << totalDefensiveEvents.passInterceptions
                << " passBlocks=" << totalDefensiveEvents.passBlocks
                << " looseBallRecoveries=" << totalDefensiveEvents.looseBallRecoveries
                << " keeperClaims=" << totalDefensiveEvents.keeperClaims
                << '\n'
                << "[Shot/xG]\n"
                << "  rawXG=" << totalRawExpectedGoals
                << " preShotXG=" << totalPreShotExpectedGoals
                << " keeperFacingXG=" << totalKeeperFacingExpectedGoals
                << " effectiveXG=" << totalExpectedGoals
                << " blockedXG=" << totalBlockedExpectedGoals
                << " xgPerShot=" << xgPerShot
                << " preShotXgPerShot=" << preShotXgPerShot
                << " effectiveXgPerShot=" << effectiveXgPerShot
                << '\n'
                << "  preShotLt003=" << totalShotOutcomes.preShotUnder003
                << " preShot003To008=" << totalShotOutcomes.preShot003To008
                << " preShot008To015=" << totalShotOutcomes.preShot008To015
                << " preShot015To030=" << totalShotOutcomes.preShot015To030
                << " preShotGt030=" << totalShotOutcomes.preShotAbove030
                << '\n'
                << "[Shot outcome]\n"
                << "  offTarget=" << totalShotOutcomes.offTarget
                << " savedHeld=" << totalShotOutcomes.savedHeld
                << " savedParried=" << totalShotOutcomes.savedParried
                << " woodwork=" << totalShotOutcomes.woodwork
                << " blockedShots=" << totalShotOutcomes.blockedShots
                << " reboundShots=" << totalShotOutcomes.reboundShots
                << '\n'
                << "[Shot distance]\n"
                << "  avgShotDistance=" << averageShotDistance
                << " shots0To4m=" << totalShotOutcomes.shotsInsideFourMeters
                << " shots4To8m=" << totalShotOutcomes.shotsFourToEightMeters
                << " shots8To12m=" << totalShotOutcomes.shotsEightToTwelveMeters
                << " shots12To18m=" << totalShotOutcomes.shotsTwelveToEighteenMeters
                << " shots18mPlus=" << totalShotOutcomes.shotsBeyondEighteenMeters
                << " shots0To4mRate=" << shotsInsideFourRate
                << " shots4To8mRate=" << shotsFourToEightRate
                << '\n'
                << "[Shot source]\n"
                << "  assistedShots=" << totalShotOutcomes.assistedShots
                << " soloCarryShots=" << totalShotOutcomes.soloShots
                << " simplePassShots=" << totalShotOutcomes.simplePassShots
                << " finalBallShots=" << totalShotOutcomes.finalBallShots
                << " throughBallShots=" << totalShotOutcomes.throughBallShots
                << " lowCrossShots=" << totalShotOutcomes.lowCrossShots
                << " cutbackShots=" << totalShotOutcomes.cutbackShots
                << " reboundShots=" << totalShotOutcomes.reboundOrLooseShots
                << " turnoverShots=" << totalShotOutcomes.turnoverShots
                << '\n'
                << "[Goal source]\n"
                << "  assistedGoals=" << totalGoalSources.assistedGoals
                << " unassistedOpenPlayGoals=" << totalGoalSources.unassistedOpenPlayGoals
                << " reboundGoals=" << totalGoalSources.reboundGoals
                << " transitionGoals=" << totalGoalSources.transitionGoals
                << '\n'
                << '\n';
            printDefensiveEvents(totalDefensiveEvents);
            printMovementSummary(watchedAggregate, 8);
            std::cerr << "[Action mix]\n"
                << "  passesAttempted=" << totalPasses
                << " carryTraces=" << totalCarryTraces
                << " dribbleTraces=" << totalDribbleTraces
                << " carryPlusDribbleTraces=" << (totalCarryTraces + totalDribbleTraces)
                << " shotTraces=" << totalShotTraces
                << '\n';
            printRoleBuckets(watchedAggregate, 8, "watched balance both-teams aggregate");
            printPlayerLeaders(watchedAggregate, "watched balance both-teams aggregate");
            std::cerr << "[Sanity checks]\n"
                << "  shotOutcomeInvariant=active"
                << " defensiveEventTracking=active"
                << " distanceResetMovement=ignored"
                << " roleBucketTotals=sampledFromPlayerStats\n";
            std::cerr << "[Guardrail summary]\n"
                << "  active=watched-shot-distance, watched-xg-mix, watched-sot-rate,"
                << " watched-pass-flow, assist-source, xg-sot-consistency,"
                << " shot-volume, conversion\n";
            requireShotOutcomeInvariants(
                "watched balance aggregate",
                totalShots,
                totalShotsOnTarget,
                totalGoals,
                totalShotOutcomes);
            if (totalDefensiveEvents.duels == 0 && totalDefensiveEvents.tackleAttempts == 0) {
                std::cerr << "NO_DUELS_OR_TACKLES_TRACKED: defensive contest system is not producing diagnostic events.\n";
            }
            require(totalDefensiveEvents.duels > 0 || totalDefensiveEvents.tackleAttempts > 0,
                "NO_DUELS_OR_TACKLES_TRACKED: defensive contest system is not producing diagnostic events.");
            require(roleBucketShotTotal(watchedAggregate) == totalShots,
                "watched role bucket shot totals should match aggregate shots");
            require(roleBucketPassAttemptTotal(watchedAggregate) == totalPasses,
                "watched role bucket pass totals should match aggregate passes");
            const int mediumOrBetterPreShotShots =
                totalShotOutcomes.preShot015To030 + totalShotOutcomes.preShotAbove030;
            const int usefulBoxCorridorShots =
                totalShotOutcomes.shotsFourToEightMeters
                + totalShotOutcomes.shotsEightToTwelveMeters
                + totalShotOutcomes.shotsTwelveToEighteenMeters;
            require(averageShotDistance >= 8.0,
                "watched samples should not create goalmouth-average shot distance");
            require(shotsInsideFourRate <= 0.12,
                "0-4m shots should be rare across watched samples");
            require(shotsFourToEightRate <= 0.35,
                "4-8m shots should not dominate watched samples");
            require(effectiveXgPerShot <= 0.25,
                "effective xG per shot should reflect mixed chance quality across watched samples");
            require(preShotXgPerShot >= 0.08,
                "pre-shot xG per shot should not collapse to weak long-shot quality across watched samples");
            require(totalPreShotExpectedGoals >= 3.0,
                "watched samples should create some meaningful pre-shot chance value");
            require(mediumOrBetterPreShotShots >= std::max(2, totalShots / 20),
                "watched samples should include some medium/high pre-shot chance positions");
            require(usefulBoxCorridorShots >= std::max(4, totalShots / 3),
                "watched samples should include realistic 4-18m shot locations");
            require(shotOnTargetRate <= 0.72,
                "shots on target should not be near automatic across watched samples");
            require(totalShotOutcomes.offTarget >= std::max(4, totalShots / 8),
                "watched samples should include natural off-target shots");
            require(strongPassAccuracy >= 0.62,
                "strong smoke team should retain plausible pass completion");
            require(weakPassAccuracy >= 0.52,
                "weaker smoke team should still complete ordinary circulation often enough");
            const int openPlayGoals =
                totalGoalSources.assistedGoals + totalGoalSources.unassistedOpenPlayGoals;
            if (openPlayGoals >= 6) {
                const double assistedOpenPlayGoalRate =
                    static_cast<double>(totalGoalSources.assistedGoals)
                    / static_cast<double>(std::max(openPlayGoals, 1));
                require(assistedOpenPlayGoalRate >= 0.40,
                    "watched open-play goals should retain plausible assisted source attribution");
            }
            const double assistedShotShare =
                static_cast<double>(totalShotOutcomes.assistedShots)
                / static_cast<double>(std::max(totalShots, 1));
            require(assistedShotShare >= 0.30,
                "assisted/final-ball shot source attribution should not collapse");
            if (totalGoals >= 4) {
                require(totalGoalSources.reboundGoals + totalGoalSources.transitionGoals
                        <= std::max(1, totalGoals / 2),
                    "rebound and transition goals should not dominate watched samples");
            }
        }

        int dominantShots = 0;
        int dominantShotsOnTarget = 0;
        int dominantGoals = 0;
        int dominantPassesAttempted = 0;
        int dominantPassesCompleted = 0;
        int dominantCarryTraces = 0;
        int dominantDribbleTraces = 0;
        double dominantPossession = 0.0;
        double dominantRawExpectedGoals = 0.0;
        double dominantPreShotExpectedGoals = 0.0;
        double dominantKeeperFacingExpectedGoals = 0.0;
        double dominantEffectiveExpectedGoals = 0.0;
        double dominantBlockedExpectedGoals = 0.0;
        GoalSourceDiagnostic dominantGoalSources;
        DefensiveEventDiagnostic dominantDefensiveEvents;
        ShotOutcomeDiagnostic dominantShotOutcomes;
        SmokeAggregateDiagnostic dominantAggregate;
        for (std::uint64_t seed = 1; seed <= 3; ++seed) {
            const MatchEngineInput input =
                buildInput(MatchSimulationDetail::WatchedMatch, 0x9400ULL + seed, 28, -6);
            const MatchEngineResult result = MatchEngine{}.simulate(input);
            assertResultReportConsistency(input, result);

            const ShotOutcomeDiagnostic shotOutcomes =
                shotOutcomeDiagnosticFor(input, result, result.homeStats.teamId);
            dominantShots += result.homeStats.shots;
            dominantShotsOnTarget += result.homeStats.shotsOnTarget;
            dominantGoals += result.homeStats.goals;
            dominantPassesAttempted += result.homeStats.passesAttempted;
            dominantPassesCompleted += result.homeStats.passesCompleted;
            dominantPossession += result.homeStats.possessionShare;
            dominantRawExpectedGoals += result.homeStats.rawExpectedGoals;
            dominantPreShotExpectedGoals += result.homeStats.preShotExpectedGoals;
            dominantKeeperFacingExpectedGoals += result.homeStats.keeperFacingExpectedGoals;
            dominantEffectiveExpectedGoals += result.homeStats.expectedGoals;
            dominantBlockedExpectedGoals += result.homeStats.blockedExpectedGoals;
            dominantGoalSources.assistedGoals += result.homeStats.assistedGoals;
            dominantGoalSources.unassistedOpenPlayGoals += result.homeStats.unassistedOpenPlayGoals;
            dominantGoalSources.reboundGoals += result.homeStats.reboundGoals;
            dominantGoalSources.transitionGoals += result.homeStats.transitionGoals;
            addDefensiveEvents(
                dominantDefensiveEvents,
                defensiveEventDiagnosticFor(result, result.homeStats.teamId));
            dominantShotOutcomes.offTarget += shotOutcomes.offTarget;
            dominantShotOutcomes.savedHeld += shotOutcomes.savedHeld;
            dominantShotOutcomes.savedParried += shotOutcomes.savedParried;
            dominantShotOutcomes.woodwork += shotOutcomes.woodwork;
            dominantShotOutcomes.blockedShots += shotOutcomes.blockedShots;
            dominantShotOutcomes.reboundShots += result.homeStats.reboundShots;
            dominantShotOutcomes.totalShotDistance += shotOutcomes.totalShotDistance;
            dominantShotOutcomes.distanceSamples += shotOutcomes.distanceSamples;
            dominantShotOutcomes.shotsInsideFourMeters += shotOutcomes.shotsInsideFourMeters;
            dominantShotOutcomes.shotsFourToEightMeters += shotOutcomes.shotsFourToEightMeters;
            dominantShotOutcomes.shotsEightToTwelveMeters += shotOutcomes.shotsEightToTwelveMeters;
            dominantShotOutcomes.shotsTwelveToEighteenMeters += shotOutcomes.shotsTwelveToEighteenMeters;
            dominantShotOutcomes.shotsBeyondEighteenMeters += shotOutcomes.shotsBeyondEighteenMeters;
            dominantShotOutcomes.preShotUnder003 += shotOutcomes.preShotUnder003;
            dominantShotOutcomes.preShot003To008 += shotOutcomes.preShot003To008;
            dominantShotOutcomes.preShot008To015 += shotOutcomes.preShot008To015;
            dominantShotOutcomes.preShot015To030 += shotOutcomes.preShot015To030;
            dominantShotOutcomes.preShotAbove030 += shotOutcomes.preShotAbove030;
            dominantShotOutcomes.carryShots += result.homeStats.soloCarryShots;
            dominantShotOutcomes.simplePassShots +=
                std::max(0, result.homeStats.assistedShots - result.homeStats.finalBallShots);
            dominantShotOutcomes.finalBallShots += result.homeStats.finalBallShots;
            dominantShotOutcomes.throughBallShots += result.homeStats.throughBallShots;
            dominantShotOutcomes.lowCrossShots += result.homeStats.lowCrossShots;
            dominantShotOutcomes.cutbackShots += result.homeStats.cutbackShots;
            dominantShotOutcomes.reboundOrLooseShots += result.homeStats.reboundShots;
            dominantShotOutcomes.turnoverShots += result.homeStats.turnoverShots;
            dominantShotOutcomes.assistedShots += result.homeStats.assistedShots;
            dominantShotOutcomes.soloShots += result.homeStats.soloCarryShots;
            addResultToAggregate(dominantAggregate, input, result, result.homeStats.teamId);
            printGoalChains(input, result, "dominant seed=" + std::to_string(0x9400ULL + seed));

            const TraceMixDiagnostic dominantTraceMix = traceMixFor(result, result.homeStats.teamId);
            dominantCarryTraces += dominantTraceMix.carryTraces;
            dominantDribbleTraces += dominantTraceMix.dribbleTraces;
        }

        const double dominantPreShotXgPerShot =
            dominantPreShotExpectedGoals / static_cast<double>(std::max(dominantShots, 1));
        const double dominantEffectiveXgPerShot =
            dominantEffectiveExpectedGoals / static_cast<double>(std::max(dominantShots, 1));
        const double dominantAverageShotDistance =
            dominantShotOutcomes.distanceSamples > 0
                ? dominantShotOutcomes.totalShotDistance
                    / static_cast<double>(dominantShotOutcomes.distanceSamples)
                : 0.0;
        const double dominantShotsInsideFourRate =
            static_cast<double>(dominantShotOutcomes.shotsInsideFourMeters)
            / static_cast<double>(std::max(dominantShots, 1));
        const double dominantShotOnTargetRate =
            static_cast<double>(dominantShotsOnTarget) / static_cast<double>(std::max(dominantShots, 1));
        const double dominantPassAccuracy =
            static_cast<double>(dominantPassesCompleted)
            / static_cast<double>(std::max(dominantPassesAttempted, 1));
        const int dominantMediumOrBetterShots =
            dominantShotOutcomes.preShot015To030 + dominantShotOutcomes.preShotAbove030;
        std::cerr << "[Smoke config]\n"
            << "  watchedBalanceMatches=8 dominantMatches=3 seeds=0x9401-0x9403 scope=dominantTeamOnly\n"
            << "[Score]\n"
            << "  goals=" << dominantGoals
            << " shots=" << dominantShots
            << " shotsOnTarget=" << dominantShotsOnTarget
            << " shotOnTargetRate=" << dominantShotOnTargetRate
            << '\n'
            << "[Possession/Passing]\n"
            << "  averagePossession=" << (dominantPossession / 3.0)
            << " passes=" << dominantPassesCompleted << "/" << dominantPassesAttempted
            << " passAccuracy=" << dominantPassAccuracy
            << '\n'
            << "[Pass outcome]\n"
            << "  passInterceptions=" << dominantDefensiveEvents.passInterceptions
            << " passBlocks=" << dominantDefensiveEvents.passBlocks
            << " looseBallRecoveries=" << dominantDefensiveEvents.looseBallRecoveries
            << " keeperClaims=" << dominantDefensiveEvents.keeperClaims
            << '\n'
            << "[Shot/xG]\n"
            << "  rawXG=" << dominantRawExpectedGoals
            << " preShotXG=" << dominantPreShotExpectedGoals
            << " keeperFacingXG=" << dominantKeeperFacingExpectedGoals
            << " effectiveXG=" << dominantEffectiveExpectedGoals
            << " blockedXG=" << dominantBlockedExpectedGoals
            << " preShotXgPerShot=" << dominantPreShotXgPerShot
            << " effectiveXgPerShot=" << dominantEffectiveXgPerShot
            << '\n'
            << "  preShotLt003=" << dominantShotOutcomes.preShotUnder003
            << " preShot003To008=" << dominantShotOutcomes.preShot003To008
            << " preShot008To015=" << dominantShotOutcomes.preShot008To015
            << " preShot015To030=" << dominantShotOutcomes.preShot015To030
            << " preShotGt030=" << dominantShotOutcomes.preShotAbove030
            << '\n'
            << "[Shot outcome]\n"
            << "  offTarget=" << dominantShotOutcomes.offTarget
            << " savedHeld=" << dominantShotOutcomes.savedHeld
            << " savedParried=" << dominantShotOutcomes.savedParried
            << " woodwork=" << dominantShotOutcomes.woodwork
            << " blockedShots=" << dominantShotOutcomes.blockedShots
            << " reboundShots=" << dominantShotOutcomes.reboundShots
            << '\n'
            << "[Shot distance]\n"
            << "  avgShotDistance=" << dominantAverageShotDistance
            << " shots0To4m=" << dominantShotOutcomes.shotsInsideFourMeters
            << " shots4To8m=" << dominantShotOutcomes.shotsFourToEightMeters
            << " shots8To12m=" << dominantShotOutcomes.shotsEightToTwelveMeters
            << " shots12To18m=" << dominantShotOutcomes.shotsTwelveToEighteenMeters
            << " shots18mPlus=" << dominantShotOutcomes.shotsBeyondEighteenMeters
            << '\n'
            << "[Shot source]\n"
            << "  carryShots=" << dominantShotOutcomes.carryShots
            << " simplePassShots=" << dominantShotOutcomes.simplePassShots
            << " finalBallShots=" << dominantShotOutcomes.finalBallShots
            << " throughBallShots=" << dominantShotOutcomes.throughBallShots
            << " lowCrossShots=" << dominantShotOutcomes.lowCrossShots
            << " cutbackShots=" << dominantShotOutcomes.cutbackShots
            << " reboundOrLooseShots=" << dominantShotOutcomes.reboundOrLooseShots
            << " turnoverShots=" << dominantShotOutcomes.turnoverShots
            << " assistedShots=" << dominantShotOutcomes.assistedShots
            << " soloShots=" << dominantShotOutcomes.soloShots
            << '\n'
            << "[Goal source]\n"
            << "  assistedGoals=" << dominantGoalSources.assistedGoals
            << " unassistedOpenPlayGoals=" << dominantGoalSources.unassistedOpenPlayGoals
            << " reboundGoals=" << dominantGoalSources.reboundGoals
            << " transitionGoals=" << dominantGoalSources.transitionGoals
            << '\n'
            << '\n';
        printDefensiveEvents(dominantDefensiveEvents);
        printMovementSummary(dominantAggregate, 3);
        std::cerr << "[Action mix]\n"
            << "  carryTraces=" << dominantCarryTraces
            << " dribbleTraces=" << dominantDribbleTraces
            << " carryPlusDribbleTraces=" << (dominantCarryTraces + dominantDribbleTraces)
            << '\n';
        printRoleBuckets(dominantAggregate, 3, "dominant team only");
        printPlayerLeaders(dominantAggregate, "dominant team only");
        std::cerr << "[Sanity checks]\n"
            << "  shotOutcomeInvariant=active"
            << " defensiveEventTracking=active"
            << " distanceResetMovement=ignored"
            << " roleBucketTotals=sampledFromPlayerStats\n";
        std::cerr << "[Guardrail summary]\n"
            << "  active=dominant-shot-volume, dominant-xg-creation,"
            << " dominant-shot-distance, dominant-sot-rate, dominant-pass-flow\n";
        requireShotOutcomeInvariants(
            "dominant team aggregate",
            dominantShots,
            dominantShotsOnTarget,
            dominantGoals,
            dominantShotOutcomes);
        require(roleBucketShotTotal(dominantAggregate) == dominantShots,
            "dominant role bucket shot totals should match aggregate shots");
        require(roleBucketPassAttemptTotal(dominantAggregate) == dominantPassesAttempted,
            "dominant role bucket pass totals should match aggregate passes");
        require(dominantShots > 0,
            "dominant watched samples should generate shots");
        require(dominantPreShotExpectedGoals >= 2.0,
            "dominant watched samples should not collapse to very low pre-shot xG");
        require(dominantPreShotXgPerShot >= 0.08,
            "dominant watched samples should find some useful shooting positions");
        require(dominantMediumOrBetterShots >= 2,
            "dominant watched samples should create some medium/high pre-shot chance positions");
        require(dominantAverageShotDistance >= 8.0,
            "dominant watched samples should not rely on goalmouth-average shot distance");
        require(dominantShotsInsideFourRate <= 0.15,
            "dominant watched samples should keep 0-4m shots rare");
        require(dominantEffectiveXgPerShot <= 0.30,
            "dominant watched samples should not revive repeated extreme xG shots");
        require(dominantShotOnTargetRate <= 0.78,
            "dominant watched samples should not make every shot hit the target");
        require(dominantPassAccuracy >= 0.64,
            "dominant watched samples should retain sane passing flow");

        if (totalGoals >= 3) {
            require(assistedGoalSamples > 0,
                "detailed coordinate possession-created goals should be able to record assists");
        }
        if (totalGoals > 0) {
            require(goalScorerRatingSamples > 0,
                "detailed coordinate goal scorers should receive a visible rating lift");
        }
        require(totalShots > 0 && totalShotTraces > 0,
            "detailed coordinate samples should include terminal shot actions");
        require(threeOpenPlayGoalZeroAssistSamples == 0,
            "matches with 3+ non-rebound open-play goals should not have zero assists");
        require(totalCarryTraces > 0,
            "detailed coordinate samples should include carry actions without suppressing shots");
        require(totalPasses > totalShots,
            "detailed coordinate samples should retain passing without becoming only shots");
        if (totalGoals >= 3 && assistedGoalSamples == 0) {
            std::cerr << "assist-source diagnostic totalGoals=" << totalGoals
                << " totalShots=" << totalShots
                << " totalSOT=" << totalShotsOnTarget
                << " totalXg=" << totalExpectedGoals
                << " assistedGoals=" << totalGoalSources.assistedGoals
                << " unassistedOpenPlayGoals=" << totalGoalSources.unassistedOpenPlayGoals
                << " reboundGoals=" << totalGoalSources.reboundGoals
                << " transitionGoals=" << totalGoalSources.transitionGoals
                << '\n';
        }
        if (totalExpectedGoals >= 12.0 && totalShotsOnTarget * 3 < totalShots) {
            const double explainedByBlocks =
                totalBlockedExpectedGoals / std::max(totalKeeperFacingExpectedGoals, 0.001);
            if (explainedByBlocks < 0.35) {
                std::cerr << "xg-sot consistency guardrail effectiveXG=" << totalExpectedGoals
                    << " keeperFacingXG=" << totalKeeperFacingExpectedGoals
                    << " blockedXG=" << totalBlockedExpectedGoals
                    << " totalShots=" << totalShots
                    << " totalSOT=" << totalShotsOnTarget
                    << " blockedShots=" << totalShotOutcomes.blockedShots
                    << '\n';
            }
            require(explainedByBlocks >= 0.35,
                "high effective xG with low SOT should be explained by blocked xG");
        }
        require(hundredShotSamples <= 1,
            "balanced 4-4-2 coordinate matches should not regularly produce 100+ shots");
        require(explodedXGSamples == 0,
            "balanced 4-4-2 coordinate matches should not explode into 30+ total xG");
        require(extremePerfectConversionSamples <= 1,
            "balanced coordinate samples should not regularly convert every high-volume shot on target");
        if (totalShotsOnTarget >= 20) {
            const double conversion =
                static_cast<double>(totalGoals) / static_cast<double>(totalShotsOnTarget);
            if (conversion >= 0.70) {
                std::cerr << "SOT conversion guardrail totalGoals=" << totalGoals
                    << " totalSOT=" << totalShotsOnTarget
                    << " conversion=" << conversion
                    << " totalShots=" << totalShots
                    << '\n';
            }
            require(static_cast<double>(totalGoals) / static_cast<double>(totalShotsOnTarget) < 0.70,
                "balanced coordinate samples should not regularly exceed 70 percent SOT-to-goal conversion");
        }
    }

    void runGoalSourceDiagnosticMetadataSmoke() {
        MatchEngineResult result;
        result.homeStats.teamId = 1;
        result.homeStats.assistedGoals = 2;
        result.homeStats.reboundGoals = 1;
        result.homeStats.transitionGoals = 1;
        result.homeStats.assistedShots = 2;
        result.homeStats.finalBallShots = 1;
        result.homeStats.lowCrossShots = 1;
        result.homeStats.reboundShots = 1;
        result.homeStats.turnoverShots = 1;

        MatchGoalChainDiagnostic finalBallGoal;
        finalBallGoal.minute = 12;
        finalBallGoal.teamId = 1;
        finalBallGoal.scorerPlayerId = 9;
        finalBallGoal.assistPlayerId = 8;
        finalBallGoal.sourceActionType = BallCarrierActionType::LowCross;
        finalBallGoal.sourceCategory = MatchGoalSourceCategory::AssistedFinalBall;
        finalBallGoal.shotDistanceMeters = 7.5;
        finalBallGoal.preShotXG = 0.24;
        finalBallGoal.keeperFacingXG = 0.21;
        finalBallGoal.effectiveXG = 0.18;
        finalBallGoal.assistRetained = true;
        finalBallGoal.finalBallLowCross = true;
        result.goalChains.push_back(finalBallGoal);

        MatchGoalChainDiagnostic simpleAssistGoal;
        simpleAssistGoal.minute = 24;
        simpleAssistGoal.teamId = 1;
        simpleAssistGoal.scorerPlayerId = 10;
        simpleAssistGoal.assistPlayerId = 6;
        simpleAssistGoal.sourceActionType = BallCarrierActionType::ShortPass;
        simpleAssistGoal.sourceCategory = MatchGoalSourceCategory::AssistedSimplePass;
        simpleAssistGoal.shotDistanceMeters = 11.0;
        simpleAssistGoal.preShotXG = 0.14;
        simpleAssistGoal.keeperFacingXG = 0.12;
        simpleAssistGoal.effectiveXG = 0.10;
        simpleAssistGoal.assistRetained = true;
        simpleAssistGoal.finalBallSimplePass = true;
        result.goalChains.push_back(simpleAssistGoal);

        MatchGoalChainDiagnostic reboundGoal;
        reboundGoal.minute = 53;
        reboundGoal.teamId = 1;
        reboundGoal.scorerPlayerId = 11;
        reboundGoal.sourceActionType = BallCarrierActionType::Hold;
        reboundGoal.sourceCategory = MatchGoalSourceCategory::Rebound;
        reboundGoal.shotDistanceMeters = 5.5;
        reboundGoal.preShotXG = 0.36;
        reboundGoal.keeperFacingXG = 0.31;
        reboundGoal.effectiveXG = 0.28;
        reboundGoal.rebound = true;
        reboundGoal.assistNoneReason = "Rebound";
        result.goalChains.push_back(reboundGoal);

        MatchGoalChainDiagnostic turnoverGoal;
        turnoverGoal.minute = 71;
        turnoverGoal.teamId = 1;
        turnoverGoal.scorerPlayerId = 7;
        turnoverGoal.sourceActionType = BallCarrierActionType::Carry;
        turnoverGoal.sourceCategory = MatchGoalSourceCategory::Turnover;
        turnoverGoal.shotDistanceMeters = 14.0;
        turnoverGoal.preShotXG = 0.08;
        turnoverGoal.keeperFacingXG = 0.06;
        turnoverGoal.effectiveXG = 0.05;
        turnoverGoal.followedControlledCarryAfterReceive = true;
        turnoverGoal.turnover = true;
        turnoverGoal.transition = true;
        turnoverGoal.assistNoneReason = "Turnover";
        result.goalChains.push_back(turnoverGoal);

        const GoalSourceDiagnostic sources = goalSourceDiagnosticFor(result);
        require(sources.assistedGoals == 2,
            "goal source diagnostics should preserve assisted goal count");
        require(sources.reboundGoals == 1,
            "goal source diagnostics should preserve rebound goal count");
        require(sources.transitionGoals == 1,
            "goal source diagnostics should preserve transition goal count");
        require(result.homeStats.finalBallShots == 1 && result.homeStats.lowCrossShots == 1,
            "final-ball source diagnostics should preserve low-cross shot counts");
        require(result.homeStats.reboundShots == 1,
            "rebound source diagnostics should preserve rebound shot counts");
        require(result.homeStats.turnoverShots == 1,
            "turnover source diagnostics should preserve turnover shot counts");
        require(result.goalChains[0].sourceCategory == MatchGoalSourceCategory::AssistedFinalBall
                && result.goalChains[0].finalBallLowCross,
            "goal chain diagnostics should preserve final-ball source metadata");
        require(result.goalChains[1].sourceCategory == MatchGoalSourceCategory::AssistedSimplePass
                && result.goalChains[1].finalBallSimplePass,
            "goal chain diagnostics should preserve simple assist source metadata");
        require(result.goalChains[2].sourceCategory == MatchGoalSourceCategory::Rebound,
            "goal chain diagnostics should preserve rebound source metadata");
        require(result.goalChains[3].sourceCategory == MatchGoalSourceCategory::Turnover
                && result.goalChains[3].followedControlledCarryAfterReceive,
            "goal chain diagnostics should preserve turnover carry source metadata");
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
        runReboundTrajectoryModelSmoke();
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
        runTeamShapeDynamicTargetsSmoke();
        runDetailedCoordinateBalanceSmoke();
        runGoalSourceDiagnosticMetadataSmoke();
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
