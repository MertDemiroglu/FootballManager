#pragma once
#include<memory>
#include<string>
#include<functional>
#include<optional>

#include"Date.h"
#include"EventQueue.h"
#include"User.h"
#include"MatchScheduler.h"
#include"FixtureGenerator.h"
#include"World.h"
#include"InteractionManager.h"

enum class GameState {
	PreSeason, InSeason, PostSeason
};

class GameInteraction;
class PostMatchInteraction;
class PreMatchInteraction;
class TransferOfferDecisionInteraction;
class Team;

class Game {
private:
	Date date;
	World world;
	GameState state;
	EventQueue eventsQueue;
	MatchScheduler matchScheduler;
	FixtureGenerator fixtureGenerator;
	InteractionManager interactionManager;
	User user;
	bool timePaused;
	bool userPaused;
	bool dateWasReset;
	std::optional<PlayMatchCommand> pendingPreMatchCommand;

	//debug icin---------------------------------------------------------
	int lastDebugOfferYear = -1;
	int lastDebugOfferMonth = -1;
	std::size_t debugOfferPlayerCursor = 0;
	PlayerId pickNextDebugOfferCandidatePlayerId(const Team& sellerTeam);
	//-------------------------------------------------------------------

	void seasonStartChecksForContext(LeagueContext& context);
	void seasonEndChecksForContext(LeagueContext& context);
	void refreshTimePauseState();

	void temporaryForDebug_tryCreateWeeklyManagedTransferOffer();
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


	//Blocking event var ise zamani durdurur
	void stopTime();
	//Oyuncunun karar vermesi gereken eventin islendigi fonksiyon
	void processBlockingEvent();

	bool pauseSimulation();
	bool resumeSimulation();


	//zamanin akmasini kontrol eder (true/false)
	bool isTimePaused() const;

	bool hasActiveBlockingInteraction() const;
	const GameInteraction* getActiveInteraction() const;
	const PreMatchInteraction* getActivePreMatchInteraction() const;
	const PostMatchInteraction* getActivePostMatchInteraction() const;
	const TransferOfferDecisionInteraction* getActiveTransferOfferDecisionInteraction() const;
	bool resolveActiveInteraction();
	bool playPendingPreMatch();

	bool acceptTransferOffer(OfferId offerId);
	bool rejectTransferOffer(OfferId offerId);

	//Kullanici takimini disaridan set eder
	void setUserTeam(LeagueId leagueId, TeamId teamId);

	//tarih nesnesini verir non-const
	Date& getDate();
	//tarihi verir const
	const Date& getDate() const;

	

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

	//debug
	const MatchScheduler& getMatchScheduler() const;
};
