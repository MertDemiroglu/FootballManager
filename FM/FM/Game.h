#pragma once
#include<memory>
#include<string>

#include"Date.h"
#include"League.h"
#include"LeagueRules.h"
#include"TransferRoom.h"
#include"EventQueue.h"
#include"User.h"
#include"MatchScheduler.h"
#include"FixtureGenerator.h"
#include"SeasonPlan.h"

class GameEvents;

enum class GameState {
	PreSeason, InSeason, PostSeason
};
class Game {
private:
	Date date;
	League league;
	LeagueRules rules;
	SeasonPlan seasonPlan;
	TransferRoom transferRoom;
	GameState state;
	EventQueue eventsQueue;
	MatchScheduler matchScheduler;
	FixtureGenerator fixtureGenerator;
	User user;
	bool timePaused;
	bool dateWasReset;
	std::unique_ptr<GameEvents> currentBlockingEvent;
public:
	Game();
	~Game();

	void updateDaily();
	void updateState();

	void handleSeasonalEvents();
	void handleMonthlyEvents();
	void handleWeeklyEvents();

	void seasonStartChecks();
	void seasonEndChecks();
	void updateTransferWindow();

	TransferRoom& getTransferRoom();
	const TransferRoom& getTransferRoom() const;

	void stopTime();
	void processBlockingEvent();

	bool isTimePaused() const;

	void setUserTeam(const std::string& teamName);

	Date& getDate();
	const Date& getDate() const;

	League& getLeague();
	const League& getLeague() const;

	const MatchScheduler& getMatchScheduler() const;
	GameState getState() const;
};
