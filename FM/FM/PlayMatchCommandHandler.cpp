#include"PlayMatchCommandHandler.h"

#include<stdexcept>

#include"MatchSimulation.h"
#include"Team.h"

PlayMatchCommandHandler::PlayMatchCommandHandler(DomainEventPublisher& publisher)   : publisher(publisher) {}

void PlayMatchCommandHandler::handle(League& league, const PlayMatchCommand& command) {
	FixtureMatch* match = league.findFixtureMatch(command.date, command.homeId, command.awayId);

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

    const Team* homeTeam = league.findTeamById(command.homeId);
    const Team* awayTeam = league.findTeamById(command.awayId);

    if (!homeTeam || !awayTeam) {
        throw std::out_of_range("play command contains unknown team id");
    }

    const MatchReport report = MatchSimulation::buildStrengthBasedReport(
        command.leagueId,
        command.seasonYear,
        command.matchweek,
        *homeTeam,
        *awayTeam,
        command.date);

    league.applyMatchReport(report);

    publisher.publish(MatchPlayedEvent{
        report.leagueId,
        report.seasonYear,
        report.date,
        report.homeId,
        report.awayId,
        report.matchweek,
        report.homeGoals,
        report.awayGoals });
}