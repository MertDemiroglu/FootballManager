#pragma once
#include"GameEvents.h"
#include"Types.h"

class Team;
class Footballer;

class TransferOfferEvent : public GameEvents {
private:
	bool block = 1;
	EventPriority priority = EventPriority::High;
	EventTargetType type = EventTargetType::Team;
	Team* sellingTeam;
	Team* buyingTeam;
	Footballer* player;
	Money fee;
public:

	TransferOfferEvent(Team* s, Team* b, Footballer* p, Money f);

	bool isBlocking() const override;

	EventPriority getPriority() const override;

	void resolve(Game& game) override;

	EventTargetType getTargetType() const override;

	Team* getSendingTeam() const override;

	Team* getReceivingTeam() const override;

	Footballer* getPlayer() const override;

	bool affectsTeam(const Team* team) const override;

};