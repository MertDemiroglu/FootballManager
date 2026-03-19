#pragma once
#include<iostream>
#include<string>
#include<memory>
#include"Contract.h"

class Footballer {

protected:
	PlayerId playerId;
	TeamId teamId;
	std::unique_ptr<Contract> contract;
	std::string name, position, team;
	int age;

public:
	//Footballer constructor
	Footballer(const std::string& name, const std::string& position, const std::string& team, int age);

	//Virtual destructor alt siniflarin silinmesi icin
	virtual ~Footballer() = default;
	//Overall hesabi (her tur kendine gore)
	virtual int totalPower() const = 0;
	//Deger hesabi (her tur kendine gore)
	virtual double calculateMarketValue() const = 0;

	//Oyuncunun ID'sini verir
	PlayerId getId() const;
	//Oyuncunun takým ID'sini verir
	TeamId getTeamId() const;
	//Oyuncunun ismi verir
	const std::string& getName() const;
	//Oyuncunun pozisyonu verir
	const std::string& getPosition() const;
	//Oyuncunun takiminin ismini verir
	const std::string& getTeam() const;
	//Oyuncunun yasini verir
	int getAge() const;

	
	//Takým bilgisini set eder
	void setTeam(const std::string& newTeam, TeamId newTeamId);
	
	//Kontrat imzalama
	void signContract(Money wage, int years);
	//Kontrati verir
	const Contract* getContract() const;
	//Kontrat suresini 1 yil azaltir
	void advanceContractYear();
	
	virtual void print(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const Footballer& f);