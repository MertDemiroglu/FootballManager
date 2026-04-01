#pragma once
#include<memory>
#include<string>
#include<functional>

#include"Date.h"
#include"EventQueue.h"
#include"User.h"
#include"MatchScheduler.h"
#include"FixtureGenerator.h"
#include"DomainEventPublisher.h"
#include"World.h"

class GameEvents;

enum class GameState {
	PreSeason, InSeason, PostSeason
};
class Game {
private:
	Date date;
	World world;
	GameState state;
	EventQueue eventsQueue;
	MatchScheduler matchScheduler;
	FixtureGenerator fixtureGenerator;
	DomainEventPublisher domainEventPublisher;
	User user;
	bool timePaused;
	bool dateWasReset;
	std::unique_ptr<GameEvents> currentBlockingEvent;

	void seasonStartChecksForContext(LeagueContext& context);
	void seasonEndChecksForContext(LeagueContext& context);
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
	//Aylik eventler
	void handleMonthlyEvents();
	//Haftalik eventler
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
	//lig nesnesini id ile aratip bulur non-const

	League* findLeagueById(LeagueId id);
	//lig nesnesini id ile aratip bulur const
	const League* findLeagueById(LeagueId id) const;
	//lig context nesnesini id ile aratip bulur non-const

	LeagueContext* findLeagueContextById(LeagueId id);
	//lig context nesnesini id ile aratip bulur const
	const LeagueContext* findLeagueContextById(LeagueId id) const;

	//lig contextleri dolasmak icin
	void forEachLeagueContext(const std::function<void(LeagueContext&)>& visitor);
	//lig contextleri dolasmak icin
	void forEachLeagueContext(const std::function<void(const LeagueContext&)>& visitor) const;


	//state'i verir
	GameState getState() const;

	//aktif lig kurallarini verir
	const LeagueRules& getRules() const;
	//aktif sezon planini verir
	const SeasonPlan& getSeasonPlan() const;

	//debug
	const MatchScheduler& getMatchScheduler() const;
};