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

    MatchEngineInput buildInput(MatchSimulationDetail detail) {
        Team homeTeam = makeTeam(101, "Home Smoke", 4);
        Team awayTeam = makeTeam(202, "Away Smoke", 0);

        TeamSelectionService selectionService;
        TeamSheet homeSheet = selectionService.buildTeamSheet(homeTeam, FormationId::FourFourTwo);
        TeamSheet awaySheet = selectionService.buildTeamSheet(awayTeam, FormationId::FourFourTwo);

        MatchEngineOptions options;
        options.detail = detail;
        options.deterministicSeed = 0x51f0c0deULL;

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
        require(report.homeId == input.homeTeam.teamId, "home report team id should match input");
        require(report.awayId == input.awayTeam.teamId, "away report team id should match input");
        require(report.matchId == input.matchId, "report match id should match input");
        require(report.leagueId == input.leagueId, "report league id should match input");
        require(report.seasonYear == input.seasonYear, "report season year should match input");
        require(report.matchweek == input.matchweek, "report matchweek should match input");
        require(hasStarterReports(report, input.homeTeam.teamSheet, input.awayTeam.teamSheet),
            "report should contain started player reports for starters");
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

    void runHandlerSmoke(MatchSimulationEngineMode engineMode) {
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
        runHandlerSmoke(MatchSimulationEngineMode::Coordinate);
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
}

int main() {
    try {
        runDeterministicRegression();
        runInvalidInputSmoke();
        runDetailSmoke(MatchSimulationDetail::BackgroundSummary);
        runDetailSmoke(MatchSimulationDetail::WatchedMatch);
        runDetailSmoke(MatchSimulationDetail::DebugFullTrace);
        runHandlerIntegrationSmoke();
        runPlayerAttributesPersistenceSmoke();
    }
    catch (const std::exception& ex) {
        std::cerr << "fm_match_engine_smoke failed: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "fm_match_engine_smoke passed\n";
    return 0;
}
