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
	TeamId homeId;
	TeamId awayId;
	FixtureMatch() = default;
	FixtureMatch(TeamId h,const TeamId a, bool p = false): played(p), homeId(h), awayId(a) {}
	int homeGoals = -1;
	int awayGoals = -1;
};

struct MatchResult {
	int homeGoals = 0;
	int awayGoals = 0;
};

struct StandingsEntry {
	TeamId teamId;
	int played = 0;
	int wins = 0;
	int draws = 0;
	int losses = 0;
	int goalsFor = 0;
	int goalsAgainst = 0;
	int goalDifference = 0;
	int points = 0;
};

class League {
private:
	std::string name;
	std::unordered_map<std::string, std::unique_ptr<Team>> teams;
	std::unordered_map<TeamId, Team*> teamIndexById;
	std::map<Date, std::vector<FixtureMatch>> fixture;
	std::vector<std::optional<Date>> matchdayEndDates;
	std::unordered_map<TeamId, StandingsEntry> standings;
	bool seasonFixtureGenerated = false;
	
public:

	//League constructor
	explicit League(const std::string& leagueName);

	//Lige takim ekler (Pointer olarak)
	void addTeam(std::unique_ptr<Team> team);
	//Takimin pointerini verir
	Team* getTeam(const std::string& teamName);
	//Takimin pointerini verir overloaded
	const Team* getTeam(const std::string& teamName) const;
	//Takimin ismini alir, var ise true verir
	bool teamExists(const std::string& teamName) const;
	//Butun takimlari unordered map olarak verir
	const std::unordered_map<std::string, std::unique_ptr<Team>>& getTeams() const;

	//Takimi ID'sinden bulup poninterini verir
	Team* findTeamById(TeamId id);
	//Takimi ID'sinden bulup poninterini verir overloaded
	const Team* findTeamById(TeamId id) const;
	//Takim adinin verir
	const std::string& getTeamName(TeamId id) const;
	//Takimin olup olmadigini kontrol eder ID ile
	bool hasTeam(TeamId id) const;

	//Fixture mac uretir
	void addFixtureMatch(int matchDayIndex, const Date& date, TeamId homeId, TeamId awayId);

	//Fixturu temizler
	void clearFixture();

	//Fixturu verir
	const std::map<Date, std::vector<FixtureMatch>>& getFixture() const;

	//O gunun maclarini vector olarak verir
	std::vector<FixtureMatch*> getMatchesForDate(const Date& date);

	//Maci araryip bulur
	FixtureMatch* findFixtureMatch(const Date& date, TeamId homeId, TeamId awayId);
	//Maci arayip bulur overloaded
	const FixtureMatch* findFixtureMatch(const Date& date, TeamId homeId, TeamId awayId) const;

	void initializeStandings();
	void resetStandings();
	void updateStandingsForMatch(TeamId homeId, TeamId awayId, const MatchResult& result);
	void applyMatchResult(const Date& date, TeamId homeId, TeamId awayId, const MatchResult& result);
	std::vector<StandingsEntry> getSortedStandings() const;
	const std::unordered_map<TeamId, StandingsEntry>& getStandings() const;

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