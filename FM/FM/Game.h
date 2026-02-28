#pragma once
#include<memory>

#include"Date.h"
#include"League.h"
#include"TransferRoom.h"
#include"EventQueue.h"
#include"User.h"

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
	User user;
	bool timePaused;
	std::unique_ptr<GameEvents> currentBlockingEvent;
public:
	//Game constructor
	Game();

	//Gün ilerletir event kontrol eder
	void updateDaily();

	//Pre season / In season / Post season kontrolü yapar
	void updateState();


	//Pre/In/Post season eventler
	void handleSeasonalEvents();
	//Aylýk eventler
	void handleMonthlyEvents();
	//Haftalýk eventler
	void handleWeeklyEvents();

	//sezon baþý rutin kontroller
	void seasonStartChecks();
	//sezon sonu rutin kontroller
	void seasonEndChecks();
	//transfer penceresi kontrolü
	void updateTransferWindow();

	TransferRoom& getTransferRoom();

	const TransferRoom& getTransferRoom() const;
	//Blocking event var ise zamani durdurur
	void stopTime();
	//Oyuncunun karar vermesi gereken eventin islendigi fonksiyon
	void processBlockingEvent();

	bool isTimePaused() const;
};