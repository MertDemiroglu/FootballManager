#include"TransferRoom.h"
#include"League.h"
#include"Team.h"
#include"Footballer.h"

#include<algorithm>
#include<iostream>
TransferRoom::TransferRoom(League& league) : league(league){}

void TransferRoom::addFreeAgent(std::unique_ptr<Footballer> player) {
	freeAgents.push_back(std::move(player));
}

void TransferRoom::listFreeAgents() const {
	for (const auto& p : freeAgents) {
		std::cout << *p << std::endl;
	}
}

bool TransferRoom::transferPlayer(const std::string& fromTeam, const std::string& toTeam, const std::string& playerName, Money fee) {
	Team* buyer = league.getTeam(toTeam);
	Team* seller = league.getTeam(fromTeam);

	if (!seller || !buyer) {
		return false;
	}
	if (!buyer->canAffordTransfer(fee)) {
		return false;
	}

	Footballer* player = seller->findPlayer(playerName);
	if (!player) { return false; }

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
	if (transferredPlayer && league.getCurrentSeasonYear() >= 0 && transferredPlayer->getCurrentSeasonStats().seasonYear != league.getCurrentSeasonYear()) {
		transferredPlayer->initializeSeasonStats(league.getCurrentSeasonYear());
	}
	return true;
}

bool TransferRoom::transferFreeAgent(const std::string& teamName, const std::string& playerName) {
	Team* team = league.getTeam(teamName);
	if (!team) {
		return false;
	}
		
	auto it = std::find_if(freeAgents.begin(), freeAgents.end(), [&](const std::unique_ptr<Footballer>& p) {return p->getName() == playerName; });

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
	if (signedPlayer && league.getCurrentSeasonYear() >= 0 && signedPlayer->getCurrentSeasonStats().seasonYear != league.getCurrentSeasonYear()) {
		signedPlayer->initializeSeasonStats(league.getCurrentSeasonYear());
	}

	return true;
}
bool TransferRoom::negotiateContract(Team* team, Footballer* player) {
	Money wage;
	int years;

	wage = Money(1000);
	years = 3;

	if (!team->canAffordWage(wage)) {
		return false;
	}
	player->setTeam(team->getName(), team->getId());
	player->signContract(wage, years);
	return true;
}
void TransferRoom::collectFreeAgentsFromTeams() {

	for (const auto& [name, teamPtr] : league.getTeams()) {
		auto expiredPlayers = teamPtr->collectExpiredContracts();

		for (auto& p : expiredPlayers) {
			p->setTeam("Free Agent", 0);
			addFreeAgent(std::move(p));
		}
	}

}

void TransferRoom::updatePlayersContractYearsInTeams() {
	for (const auto& [name, teamPtr] : league.getTeams()) {
		teamPtr->updateContracts();
	}
}

bool TransferRoom::isOpen() const {
	return transferOpen;
}
void TransferRoom::openWindow() {
	transferOpen = true;
}
void TransferRoom::closeWindow() {
	transferOpen = false;
}
