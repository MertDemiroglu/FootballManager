#pragma once
#include"GameEvents.h"
#include"Date.h"
#include"Types.h"

class Team;

class MatchEvent : public GameEvents {
private:
	bool block = true;
	EventPriority priority = EventPriority::Critical;
	std::string homeName;
	std::string awayName;
	TeamId homeId;
	TeamId awayId;
	Date matchDate;
	EventTargetType type = EventTargetType::League;
public:
	MatchEvent(TeamId homeId, TeamId awayId, const Date& matchDate, std::string h, std::string a);

	bool isBlocking() const override;

	EventPriority getPriority() const override;

	void resolve(Game& game) override;

	EventTargetType getTargetType() const override;

	const std::string& getSendingTeam() const override;

	const std::string& getReceivingTeam() const override;
	//oyuncu yok o yuzden empty
	const std::string& getPlayer() const override;

	bool affectsTeam(const Team* team) const override;
};	