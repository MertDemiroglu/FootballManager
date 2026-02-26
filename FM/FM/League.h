#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

#include "Types.h"
#include "Team.h"
#include "Footballer.h"

class League {
private:
	std::string name;
	std::unordered_map<std::string, std::unique_ptr<Team>> teams;
public:
	//League constructor
	explicit League(const std::string& name);
	//Lige takým ekler (Pointer olarak)
	void addTeam(std::unique_ptr<Team> team);
	//Takýmýn pointerýnýn döndürür
	Team* getTeam(const std::string& teamName);
	//Takýmýn ismini alýr, var ise true döner
	bool teamExists(const std::string& teamName) const;
	//Bütün takýmlarý unordered map olarak döndürür
	const std::unordered_map<std::string, std::unique_ptr<Team>>& getTeams() const;

};