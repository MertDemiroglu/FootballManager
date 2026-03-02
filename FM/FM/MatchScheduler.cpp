#include "MatchScheduler.h"

#include "EventQueue.h"
#include "Game.h"
#include "League.h"
#include "MatchEvent.h"

void MatchScheduler::update(Game& game, EventQueue& queue) {
    const Date& currentDate = game.getDate();
    League& league = game.getLeague();

    const std::vector<FixtureMatch*> todaysMatches = league.getMatchesForDate(currentDate);

    for (FixtureMatch* match : todaysMatches) {
        if (match == nullptr) {
            continue;
        }
        Team* home = league.getTeam(match->home);
        Team* away = league.getTeam(match->away);

        if (home == nullptr || away == nullptr) {
            continue;   
        }

        queue.pushEvent(std::make_unique<MatchEvent>(home, away));  
        generatedMatchEvents++;//debug
        match->played = true;
    }
}