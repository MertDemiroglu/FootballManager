#pragma once

#include<iostream>
#include<string>
#include"Contract.h"
class Footballer {

protected:
	std::unique_ptr<Contract> contract;
	std::string name, position, team;
	int age;

public:
	//Footballer constructor
	Footballer(const std::string& name, const std::string& position, const std::string& team, int age);

	//Virtual destructor alt sýnýflarýn silinmesi için
	virtual ~Footballer() = default;
	//Overall hesabý (her tür kendine göre)
	virtual int totalPower() const = 0;
	//Deðer hesabý (her tür kendine göre)
	virtual double calculateMarketValue() const = 0;

	//Oyuncunun ismi döndürür
	const std::string& getName() const;
	//Oyuncunun pozisyonu döndürür
	const std::string& getPosition() const;
	//Oyuncunun takýmýnýn ismini döndürür
	const std::string& getTeam() const;
	//Oyuncunun yaþýný döner
	int getAge() const;

	
	//Takým adýný düzenler
	void setTeam(const std::string& newTeam);
	
	//Kontrat imzalama
	void signContract(Money wage, int years);
	//Kontratý döndürür
	const Contract* getContract() const;
	//Kontrat süresini 1 yýl azaltýr
	void advanceContractYear();
	
	virtual void print(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const Footballer& f);