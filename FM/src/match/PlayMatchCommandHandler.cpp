#include"fm/match/PlayMatchCommandHandler.h"

#include<stdexcept>
#include<optional>

#include"fm/match_engine/MatchEngine.h"
#include"fm/match_engine/MatchEngineInputBuilder.h"
#include"fm/match/MatchSimulation.h"
#include"fm/match/TeamSelectionService.h"
#include"fm/match/TeamSheet.h"
#include"fm/roster/Team.h"
#include"fm/match/PlayerConditionService.h"

namespace {
    MatchReport buildLightweightReport(
        const PlayMatchCommand& command,
        const Team& homeTeam,
        const Team& awayTeam,
        const TeamSheet& homeSheet,
        const TeamSheet& awaySheet) {
        return MatchSimulation::buildStrengthBasedReport(
            command.matchId,
            command.leagueId,
            command.seasonYear,
            command.matchweek,
            homeTeam,
            awayTeam,
            homeSheet,
            awaySheet,
            command.date);
    }

    bool isSafeCoordinateReport(const MatchReport& report, const PlayMatchCommand& command) {
        return report.matchId == command.matchId
            && report.leagueId == command.leagueId
            && report.seasonYear == command.seasonYear
            && report.matchweek == command.matchweek
            && report.homeId == command.homeId
            && report.awayId == command.awayId;
    }

    std::optional<MatchReport> tryBuildCoordinatePrototypeReport(
        const PlayMatchCommand& command,
        const Team& homeTeam,
        const Team& awayTeam,
        const TeamSheet& homeSheet,
        const TeamSheet& awaySheet) {
        try {
            MatchEngineOptions engineOptions;
            engineOptions.detail = MatchSimulationDetail::BackgroundSummary;

            const MatchEngineInput input = MatchEngineInputBuilder{}.build(
                command.matchId,
                command.leagueId,
                command.seasonYear,
                command.matchweek,
                command.date,
                homeTeam,
                awayTeam,
                homeSheet,
                awaySheet,
                engineOptions);

            const MatchEngineResult result = MatchEngine{}.simulate(input);
            if (!result.report.has_value()) {
                return std::nullopt;
            }

            if (!isSafeCoordinateReport(*result.report, command)) {
                return std::nullopt;
            }

            return result.report;
        }
        catch (const std::exception&) {
            return std::nullopt;
        }
    }
}

PlayMatchCommandHandler::PlayMatchCommandHandler(
    DomainEventPublisher& publisher,
    PlayMatchCommandHandlerOptions options)
    : publisher(publisher), options(options) {}

void PlayMatchCommandHandler::handle(League& league, const PlayMatchCommand& command) {
    if (command.matchId == 0) {
        throw std::invalid_argument("play command match id cannot be zero");
    }
    FixtureMatch* match = league.findFixtureMatchById(command.matchId);

    if (!match) {
        throw std::runtime_error("fixture match not found for PlayMatchCommand");
    }

    if (match->played) {
        return;
    }

    if (command.leagueId != league.getId()) {
        throw std::logic_error("play command league id mismatch");
    }

    if (command.seasonYear != league.getCurrentSeasonYear()) {
        throw std::logic_error("play command season year mismatch");
    }

    if (match->matchweek != command.matchweek) {
        throw std::logic_error("play command matchweek mismatch");
    }

    if (match->homeId != command.homeId || match->awayId != command.awayId) {
        throw std::logic_error("play command team ids mismatch");
    }

    const Team* homeTeam = league.findTeamById(command.homeId);
    const Team* awayTeam = league.findTeamById(command.awayId);

    if (!homeTeam || !awayTeam) {
        throw std::out_of_range("play command contains unknown team id");
    }

    TeamSelectionService selectionService;
    const TeamSheet homeSheet = selectionService.buildTeamSheet(*homeTeam);
    const TeamSheet awaySheet = selectionService.buildTeamSheet(*awayTeam);
    handle(league, command, homeSheet, awaySheet);
}

void PlayMatchCommandHandler::handle(League& league,
    const PlayMatchCommand& command,
    const TeamSheet& homeSheet,
    const TeamSheet& awaySheet) {
    if (command.matchId == 0) {
        throw std::invalid_argument("play command match id cannot be zero");
    }
    FixtureMatch* match = league.findFixtureMatchById(command.matchId);
    if (!match) {
        throw std::runtime_error("fixture match not found for PlayMatchCommand");
    }
    if (match->played) {
        return;
    }
    if (command.leagueId != league.getId()) {
        throw std::logic_error("play command league id mismatch");
    }
    if (command.seasonYear != league.getCurrentSeasonYear()) {
        throw std::logic_error("play command season year mismatch");
    }
    if (match->matchweek != command.matchweek) {
        throw std::logic_error("play command matchweek mismatch");
    }
    if (match->homeId != command.homeId || match->awayId != command.awayId) {
        throw std::logic_error("play command team ids mismatch");
    }

    const Team* homeTeam = league.findTeamById(command.homeId);
    const Team* awayTeam = league.findTeamById(command.awayId);
    if (!homeTeam || !awayTeam) {
        throw std::out_of_range("play command contains unknown team id");
    }
    validateTeamSheetForTeam(homeSheet, *homeTeam);
    validateTeamSheetForTeam(awaySheet, *awayTeam);

    MatchReport report;
    if (options.engineMode == MatchSimulationEngineMode::CoordinatePrototype) {
        std::optional<MatchReport> coordinateReport =
            tryBuildCoordinatePrototypeReport(command, *homeTeam, *awayTeam, homeSheet, awaySheet);
        report = coordinateReport.has_value()
            ? *coordinateReport
            : buildLightweightReport(command, *homeTeam, *awayTeam, homeSheet, awaySheet);
    } else {
        report = buildLightweightReport(command, *homeTeam, *awayTeam, homeSheet, awaySheet);
    }

    league.applyMatchReport(report);

    PlayerConditionService conditionService;
    conditionService.applyMatchEffects(report, league);

    publisher.publish(MatchPlayedEvent{
        report.matchId,
        report.leagueId,
        report.seasonYear,
        report.date,
        report.homeId,
        report.awayId,
        report.matchweek,
        report.homeGoals,
        report.awayGoals });
}
