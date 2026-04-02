#pragma once
#include<unordered_map>
#include<map>
#include<memory>
#include<optional>
#include<vector>
#include<string>

#include"Types.h"
#include"Team.h"
#include"Date.h"
#include"MatchPlayedEvent.h"

struct FixtureMatch {

	bool played = false;
	bool eventEnqueued = false;

	TeamId homeId;
	TeamId awayId;
	int matchweek = 0;

	FixtureMatch() = default;
	FixtureMatch(TeamId h,const TeamId a, int mw): homeId(h), awayId(a), matchweek(mw) {}

	int homeGoals = -1;
	int awayGoals = -1;
};

struct FixtureMatchPreview {
	Date date{ 1900, Month::January, 1 };
	TeamId homeId = 0;
	TeamId awayId = 0;
	int matchweek = 0;
};

struct MatchResult {
	int homeGoals = 0;
	int awayGoals = 0;
};

struct MatchRecord {
	Date date{ 1900, Month::January, 1 };
	int seasonYear = 0;
	TeamId homeId = 0;
	TeamId awayId = 0;
	int homeGoals = 0;
	int awayGoals = 0;
	int matchweek = 0;
};

enum class MatchOutcome {
	Win,
	Draw,
	Loss
};

struct StandingsEntry {
	TeamId teamId = 0;
	int played = 0;
	int wins = 0;
	int draws = 0;
	int losses = 0;
	int goalsFor = 0;
	int goalsAgainst = 0;
	int goalDifference = 0;
	int points = 0;
};

struct TeamSeasonStats {
	TeamId teamId = 0;
	int seasonYear = 0;

	int played = 0;
	int homePlayed = 0;
	int awayPlayed = 0;

	int wins = 0;
	int draws = 0;
	int losses = 0;

	int homeWins = 0;
	int homeDraws = 0;
	int homeLosses = 0;

	int awayWins = 0;
	int awayDraws = 0;
	int awayLosses = 0;

	int goalsFor = 0;
	int goalsAgainst = 0;

	int homeGoalsFor = 0;
	int homeGoalsAgainst = 0;

	int awayGoalsFor = 0;
	int awayGoalsAgainst = 0;

	int points = 0;
	int homePoints = 0;
	int awayPoints = 0;

	int cleanSheets = 0;
	int failedToScore = 0;
};

class League {
private:
	//Lig ismi
	std::string name;
	//Lig id'si
	LeagueId id = 0;
	//Takimlari tutan map
	std::unordered_map<TeamId, std::unique_ptr<Team>> teams;
	std::unordered_map<std::string, TeamId> teamNameToId;

	std::map<Date, std::vector<FixtureMatch>> fixture;
	std::vector<std::optional<Date>> matchWeekEndDates;
	std::unordered_map<TeamId, StandingsEntry> standings;

	std::vector<MatchRecord> currentSeasonHistory;
	std::unordered_map<int, std::vector<MatchRecord>> archivedHistoryBySeason;

	//O sezondaki takim istatistiklerini tutan map
	std::unordered_map<TeamId, TeamSeasonStats> currentTeamSeasonStats;
	//Gecmis tum sezonlarin istatistiklerini tutan map
	std::unordered_map<int, std::unordered_map<TeamId, TeamSeasonStats>> archivedTeamSeasonStatsBySeason;

	bool seasonFixtureGenerated = false;
	int currentSeasonYear = -1;

	std::vector<MatchRecord>filterMatchesForTeam(const std::vector<MatchRecord>& records, TeamId teamId) const;
	std::vector<MatchRecord>getAllMatchesForTeam(TeamId teamId) const;
	static MatchOutcome toOutcome(const MatchRecord& record, TeamId teamId);
	static bool recordBelongsToTeam(const MatchRecord& record, TeamId teamId);
	bool hasCurrentSeasonHistoryRecord(const Date& date, TeamId homeId, TeamId awayId) const;

	//Bu 6 fonksiyon tek bir rollover fonksiyonundan cagirilacak disari acilmayacak
	void archiveCompletedSeasonHistory(int seasonYear);
	void archiveCompletedTeamSeasonStats(int seasonYear);
	void resetCurrentSeasonHistory();
	void resetCurrentTeamSeasonStats();
	void initializeTeamSeasonStats();
	void setCurrentSeasonYear(int seasonYear);
	//--------------------------------------------------------------------------------

public:

