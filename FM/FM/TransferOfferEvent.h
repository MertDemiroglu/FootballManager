#pragma once

#include<string>

#include"GameEvents.h"
#include"Types.h"

class Team;


class TransferOfferEvent : public GameEvents {
private:
	bool block = 1;
	EventPriority priority = EventPriority::High;
	EventTargetType type = EventTargetType::Team;
	std::string sellingTeamName;
	std::string buyingTeamName;
	std::string playerName;
	Money fee;
public:

	TransferOfferEvent(const std::string& sellingTeamName, const std::string& buyingTeamName, const std::string& playerName, Money f);

	bool isBlocking() const override;

	EventPriority getPriority() const override;

	void resolve(Game& game) override;

	EventTargetType getTargetType() const override;

	const std::string& getSendingTeam() const override;

	const std::string& getReceivingTeam() const override;

	const std::string& getPlayer() const override;

	bool affectsTeam(const Team* team) const override;

};