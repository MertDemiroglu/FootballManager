#include"fm/match_engine/MatchEngine.h"
#include"fm/match_engine/MatchEngineInputBuilder.h"
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
        std::uint64_t deterministicSeed = 0x51f0c0deULL) {
        Team homeTeam = makeTeam(101, "Home Smoke", 4);
        Team awayTeam = makeTeam(202, "Away Smoke", 0);

        TeamSelectionService selectionService;
        TeamSheet homeSheet = selectionService.buildTeamSheet(homeTeam, FormationId::FourFourTwo);
        TeamSheet awaySheet = selectionService.buildTeamSheet(awayTeam, FormationId::FourFourTwo);

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
            require(combinedShots >= 12 && combinedShots <= 40,
                "background summary combined shots should stay in a playable range");
            require(combinedXg >= 0.4 && combinedXg <= 6.5,
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
        runDetailSmoke(MatchSimulationDetail::BackgroundSummary);
        runDetailSmoke(MatchSimulationDetail::WatchedMatch);
        runDetailSmoke(MatchSimulationDetail::DebugFullTrace);
        runOfficialGoalEventSmoke();
        runBackgroundSummaryCalibrationSmoke();
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
