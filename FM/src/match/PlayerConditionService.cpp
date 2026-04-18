#include"fm/match/PlayerConditionService.h"

#include<stdexcept>

#include"fm/roster/Footballer.h"

namespace {
	constexpr int kDailyFitnessRecovery = 20;
}

void PlayerConditionService::applyMatchEffects(const MatchReport& report, League& league) const {
	if (report.leagueId != league.getId()) {
		throw std::logic_error("player condition update league mismatch");
	}

	for (const MatchPlayerReport& playerReport : report.playerReports) {
		if (playerReport.playerId == 0) {
			continue;
		}
		Footballer* player = league.findPlayerById(playerReport.playerId);
		if (!player) {
			throw std::logic_error("player condition update references unknown player");
		}

		PlayerConditionState& state = player->getConditionState();
		state.adjustFitness(fitnessDeltaFromMinutes(playerReport.minutesPlayed));
		state.adjustForm(formDeltaFromMatch(playerReport, report));
		state.adjustMorale(moraleDeltaFromMatch(playerReport, report));
	}
}

void PlayerConditionService::applyDailyRecovery(League& league) {
	for (auto& [teamId, team] : league.getTeams()) {
		(void)teamId;
		if (!team) {
			continue;
		}

		for (const auto& player : team->getPlayers()) {
			if (!player) {
				continue;
			}
			player->getConditionState().adjustFitness(kDailyFitnessRecovery);
		}
	}
}

int PlayerConditionService::fitnessDeltaFromMinutes(int minutesPlayed) {
	if (minutesPlayed < 0) {
		throw std::logic_error("match minutes cannot be negative for condition update");
	}
	//Daha sonra takimin temposuna gore hesaplanacak
	return minutesPlayed*(0.9);
}

int PlayerConditionService::formDeltaFromMatch(const MatchPlayerReport& playerReport, const MatchReport& report) {
	int delta = 0;

	if (playerReport.minutesPlayed > 0) {
		++delta;
	}
	else {
		--delta;
	}

	delta += (playerReport.goals * 2);
	delta += playerReport.assists;
	delta += teamResultSign(playerReport.teamId, report) * 2;

	return delta;
}

int PlayerConditionService::moraleDeltaFromMatch(const MatchPlayerReport& playerReport, const MatchReport& report) {
	int delta = teamResultSign(playerReport.teamId, report);

	if (playerReport.minutesPlayed > 0) {
		delta += 1;
	}

	return delta;
}

int PlayerConditionService::teamResultSign(TeamId teamId, const MatchReport& report) {
	if (teamId == report.homeId) {
		if (report.homeGoals > report.awayGoals) {
			return 1;
		}
		if (report.homeGoals < report.awayGoals) {
			return -1;
		}
		return 0;
	}

	if (teamId == report.awayId) {
		if (report.awayGoals > report.homeGoals) {
			return 1;
		}
		if (report.awayGoals < report.homeGoals) {
			return -1;
		}
		return 0;
	}

	throw std::logic_error("team id in player report does not belong to match report");
}