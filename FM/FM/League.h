#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

#include "Types.h"
#include "Team.h"
#include "Footballer.h"
#include "Date.h"

struct FixtureMatch {
	Date date;
	std::string homeTeamName;
	std::string awayTeamName;	
};

class League {
private:
	std::string name;
	std::unordered_map<std::string, std::unique_ptr<Team>> teams;
	std::vector<FixtureMatch> fixture;
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
	void addFixtureMatch(const Date& date, std::string homeTeamName, std::string awayTeamName);

	//Fixturu temizler
	void clearFixture();

	//Fixturu verir
	const std::vector<FixtureMatch>& getFixture() const;

	//O gunun maclarini vector olarak verir
	std::vector<FixtureMatch> getMatchesForDate(const Date& date) const;
};