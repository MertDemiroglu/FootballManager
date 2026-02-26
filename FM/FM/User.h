#pragma once
#include<string>
class Game;
class User {
private:
	std::string userName = "xx";
	std::string managedTeam = "x";
public:
	void setTeam();
	std::string getTeam() const;
};