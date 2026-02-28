#include"League.h"

League::League(const std::string& leagueName) : name(leagueName) {}

void League::addTeam(std::unique_ptr<Team> team) {
	teams.emplace(team->getName(), std::move(team));
}
bool League::teamExists(const std::string& teamName) const {
	return teams.find(teamName) != teams.end();
}

Team* League::getTeam(const std::string& teamName) {
	auto it = teams.find(teamName);
	if (it != teams.end()) {
		return it->second.get();
	}
	return nullptr;
}


const std::unordered_map<std::string, std::unique_ptr<Team>>& League::getTeams() const {
	return teams;
}

void League::addFixtureMatch(const Date& date, std::string homeTeamName, std::string awayTeamName) {
	//struct'a bilgileri verdigimiz yer, ayni anda push ediliyor.
	fixture.push_back(FixtureMatch{ date, std::move(homeTeamName), std::move(awayTeamName) });
}

void League::clearFixture() {
	fixture.clear();
}

const std::vector<FixtureMatch>& League::getFixture() const {
	return fixture;
}

std::vector<FixtureMatch> League::getMatchesForDate(const Date& date) const {
	std::vector<FixtureMatch> matches;

	for (const auto& match : fixture) {
		const bool sameDay = match.date.getDay() == date.getDay() && match.date.getMonth() == date.getMonth() && match.date.getYear() == date.getYear();

		if (sameDay) {
			matches.push_back(match);
		}
	}
	return matches;
}