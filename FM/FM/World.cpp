#include "World.h"

#include <stdexcept>
#include <tuple>

#include "DomainEventPublisher.h"

LeagueContext& World::addLeagueContext(League league,
    LeagueRules rules,
    SeasonPlan seasonPlan,
    DomainEventPublisher& publisher) {
    const LeagueId leagueId = league.getId();
    auto [it, inserted] = leagueContexts.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(leagueId),
        std::forward_as_tuple(std::move(league), std::move(rules), std::move(seasonPlan), publisher));

    if (!inserted) {
        throw std::logic_error("league context already exists");
    }

    if (!primaryLeagueId.has_value()) {
        primaryLeagueId = leagueId;
    }

    return it->second;
}

LeagueContext* World::findLeagueContext(LeagueId leagueId) {
    const auto it = leagueContexts.find(leagueId);
    if (it == leagueContexts.end()) {
        return nullptr;
    }
    return &it->second;
}

const LeagueContext* World::findLeagueContext(LeagueId leagueId) const {
    const auto it = leagueContexts.find(leagueId);
    if (it == leagueContexts.end()) {
        return nullptr;
    }
    return &it->second;
}

League* World::findLeagueById(LeagueId leagueId) {
    LeagueContext* context = findLeagueContext(leagueId);
    return context ? &context->getLeague() : nullptr;
}

const League* World::findLeagueById(LeagueId leagueId) const {
    const LeagueContext* context = findLeagueContext(leagueId);
    return context ? &context->getLeague() : nullptr;
}

bool World::hasPrimaryLeagueContext() const {
    return primaryLeagueId.has_value();
}

LeagueContext& World::getPrimaryLeagueContext() {
    if (!primaryLeagueId.has_value()) {
        throw std::logic_error("world has no primary league context");
    }
    LeagueContext* context = findLeagueContext(*primaryLeagueId);
    if (!context) {
        throw std::logic_error("primary league context is missing");
    }
    return *context;
}

const LeagueContext& World::getPrimaryLeagueContext() const {
    if (!primaryLeagueId.has_value()) {
        throw std::logic_error("world has no primary league context");
    }
    const LeagueContext* context = findLeagueContext(*primaryLeagueId);
    if (!context) {
        throw std::logic_error("primary league context is missing");
    }
    return *context;
}