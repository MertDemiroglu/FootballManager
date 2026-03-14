#include"League.h"

#include <stdexcept>

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

void League::initializeMatchdayTracking(int matchdaysPerSeason) {
	if (matchdaysPerSeason < 0) {
		throw std::invalid_argument("matchdaysPerSeason cannot be negative");
	}
	matchdayEndDates.assign(static_cast<std::size_t>(matchdaysPerSeason), std::nullopt);
}

void League::addFixtureMatch(int matchdayIndex, const Date& date, const std::string& home, const std::string& away) {
	if (matchdayIndex <= 0) {
		throw std::invalid_argument("matchdayIndex must be 1-based positive");
	}

	const std::size_t zeroBased = static_cast<std::size_t>(matchdayIndex - 1);
	if (zeroBased >= matchdayEndDates.size()) {
		matchdayEndDates.resize(zeroBased + 1, std::nullopt);
	}

	auto& dayMatches = fixture[date];
	dayMatches.push_back(FixtureMatch{ home, away, false });

	auto& endDate = matchdayEndDates[zeroBased];
	if (!endDate.has_value() || *endDate < date) {
		endDate = date;
	}
}

void League::clearFixture() {
	fixture.clear();
	matchdayEndDates.clear();
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

std::optional<Date> League::tryGetMatchdayEndDate(int matchdayIndex) const {
	if (matchdayIndex <= 0) {
		return std::nullopt;
	}
	const std::size_t zeroBased = static_cast<std::size_t>(matchdayIndex - 1);
	if (zeroBased >= matchdayEndDates.size()) {
		return std::nullopt;
	}
	return matchdayEndDates[zeroBased];
}

Date League::getLastFixtureDate() const {
	if (fixture.empty()) {
		throw std::runtime_error("fixture is empty");
	}
	return fixture.rbegin()->first;
}

bool League::allMatchesPlayed() const {
	for (const auto& [date, matches] : fixture) {
		(void)date;
		for (const auto& match : matches) {
			if (!match.played) {
				return false;
			}
		}
	}
	return !fixture.empty();
}

bool League::isSeasonFixtureGenerated() const {
		return seasonFixtureGenerated;
}

void League::setSeasonFixtureGenerated(bool generated) {
		seasonFixtureGenerated = generated;
}

void League::resetForNewSeason() {

		fixture.clear();
		matchdayEndDates.clear();
		seasonFixtureGenerated = false;
}


//debug
int League::debugFixtureDayCount() const {
	return static_cast<int>(fixture.size());
}

int League::debugTotalFixtureMatches() const {
	int total = 0;
	for (const auto& [date, matches] : fixture) {
		total += static_cast<int>(matches.size());
	}
	return total;
}

int League::debugMatchdayCount() const {
	return static_cast<int>(matchdayEndDates.size());
}
