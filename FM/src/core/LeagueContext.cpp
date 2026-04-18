#include"fm/core/LeagueContext.h"

LeagueContext::LeagueContext(League league, LeagueRules rules, SeasonPlan seasonPlan, DomainEventPublisher& publisher)
    : league(std::move(league)), rules(std::move(rules)), seasonPlan(std::move(seasonPlan)), playMatchCommandHandler(publisher) {
}

League& LeagueContext::getLeague() {
    return league;
}

const League& LeagueContext::getLeague() const {
    return league;
}

LeagueRules& LeagueContext::getRules() {
    return rules;
}

const LeagueRules& LeagueContext::getRules() const {
    return rules;
}

SeasonPlan& LeagueContext::getSeasonPlan() {
    return seasonPlan;
}

const SeasonPlan& LeagueContext::getSeasonPlan() const {
    return seasonPlan;
}

PlayMatchCommandHandler& LeagueContext::getPlayMatchCommandHandler() {
    return playMatchCommandHandler;
}

const PlayMatchCommandHandler& LeagueContext::getPlayMatchCommandHandler() const {
    return playMatchCommandHandler;
}

int LeagueContext::getLastSeasonRolloverYear() const {
    return lastSeasonRolloverYear;
}

void LeagueContext::setLastSeasonRolloverYear(int year) {
    lastSeasonRolloverYear = year;
}
