#include"fm/transfer/TransferRoom.h"

#include"fm/roster/Footballer.h"
#include"fm/competition/League.h"
#include"fm/core/LeagueContext.h"
#include"fm/roster/Team.h"
#include"fm/core/World.h"

#include<algorithm>
#include<iostream>
#include<stdexcept>
#include<utility>
	
TransferRoom::TransferRoom() = default;

TransferRoom::TransferRoom(World& world) : world(&world) {}

void TransferRoom::bindWorld(World& worldRef) {
	world = &worldRef;
}

void TransferRoom::setRuntimeMutationCallback(std::function<void()> callback) {
	runtimeMutationCallback = std::move(callback);
}

void TransferRoom::notifyRuntimeMutation() const {
	if (runtimeMutationCallback) {
		runtimeMutationCallback();
	}
}

void TransferRoom::addFreeAgent(std::unique_ptr<Footballer> player) {
	if (!player) {
		return;
	}
	const PlayerId playerId = player->getId();
	if (playerId == 0) {
		throw std::invalid_argument("free agent player id cannot be zero");
	}
	const auto duplicate = std::find_if(freeAgents.begin(), freeAgents.end(), [&](const std::unique_ptr<Footballer>& existing) {
		return existing && existing->getId() == playerId;
	});
	if (duplicate != freeAgents.end()) {
		throw std::logic_error("duplicate free agent player id");
	}
	player->setTeam(0);
	freeAgents.push_back(std::move(player));
}

const std::vector<std::unique_ptr<Footballer>>& TransferRoom::getFreeAgents() const {
	return freeAgents;
}

void TransferRoom::clearFreeAgents() {
	freeAgents.clear();
}

void TransferRoom::restoreFreeAgents(std::vector<std::unique_ptr<Footballer>> players) {
	clearFreeAgents();
	for (auto& player : players) {
		addFreeAgent(std::move(player));
	}
}

void TransferRoom::listFreeAgents() const {
	for (const auto& p : freeAgents) {
		std::cout << *p << std::endl;
	}
}

bool TransferRoom::transferPlayer(LeagueId fromLeagueId, TeamId fromTeamId, LeagueId toLeagueId, TeamId toTeamId, PlayerId playerId, Money fee) {
	if (!world) {
		return false;
	}

	League* sellerLeague = world->findLeagueById(fromLeagueId);
	League* buyerLeague = world->findLeagueById(toLeagueId);

	if (!sellerLeague || !buyerLeague) {
		return false;
	}

	if (!isOpenForLeague(fromLeagueId) || !isOpenForLeague(toLeagueId)) {
		return false;
	}

	Team* seller = sellerLeague->findTeamById(fromTeamId);
	Team* buyer = buyerLeague->findTeamById(toTeamId);

	if (!seller || !buyer) {
		return false;
	}
	if (!buyer->canAffordTransfer(fee)) {
		return false;
	}
	if (buyer->findPlayerById(playerId)) {
		return false;
	}

	Footballer* player = seller->findPlayerById(playerId);
	if (!player) {
		return false;
	}

	constexpr Money negotiatedWage = Money(1000);
	constexpr int negotiatedYears = 3;
	if (!buyer->canAffordWage(negotiatedWage)) {
		return false;
	}

	auto soldPlayer = seller->releasePlayer(playerId);
	if (!soldPlayer) {
		return false;
	}

	buyer->spendTransferFee(fee);
	seller->receiveTransferFee(fee);
	soldPlayer->setTeam(buyer->getId());
	soldPlayer->signContract(negotiatedWage, negotiatedYears);
	buyer->addPlayer(std::move(soldPlayer));

	Footballer* transferredPlayer = buyer->findPlayerById(playerId);
	if (transferredPlayer && buyerLeague && buyerLeague->getCurrentSeasonYear() >= 0 &&	transferredPlayer->getCurrentSeasonStats().seasonYear != buyerLeague->getCurrentSeasonYear()) {
		transferredPlayer->initializeSeasonStats(buyerLeague->getCurrentSeasonYear());
	}
	return true;
}

bool TransferRoom::transferFreeAgent(LeagueId toLeagueId, TeamId toTeamId, PlayerId playerId) {
	if (!world) {
		return false;
	}

	League* buyerLeague = world->findLeagueById(toLeagueId);
	if (!buyerLeague) {
		return false;
	}
	Team* team = buyerLeague->findTeamById(toTeamId);
	if (!team) {
		return false;
	}
	constexpr Money negotiatedWage = Money(1000);
	if (!team->canAffordWage(negotiatedWage)) {
		return false;
	}
	if (team->findPlayerById(playerId)) {
		return false;
	}

	auto it = std::find_if(freeAgents.begin(), freeAgents.end(), [&](const std::unique_ptr<Footballer>& p) { return p->getId() == playerId; });
	
	if (it == freeAgents.end()) {
		return false;
	}

	auto player = it->get();
	player->setTeam(team->getId());
	if (!negotiateContract(team, player)) {
		player->setTeam(0);
		return false;
	}

	std::unique_ptr<Footballer> movingPlayer = std::move(*it);
	freeAgents.erase(it);

	team->addPlayer(std::move(movingPlayer));

	Footballer* signedPlayer = team->findPlayerById(playerId);
	if (signedPlayer && buyerLeague && buyerLeague->getCurrentSeasonYear() >= 0 &&signedPlayer->getCurrentSeasonStats().seasonYear != buyerLeague->getCurrentSeasonYear()) {
		signedPlayer->initializeSeasonStats(buyerLeague->getCurrentSeasonYear());
	}

	notifyRuntimeMutation();
	return true;
}
bool TransferRoom::negotiateContract(Team* team, Footballer* player) {
	Money wage = Money(1000);
	int years = 3;

	if (!team->canAffordWage(wage)) {
		return false;
	}
	player->setTeam(team->getId());
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

	bool movedAnyPlayer = false;
	for (const auto& [teamId, teamPtr] : context->getLeague().getTeams()) {
		(void)teamId;
		auto expiredPlayers = teamPtr->collectExpiredContracts();

		for (auto& p : expiredPlayers) {
			p->setTeam(0);
			p->clearContract();
			addFreeAgent(std::move(p));
			movedAnyPlayer = true;
		}
	}
	if (movedAnyPlayer) {
		notifyRuntimeMutation();
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

	for (const auto& [teamId, teamPtr] : context->getLeague().getTeams()) {
		(void)teamId;
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
