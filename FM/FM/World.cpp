#include "World.h"

#include <stdexcept>

#include "DomainEventPublisher.h"
#include "Team.h"

World::World() : leagueContexts(), primaryLeagueId(std::nullopt), domainEventPublisher(), transferRoom(*this), transferOfferService(*this) {
}

LeagueContext& World::addLeagueContext(League league, LeagueRules rules, SeasonPlan seasonPlan) {
    const LeagueId leagueId = league.getId();
    auto [it, inserted] = leagueContexts.try_emplace(leagueId, std::move(league), std::move(rules), std::move(seasonPlan), domainEventPublisher);

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

DomainEventPublisher& World::getDomainEventPublisher() {
    return domainEventPublisher;
}

const DomainEventPublisher& World::getDomainEventPublisher() const {
    return domainEventPublisher;
}

bool World::hasPrimaryLeagueContext() const {
    return primaryLeagueId.has_value();
}

TransferRoom& World::getTransferRoom() {
    return transferRoom;
}

const TransferRoom& World::getTransferRoom() const {
    return transferRoom;
}

TransferOfferService& World::getTransferOfferService() {
    return transferOfferService;
}

const TransferOfferService& World::getTransferOfferService() const {
    return transferOfferService;
}

MatchId World::allocateMatchId() {
    return nextMatchId++;
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

void World::forEachLeagueContext(const std::function<void(LeagueContext&)>& visitor) {
    for (auto& [leagueId, context] : leagueContexts) {
        (void)leagueId;
        visitor(context);
    }
}

void World::forEachLeagueContext(const std::function<void(const LeagueContext&)>& visitor) const {
    for (const auto& [leagueId, context] : leagueContexts) {
        (void)leagueId;
        visitor(context);
    }
}