	//League constructor
	explicit League(const std::string& leagueName, LeagueId leagueId = 1);


	const std::string& getName() const;


	//Lig id'sini verir
	LeagueId getId() const;


	//Lige takim ekler
	void addTeam(std::unique_ptr<Team> team);
	

	//Butun takimlari unordered map olarak verir
	const std::unordered_map<TeamId, std::unique_ptr<Team>>& getTeams() const;


	//Takimi ID'sinden bulup poninterini verir
	Team* findTeamById(TeamId id);
	//Takimi ID'sinden bulup poninterini verir overloaded
	const Team* findTeamById(TeamId id) const;
	//Takim adinin verir
	const std::string& getTeamName(TeamId id) const;
	//Takimin olup olmadigini kontrol eder ID ile
	bool hasTeam(TeamId id) const;

	//Oyuncuyu ID ile bulur
	Footballer* findPlayerById(PlayerId id);
	//Oyuncuyu ID ile bulur overloaded
	const Footballer* findPlayerById(PlayerId id) const;

	//Fixture mac uretir
	void addFixtureMatch(int matchWeekIndex, const Date& date, TeamId homeId, TeamId awayId);

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

	//Sonraki maci bulup prewiew saglar
	std::optional<FixtureMatchPreview> getNextMatchForTeam(TeamId teamId) const;
	std::vector<FixtureMatchPreview> getUpcomingMatchesForTeam(TeamId teamId, std::size_t count) const;


	//standings fonksiyonlari-----------------------------------------------------------------------
	void initializeStandings();
	void resetStandings();
	void updateStandingsForMatch(TeamId homeId, TeamId awayId, const MatchResult& result);
	void updateTeamSeasonStatsForMatch(TeamId homeId, TeamId awayId, const MatchResult& result);
	void applyMatchResult(const Date& date, TeamId homeId, TeamId awayId, const MatchResult& result);
	void applyMatchPlayedEvent(const MatchPlayedEvent& event);
	std::vector<StandingsEntry> getSortedStandings() const;
	const std::unordered_map<TeamId, StandingsEntry>& getStandings() const;
	//----------------------------------------------------------------------------------------------


	//history fonksiyonlari-------------------------------------------------------------------------
	const std::vector<MatchRecord>& getCurrentSeasonHistory() const;
	const std::unordered_map<int, std::vector<MatchRecord>>& getArchivedHistoryBySeason() const;
	std::vector<MatchRecord> getMatchesForTeamInCurrentSeason(TeamId teamId) const;
	std::vector<MatchRecord> getMatchesForTeamInSeason(TeamId teamId, int seasonYear) const;
	std::vector<MatchRecord> getLastMatchesForTeam(TeamId teamId, std::size_t count) const;
	std::vector<MatchOutcome> getRecentOutcomes(TeamId teamId, std::size_t count) const;
	std::string getRecentFormString(TeamId teamId, std::size_t count) const;
	int getCurrentSeasonYear() const;     
	const std::unordered_map<TeamId, TeamSeasonStats>& getCurrentTeamSeasonStats() const;
	const std::unordered_map<int, std::unordered_map<TeamId, TeamSeasonStats>>& getArchivedTeamSeasonStatsBySeason() const;
	const TeamSeasonStats* getCurrentTeamSeasonStatsFor(TeamId teamId) const;
	const TeamSeasonStats* getArchivedTeamSeasonStatsFor(TeamId teamId, int seasonYear) const;
   //-----------------------------------------------------------------------------------------------


	std::optional<Date> tryGetMatchWeekEndDate(int matchWeekIndex) const;
	Date getLastFixtureDate() const;
	bool allMatchesPlayed() const;
	void initializeMatchdayTracking(int matchdaysPerSeason);

	bool isSeasonFixtureGenerated() const;
	void setSeasonFixtureGenerated(bool generated);
	void resetForNewSeason(int newSeasonYear);


	//debug
	int debugFixtureDayCount() const;
	//debug
	int debugTotalFixtureMatches() const;
	//debug
	int debugMatchdayCount() const;
};