#include"MatchScheduler.h"
#include"EventQueue.h"
#include"Game.h"
#include"League.h"
#include"PlayMatchCommand.h"

void MatchScheduler::update(Game& game, EventQueue& queue) {
    const Date& currentDate = game.getDate();
    League& league = game.getLeague();

    const std::vector<FixtureMatch*> todaysMatches = league.getMatchesForDate(currentDate);

    for (FixtureMatch* match : todaysMatches) {
        if (match == nullptr) {
            continue;
        }
        Team* home = league.findTeamById(match->homeId);
        Team* away = league.findTeamById(match->awayId);

        if (home == nullptr || away == nullptr) {
            continue;   
        }

        queue.pushCommand(PlayMatchCommand{ league.getId(), league.getCurrentSeasonYear(), currentDate, home->getId(), away->getId(), match->matchweek});
        generatedMatchEvents++;
        match->eventEnqueued = true;
    }
}