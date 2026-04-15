#include"PlayMatchCommandHandler.h"

#include<stdexcept>

#include"MatchSimulation.h"
#include "TeamSelectionService.h"
#include "TeamSheet.h"
#include"Team.h"

PlayMatchCommandHandler::PlayMatchCommandHandler(DomainEventPublisher& publisher)   : publisher(publisher) {}

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

    validateTeamSheetForTeam(homeSheet, *homeTeam);
    validateTeamSheetForTeam(awaySheet, *awayTeam);

    const MatchReport report = MatchSimulation::buildStrengthBasedReport(
        command.matchId,
        command.leagueId,
        command.seasonYear,
        command.matchweek,
        *homeTeam,
        *awayTeam,
        homeSheet,
        awaySheet,
        command.date);

    league.applyMatchReport(report);

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
