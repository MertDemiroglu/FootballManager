#pragma once

#include <string>
#include <vector>

#include "Date.h"

class Game;
class EventQueue;

class MatchScheduler {
public:
    struct ScheduledMatch {
        int year;
        Month month;
        int day;
        std::string homeTeamName;
        std::string awayTeamName;
        bool dispatched = false;
    };

    void scheduleMatch(int year, Month month, int day, std::string homeTeamName, std::string awayTeamName);

    void update(Game& game, EventQueue& queue);

private:
    std::vector<ScheduledMatch> matches;
};