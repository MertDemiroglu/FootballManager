#include"TransferRoom.h"
#include"Footballer.h"
#include"League.h"
#include"LeagueContext.h"
#include"Team.h"
#include"World.h"

#include<algorithm>
#include<iostream>
	
TransferRoom::TransferRoom() = default;

TransferRoom::TransferRoom(World& world) : world(&world) {}

void TransferRoom::bindWorld(World& worldRef) {
	world = &worldRef;
}

void TransferRoom::addFreeAgent(std::unique_ptr<Footballer> player) {
	freeAgents.push_back(std::move(player));
}

void TransferRoom::listFreeAgents() const {
	for (const auto& p : freeAgents) {
		std::cout << *p << std::endl;
	}
}

bool TransferRoom::transferPlayer(const std::string& fromTeam, const std::string& toTeam, const std::string& playerName, Money fee) {
	if (!world) {
		return false;
	}

	LeagueId sellerLeagueId = 0;
	LeagueId buyerLeagueId = 0;
	Team* seller = world->findTeamByName(fromTeam, &sellerLeagueId);
	Team* buyer = world->findTeamByName(toTeam, &buyerLeagueId);

	if (!seller || !buyer) {
		return false;
	}
	if (!buyer->canAffordTransfer(fee)) {
		return false;
	}

	Footballer* player = seller->findPlayer(playerName);
	if (!player) {
		return false;
	}

	const PlayerId playerId = player->getId();

	if (!negotiateContract(buyer, player)) {
		return false;
	}

	buyer->spend(fee);
	seller->earn(fee);

	auto soldPlayer = seller->releasePlayer(playerId);
	if (!soldPlayer) {
		return false;
	}

	buyer->addPlayer(std::move(soldPlayer));
	Footballer* transferredPlayer = buyer->findPlayer(playerName);
	League* buyerLeague = world->findLeagueById(buyerLeagueId);
	if (transferredPlayer && buyerLeague && buyerLeague->getCurrentSeasonYear() >= 0 &&	transferredPlayer->getCurrentSeasonStats().seasonYear != buyerLeague->getCurrentSeasonYear()) {
		transferredPlayer->initializeSeasonStats(buyerLeague->getCurrentSeasonYear());
	}
	return true;
}

bool TransferRoom::transferFreeAgent(const std::string& teamName, const std::string& playerName) {
	if (!world) {
		return false;
	}

	LeagueId buyerLeagueId = 0;
	Team* team = world->findTeamByName(teamName, &buyerLeagueId);
	if (!team) {
		return false;
	}

	auto it = std::find_if(freeAgents.begin(), freeAgents.end(), [&](const std::unique_ptr<Footballer>& p) { return p->getName() == playerName; });
	
	if (it == freeAgents.end()) {
		return false;
	}

	auto player = it->get();
	player->setTeam(team->getName(), team->getId());
	negotiateContract(team, player);

	std::unique_ptr<Footballer> movingPlayer = std::move(*it);
	freeAgents.erase(it);

	team->addPlayer(std::move(movingPlayer));

	Footballer* signedPlayer = team->findPlayer(playerName);
	League* buyerLeague = world->findLeagueById(buyerLeagueId);
	if (signedPlayer && buyerLeague && buyerLeague->getCurrentSeasonYear() >= 0 &&signedPlayer->getCurrentSeasonStats().seasonYear != buyerLeague->getCurrentSeasonYear()) {
		signedPlayer->initializeSeasonStats(buyerLeague->getCurrentSeasonYear());
	}

	return true;
}
bool TransferRoom::negotiateContract(Team* team, Footballer* player) {
	Money wage = Money(1000);
	int years = 3;

	if (!team->canAffordWage(wage)) {
		return false;
	}
	player->setTeam(team->getName(), team->getId());
	player->signContract(wage, years);
	return true;
}
void TransferRoom::collectFreeAgentsFromTeams() {
	if (!world) {
		return;
	}
	world->forEachLeagueContext([this](LeagueContext& context) {
		collectFreeAgentsFromLeague(context.getLeague().getId());
	});
}

void TransferRoom::collectFreeAgentsFromLeague(LeagueId leagueId) {
	if (!world) {
		return;
	}

	LeagueContext* context = world->findLeagueContext(leagueId);
	if (!context) {
		return;
	}

	for (const auto& [name, teamPtr] : context->getLeague().getTeams()) {
		(void)name;
		auto expiredPlayers = teamPtr->collectExpiredContracts();

		for (auto& p : expiredPlayers) {
			p->setTeam("Free Agent", 0);
			addFreeAgent(std::move(p));
		}
	}
}

void TransferRoom::updatePlayersContractYearsInTeams() {
	if (!world) {
		return;
	}
	world->forEachLeagueContext([this](LeagueContext& context) {
		updatePlayersContractYearsInLeague(context.getLeague().getId());
		});
}

void TransferRoom::updatePlayersContractYearsInLeague(LeagueId leagueId) {
	if (!world) {
		return;
	}

	LeagueContext* context = world->findLeagueContext(leagueId);
	if (!context) {
		return;
	}

	for (const auto& [name, teamPtr] : context->getLeague().getTeams()) {
		(void)name;
		teamPtr->updateContracts();
	}
}

bool TransferRoom::isOpen() const {
	if (!world || !world->hasPrimaryLeagueContext()) {
		return false;
	}
	const LeagueId primaryId = world->getPrimaryLeagueContext().getLeague().getId();
	return isOpenForLeague(primaryId);
}

bool TransferRoom::isOpenForLeague(LeagueId leagueId) const {
	const auto it = transferWindowByLeagueId.find(leagueId);
	if (it == transferWindowByLeagueId.end()) {
		return false;
	}
	return it->second;
}

void TransferRoom::openWindow() {
	if (!world || !world->hasPrimaryLeagueContext()) {
		return;
	}
	openWindowForLeague(world->getPrimaryLeagueContext().getLeague().getId());
}

void TransferRoom::openWindowForLeague(LeagueId leagueId) {
	transferWindowByLeagueId[leagueId] = true;
}

void TransferRoom::closeWindow() {
	if (!world || !world->hasPrimaryLeagueContext()) {
		return;
	}
	closeWindowForLeague(world->getPrimaryLeagueContext().getLeague().getId());
}

void TransferRoom::closeWindowForLeague(LeagueId leagueId) {
	transferWindowByLeagueId[leagueId] = false;
}