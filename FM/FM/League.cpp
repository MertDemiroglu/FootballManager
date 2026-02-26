#include"League.h"

League::League(const std::string& name) : name(name) {}

void League::addTeam(std::unique_ptr<Team> team) {
	teams.emplace(team->getName(), std::move(team));
}
bool League::teamExists(const std::string& name) const {
	return teams.find(name) != teams.end();
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
