#pragma once
#include<unordered_map>
#include<map>
#include<memory>
#include<optional>
#include<vector>
#include<string>

#include"Types.h"
#include"Team.h"
#include"Footballer.h"
#include"Date.h"
#include"LeagueRules.h"
#include"SeasonPlan.h"

struct FixtureMatch {
	bool played = false;
	bool eventEnqueued = false;
	std::string home;
	std::string away;
	FixtureMatch(const std::string& h,const std::string& a,bool p): played(p), home(h), away(a) {}
};

class League {
private:
	std::string name;
	std::unordered_map<std::string, std::unique_ptr<Team>> teams;
	std::map<Date, std::vector<FixtureMatch>> fixture;
	std::vector<std::optional<Date>> matchdayEndDates;
	bool seasonFixtureGenerated = false;
	
public:

	//League constructor
	explicit League(const std::string& leagueName);

	//Lige takim ekler (Pointer olarak)
	void addTeam(std::unique_ptr<Team> team);
	//Takimin pointerinin verir
	Team* getTeam(const std::string& teamName);
	//Takimin ismini alir, var ise true verir
	bool teamExists(const std::string& teamName) const;
	//Butun takimlari unordered map olarak verir
	const std::unordered_map<std::string, std::unique_ptr<Team>>& getTeams() const;

	//Fixture mac uretir
	void addFixtureMatch(int matchDayIndex, const Date& date, const std::string& home, const std::string& away);

	//Fixturu temizler
	void clearFixture();

	//Fixturu verir
	const std::map<Date, std::vector<FixtureMatch>>& getFixture() const;

	//O gunun maclarini vector olarak verir
	std::vector<FixtureMatch*> getMatchesForDate(const Date& date);

	std::optional<Date> tryGetMatchdayEndDate(int matchdayIndex) const;
	Date getLastFixtureDate() const;
	bool allMatchesPlayed() const;
	void initializeMatchdayTracking(int matchdaysPerSeason);

	bool isSeasonFixtureGenerated() const;
	void setSeasonFixtureGenerated(bool generated);
	void resetForNewSeason();


	//debug
	int debugFixtureDayCount() const;
	//debug
	int debugTotalFixtureMatches() const;
	//debug
	int debugMatchdayCount() const;
};