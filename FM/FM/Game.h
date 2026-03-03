#pragma once
#include<memory>
#include<string>

#include"Date.h"
#include"League.h"
#include"TransferRoom.h"
#include"EventQueue.h"
#include"User.h"
#include"MatchScheduler.h"
#include"FixtureGenerator.h"

class GameEvents;

enum class GameState {
	PreSeason, InSeason, PostSeason
};
class Game {
private:
	Date date;
	League league;
	TransferRoom transferRoom;
	GameState state;
	EventQueue eventsQueue;
	MatchScheduler matchScheduler;
	FixtureGenerator fixtureGenerator;
	User user;
	bool timePaused;
	std::unique_ptr<GameEvents> currentBlockingEvent;
public:
	//Game constructor
	Game();
	~Game();

	//Gun ilerletir ve event kontrol eder
	void updateDaily();
	//Pre season / In season / Post season kontrolu yapar
	void updateState();


	//Pre/In/Post season eventler
	void handleSeasonalEvents();
	//Aylýk eventler
	void handleMonthlyEvents();
	//Haftalýk eventler
	void handleWeeklyEvents();

	//sezon baslarken rutin kontroller
	void seasonStartChecks();
	//sezon sonu rutin kontroller
	void seasonEndChecks();
	//transfer penceresi kontrolu
	void updateTransferWindow();


	//Transfer room nesnesini verir non-const
	TransferRoom& getTransferRoom();
	//Transfer room nesnesini verir const
	const TransferRoom& getTransferRoom() const;

	//Blocking event var ise zamani durdurur
	void stopTime();
	//Oyuncunun karar vermesi gereken eventin islendigi fonksiyon
	void processBlockingEvent();

	//zamanin akmasini kontrol eder (true/false)
	bool isTimePaused() const;

	//Kullanici takimini disaridan set eder
	void setUserTeam(const std::string& teamName);

	//tarih nesnesini verir non-const
	Date& getDate();
	//tarihi verir const
	const Date& getDate() const;

	//lig nesnesini verir non-const
	League& getLeague();
	//lig nesnesini verir const
	const League& getLeague() const;

	//debug
	const MatchScheduler& getMatchScheduler() const;
};