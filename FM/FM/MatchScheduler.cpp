#include "MatchScheduler.h"

#include <utility>

#include "EventQueue.h"
#include "Game.h"
#include "League.h"
#include "MatchEvent.h"

void MatchScheduler::scheduleMatch(int year, Month month, int day, std::string homeTeamName, std::string awayTeamName) {
    ScheduledMatch scheduledMatch{ year, month, day, std::move(homeTeamName), std::move(awayTeamName), false };
    matches.push_back(std::move(scheduledMatch));
}

void MatchScheduler::update(Game& game, EventQueue& queue) {
    const Date& currentDate = game.getDate();
    League& league = game.getLeague();

    for (auto& match : matches) {
        if (match.dispatched) {
            continue;
        }

        const bool isToday = currentDate.getYear() == match.year && currentDate.getMonth() == match.month&& currentDate.getDay() == match.day;

        if (!isToday) {
            continue;
        }

        Team* home = league.getTeam(match.homeTeamName);
        Team* away = league.getTeam(match.awayTeamName);

        if (home == nullptr || away == nullptr) {
            continue;
        }

        queue.pushEvent(std::make_unique<MatchEvent>(home, away));
        match.dispatched = true;
    }
}