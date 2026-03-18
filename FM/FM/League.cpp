#include"League.h"
#include<algorithm>
#include<stdexcept>

namespace {
	bool datesEqual(const Date& lhs, const Date& rhs) {
		return !(lhs < rhs) && !(rhs < lhs);
	}
}

League::League(const std::string& leagueName) : name(leagueName) {}

void League::addTeam(std::unique_ptr<Team> team) {
	if (!team) {
		throw std::invalid_argument("team cannot be null");
	}
	const std::string teamName = team->getName();
	const TeamId teamId = team->getId();

	if (teams.find(teamName) != teams.end()) {
		throw std::runtime_error("duplicate team name: " + teamName);
	}
	if (teamIndexById.find(teamId) != teamIndexById.end()) {
		throw std::runtime_error("duplicate team id");
	}
	Team* rawTeam = team.get();
	teams.emplace(teamName, std::move(team));
	teamIndexById.emplace(teamId, rawTeam);
	standings[teamId] = StandingsEntry{ teamId };
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

const Team* League::getTeam(const std::string& teamName) const {
	auto it = teams.find(teamName);
	if (it != teams.end()) {
		return it->second.get();
	}
	return nullptr;
}

Team* League::findTeamById(TeamId id) {
	auto it = teamIndexById.find(id);
	if (it != teamIndexById.end()) {
		return it->second;
	}
	return nullptr;
}

const Team* League::findTeamById(TeamId id) const {
	auto it = teamIndexById.find(id);
	if (it == teamIndexById.end()) {
		return nullptr;
	}
	return it->second;
}

const std::string& League::getTeamName(TeamId id) const {
	const Team* team = findTeamById(id);
	if (!team) {
		throw std::out_of_range("unknown team id");
	}
	return team->getName();
}

bool League::hasTeam(TeamId id) const {
	return teamIndexById.find(id) != teamIndexById.end();
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

void League::addFixtureMatch(int matchdayIndex, const Date& date, TeamId homeId, TeamId awayId) {
	if (matchdayIndex <= 0) {
		throw std::invalid_argument("matchdayIndex must be 1-based positive");
	}

	if (!hasTeam(homeId) || !hasTeam(awayId)){
		throw std::out_of_range("fixture contains unknown team id");
	}

	if (homeId == awayId) {
		throw std::invalid_argument("fixture match cannot contain same team twice");
	}

	const std::size_t zeroBased = static_cast<std::size_t>(matchdayIndex - 1);
	if (zeroBased >= matchdayEndDates.size()) {
		matchdayEndDates.resize(zeroBased + 1, std::nullopt);
	}

	auto& dayMatches = fixture[date];
	dayMatches.push_back(FixtureMatch{ homeId, awayId, false });

	auto& endDate = matchdayEndDates[zeroBased];
	if (!endDate.has_value() || *endDate < date) {
		endDate = date;
	}
}

void League::clearFixture() {
	fixture.clear();
	matchdayEndDates.clear();
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
		if (!match.played && !match.eventEnqueued) {
			matches.push_back(&match);
		}
	}
	return matches;
}

FixtureMatch* League::findFixtureMatch(const Date& date, TeamId homeId, TeamId awayId) {
	auto it = fixture.find(date);
	if (it == fixture.end()) {
		return nullptr;
	}
	for (auto& m : it->second) {
		if (m.homeId == homeId && m.awayId == awayId) {
			return &m;
		}
	}
	return nullptr;
}

const FixtureMatch* League::findFixtureMatch(const Date& date, TeamId homeId, TeamId awayId) const {
	auto it = fixture.find(date);
	if (it == fixture.end()) {
		return nullptr;
	}
	for (const auto& m : it->second) {
		if (m.homeId == homeId && m.awayId == awayId) {
			return &m;
		}
	}
	return nullptr;
}

void League::initializeStandings() {
	standings.clear();
	for (const auto& [teamName, team] : teams) {
		(void)teamName;
		standings.emplace(team->getId(), StandingsEntry{ team->getId() });
	}
}

void League::resetStandings() {
	standings.clear();
	initializeStandings();
}

void League::updateStandingsForMatch(TeamId homeId, TeamId awayId, const MatchResult& result) {
	auto homeIter = standings.find(homeId);
	auto awayIter = standings.find(awayId);

	if (homeIter == standings.end() || awayIter == standings.end()) {
		throw std::out_of_range("cannot update standings for unknown team id");
	}
	StandingsEntry& homeEntry = homeIter->second;
	StandingsEntry& awayEntry = awayIter->second;

	++homeEntry.played;
	++awayEntry.played;

	homeEntry.goalsFor += result.homeGoals;
	homeEntry.goalsAgainst += result.awayGoals;
	awayEntry.goalsFor += result.awayGoals;
	awayEntry.goalsAgainst += result.homeGoals;
	homeEntry.goalDifference = homeEntry.goalsFor - homeEntry.goalsAgainst;
	awayEntry.goalDifference = awayEntry.goalsFor - awayEntry.goalsAgainst;

	if (result.homeGoals > result.awayGoals) {
		homeEntry.wins++;
		awayEntry.losses++;
		homeEntry.points += 3;
	}
	else if (result.awayGoals > result.homeGoals) {
		homeEntry.losses++;
		awayEntry.wins++;
		awayEntry.points += 3;
	}
	else {
		++homeEntry.draws;
		++awayEntry.draws;
		++homeEntry.points;
		++awayEntry.points;
	}
}

void League::applyMatchResult(const Date& date, TeamId homeId, TeamId awayId, const MatchResult& result) {
	FixtureMatch* match = findFixtureMatch(date, homeId, awayId);
	if (!match) {
		throw std::runtime_error("fixture match not found for applyMatchResult");
	}
	if (match->played) {
		throw std::logic_error("fixture match already played");
	}
	if (!match->eventEnqueued) {
		throw std::logic_error("fixture match result cannot be applied before event enqueue");
	}
	match->homeGoals = result.homeGoals;
	match->awayGoals = result.awayGoals;
	match->played = true;
	match->eventEnqueued = false;

	updateStandingsForMatch(homeId, awayId, result);
}

std::vector<StandingsEntry> League::getSortedStandings() const {
	std::vector<StandingsEntry> sorted;
	sorted.reserve(standings.size());
	for (const auto& [teamId, entry] : standings) {
		(void)teamId;
		sorted.push_back(entry);
	}
	std::sort(sorted.begin(), sorted.end(), [](const StandingsEntry& lhs, const StandingsEntry& rhs) {
		if (lhs.points != rhs.points) {
			return lhs.points > rhs.points;
		}
		if (lhs.goalDifference != rhs.goalDifference) {
			return lhs.goalDifference > rhs.goalDifference;
		}
		if (lhs.goalsFor != rhs.goalsFor) {
			return lhs.goalsFor > rhs.goalsFor;
		}
		return lhs.teamId < rhs.teamId;
		});
	return sorted;
}

const std::unordered_map<TeamId, StandingsEntry>& League::getStandings() const {
	return standings;
}

bool League::isSeasonFixtureGenerated() const {
		return seasonFixtureGenerated;
}

void League::setSeasonFixtureGenerated(bool generated) {
		seasonFixtureGenerated = generated;
}

void League::resetForNewSeason() {
	clearFixture();	
	resetStandings();
	seasonFixtureGenerated = false;
}


//debug
int League::debugFixtureDayCount() const {
	return static_cast<int>(fixture.size());
}

int League::debugTotalFixtureMatches() const {
	int total = 0;
	for (const auto& [date, matches] : fixture) {
		(void)date;
		total += static_cast<int>(matches.size());
	}
	return total;
}
int League::debugMatchdayCount() const {
	return static_cast<int>(matchdayEndDates.size());
}