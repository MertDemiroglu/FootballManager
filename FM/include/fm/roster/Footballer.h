#pragma once
#include<iostream>
#include<string>
#include<unordered_map>
#include<memory>

#include"fm/roster/Contract.h"
#include"fm/roster/PlayerPosition.h"
#include"fm/match/PlayerConditionState.h"

struct PlayerSeasonStats {
	PlayerId playerId = 0;
	int seasonYear = 0;

	int appearances = 0;
	int starts = 0;
	int substituteAppearances = 0;
	int minutesPlayed = 0;

	int goals = 0;
	int assists = 0;

	int yellowCards = 0;
	int redCards = 0;

	int cleanSheets = 0;
	int goalsConcededWhileOnPitch = 0;
};

class Footballer {

protected:

	PlayerId playerId;
	TeamId teamId;
	std::unique_ptr<Contract> contract;
	std::string name;
	PlayerPosition position;
	int age;
	PlayerSeasonStats currentSeasonStats;
	std::unordered_map<int, PlayerSeasonStats> archivedSeasonStatsByYear;
	PlayerConditionState conditionState;

public:
	//Footballer constructor
	Footballer(const std::string& name, PlayerPosition position, const std::string& team, int age);
	Footballer(const std::string& name, const std::string& position, const std::string& team, int age);

	//Virtual destructor alt siniflarin silinmesi icin
	virtual ~Footballer() = default;
	//Overall hesabi (her tur kendine gore)
	virtual int totalPower() const = 0;
	//Deger hesabi (her tur kendine gore)
	virtual double calculateMarketValue() const = 0;

	//Oyuncunun ID'sini verir
	PlayerId getId() const;
	//Oyuncunun takim ID'sini verir
	TeamId getTeamId() const;
	//Oyuncunun ismi verir
	const std::string& getName() const;
	//Oyuncunun pozisyonu verir
	std::string getPosition() const;
	PlayerPosition getPlayerPosition() const;
	//Oyuncunun yasini verir
	int getAge() const;

	//Oyuncunun form-fitness-moral degerlerini verir
	const PlayerConditionState& getConditionState() const;
	PlayerConditionState& getConditionState();

	
	//Takim bilgisini set eder
	void setTeam(TeamId newTeamId);
	void setIdForBootstrap(PlayerId newPlayerId);
	
	//Kontrat imzalama
	void signContract(Money wage, int years);
	//Kontrati verir
	const Contract* getContract() const;
	//Kontrat suresini 1 yil azaltir
	void advanceContractYear();
	
	//player stats fonksiyonlari-----------------------------------------------------------
	const PlayerSeasonStats& getCurrentSeasonStats() const;
	const std::unordered_map<int, PlayerSeasonStats>& getArchivedSeasonStatsByYear() const;
	void initializeSeasonStats(int seasonYear);
	void archiveCurrentSeasonStats();
	void resetSeasonStats(int newSeasonYear);
	void addAppearance(bool isStarter, int minutesPlayed);
	void addGoal();
	void addAssist();
	void addMinutes(int additionalMinutes);
	void addYellowCard();
	void addRedCard();
	//-------------------------------------------------------------------------------------

	virtual void print(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const Footballer& f);
