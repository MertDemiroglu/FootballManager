#include"User.h"

void User::setTeam(const std::string& teamName) {
	managedTeam = teamName;
}

std::string User::getTeam() const {
	return managedTeam;
}