#pragma once

#include"Footballer.h"
#include"TransferRoom.h"
#include"Types.h"
#include"Contract.h"

#include<vector>
#include<string>
#include<memory>
#include<ostream>
#include<algorithm>
#include<iostream>


class Team {
private:

	TeamId id;
	Money transferBudget, wageBudget, totalBudget;
	std::string name;
	std::vector<std::unique_ptr<Footballer>> players;

public:

	//Team constructor
	explicit Team(const std::string& name);
	Team(TeamId id, const std::string& name);

	TeamId getId() const;

	//takimin adini verir
	const std::string& getName() const;

	//oyuncu sayisini verir
	std::size_t playerCount() const;

	//Oyuncuyu takima ekler
	void addPlayer(std::unique_ptr<Footballer> player);

	//Oyuncuyu serbest birakir (pointer'ini birakir)
	std::unique_ptr<Footballer> releasePlayer(PlayerId playerId);
	//Oyuncuyu serbest birakir (pointer'ini birakir)
	std::unique_ptr<Footballer> releasePlayer(const std::string& playerName);

	//Oyuncuyu ismine gore bulur pointer'ini verir
	Footballer* findPlayer(const std::string& name);
	const Footballer* findPlayer(const std::string& name) const;

	//Oyuncuyu ID'sine gore bulur pointer'ini verir
	Footballer* findPlayerById(PlayerId id);
	const Footballer* findPlayerById(PlayerId id) const;

	//Transfer icin yeterli para olup olmadigini kontrol eder
	bool canAffordTransfer(Money amount) const;
	//Maas icin yeterli para olup olmadigini kontrol eder
	bool canAffordWage(Money amount) const;

	//Takima para ekler
	void earn(Money amount);
	//Takimdan para azaltir
	void spend(Money amount);

	//Butceyi belirler
	void setBudgets();
	//Aylik maas odemesini yapar
	void payWagesMonthly();

	//takimin ortalama gucunu hesaplar
	int calculateTeamRating() const;

	//Kontrati biten oyunculari bir vektore koyup verir
	std::vector<std::unique_ptr<Footballer>>collectExpiredContracts();

	//kontrat surelerini gunceller ( 1 yil azaltir )
	void updateContracts();
	
	void print(std::ostream& os) const;
};
std::ostream& operator<<(std::ostream& os, const Team& team);