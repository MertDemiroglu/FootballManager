#pragma once

#include"Team.h"
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

	Money transferBudget, wageBudget, totalBudget;
	std::string name;
	std::vector<std::unique_ptr<Footballer>> players;

public:

	//Team constructor
	explicit Team(const std::string& name);

	//takýmýn adýný döner
	const std::string& getName() const;

	//oyuncu sayýsý döner
	std::size_t playerCount() const;

	//Oyuncuyu takýma ekler
	void addPlayer(std::unique_ptr<Footballer> player);

	//Oyuncuyu serbest býrakýr (pointer'ýný býrakýr)
	std::unique_ptr<Footballer> releasePlayer(const std::string& playerName);

	//Oyuncuyu ismine göre bulur pointer'ýný döndürür
	Footballer* findPlayer(const std::string& name);

	//Transfer için yeterli para olup olmadýðýný kontrol eder
	bool canAffordTransfer(Money amount) const;
	//Maaþ için yeterli para olup olmadýðýný kontrol eder
	bool canAffordWage(Money amount) const;

	//Takýma para ekler
	void earn(Money amount);
	//Takýmdan para azaltýr
	void spend(Money amount);

	//Bütçeyi belirler
	void setBudgets();
	//Aylýk maaþ ödemesini yapar
	void payWagesMonthly();

	//takýmýn ortalama gücünü hesaplar
	int calculateTeamRating() const;

	//Kontratý biten oyuncularý bir vektöre koyup döndürür
	std::vector<std::unique_ptr<Footballer>>collectExpiredContracts();

	//kontrat sürelerini günceller ( 1 yýl azaltýr )
	void updateContracts();
	
	void print(std::ostream& os) const;
};
std::ostream& operator<<(std::ostream& os, const Team& team);