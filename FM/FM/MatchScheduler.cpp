#include"MatchScheduler.h"
#include"EventQueue.h"
#include"World.h"
#include"League.h"
#include"PlayMatchCommand.h"

#include <stdexcept>

void MatchScheduler::update(World& world, const Date& currentDate, EventQueue& queue) {
    world.forEachLeagueContext([&](LeagueContext& context) {
        League& league = context.getLeague();
        const std::vector<FixtureMatch*> todaysMatch = league.getMatchesForDate(currentDate);


        for (FixtureMatch* match : todaysMatch) {

            if (match == nullptr) {
                throw std::logic_error("fixture match pointer cannot be null");
            }

            if (match->played || match->eventEnqueued) {
                throw std::logic_error("match is already played or enqueued");
            }
            Team* home = league.findTeamById(match->homeId);
            Team* away = league.findTeamById(match->awayId);

            if (home == nullptr || away == nullptr) {
                throw std::logic_error("fixture contains unknown team id while scheduling match");
            }

            queue.pushCommand(PlayMatchCommand{ league.getId(), league.getCurrentSeasonYear(), currentDate, home->getId(), away->getId(), match->matchweek });
            generatedMatchEvents++;
            match->eventEnqueued = true;

        }
    });     
}