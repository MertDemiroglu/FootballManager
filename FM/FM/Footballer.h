#pragma once
#include<iostream>
#include<string>
#include<memory>
#include"Contract.h"

class Footballer {

protected:
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

	//Oyuncunun ismi verir
	const std::string& getName() const;
	//Oyuncunun pozisyonu verir
	const std::string& getPosition() const;
	//Oyuncunun takýmýnýn ismini verir
	const std::string& getTeam() const;
	//Oyuncunun yasýný verir
	int getAge() const;

	
	//Takým adýný set eder
	void setTeam(const std::string& newTeam);
	
	//Kontrat imzalama
	void signContract(Money wage, int years);
	//Kontratý verir
	const Contract* getContract() const;
	//Kontrat süresini 1 yil azaltir
	void advanceContractYear();
	
	virtual void print(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const Footballer& f);