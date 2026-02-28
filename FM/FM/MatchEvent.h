#pragma once
#include"GameEvents.h"

class Team;

class MatchEvent : public GameEvents {
private:
	bool block = true;
	EventPriority priority = EventPriority::Critical;
	Team* home;
	Team* away;
	EventTargetType type = EventTargetType::League;
public:
	MatchEvent(Team* h, Team* a);

	bool isBlocking() const override;

	EventPriority getPriority() const override;

	void resolve(Game& game) override;

	EventTargetType getTargetType() const override;

	Team* getSendingTeam() const override;

	Team* getReceivingTeam() const override;

	bool affectsTeam(const Team* team) const override;
};