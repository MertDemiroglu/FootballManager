#include "MatchScheduler.h"

#include "EventQueue.h"
#include "Game.h"
#include "League.h"
#include "MatchEvent.h"

void MatchScheduler::update(Game& game, EventQueue& queue) {
    const Date& currentDate = game.getDate();
    League& league = game.getLeague();

    const std::vector<FixtureMatch> todaysMatches = league.getMatchesForDate(currentDate);

    for (const auto& match : todaysMatches) {
        Team* home = league.getTeam(match.homeTeamName);
        Team* away = league.getTeam(match.awayTeamName);

        if (home == nullptr || away == nullptr) {
            continue;
        }

        queue.pushEvent(std::make_unique<MatchEvent>(home, away));
    }
}