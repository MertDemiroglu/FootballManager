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

void League::addFixtureMatch(const Date& date, const std::string& home, const std::string& away) {
	fixture[date].push_back(FixtureMatch{ home, away, false });
}

void League::clearFixture() {
	fixture.clear();
}

const std::map<Date, std::vector<FixtureMatch>>& League::getFixture() const {
	return fixture;
}

std::vector<FixtureMatch*> League::getMatchesForDate(const Date& date) {
	std::vector<FixtureMatch*> matches;

	auto it = fixture.find(date);

	if (it == fixture.end()) {
		return matches;
	}

	for (auto& match : it->second) {
		if (!match.played) {
			matches.push_back(&match);
		}
	}
	return matches;
}

bool League::isSeasonFixtureGenerated() const {
		return seasonFixtureGenerated;
}

void League::setSeasonFixtureGenerated(bool generated) {
		seasonFixtureGenerated = generated;
}

void League::resetForNewSeason() {

		fixture.clear();
		seasonFixtureGenerated = false;
}