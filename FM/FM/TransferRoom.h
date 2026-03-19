#pragma once
#include<vector>
#include<memory>
#include<string>
#include<algorithm>

#include"Types.h"

class League;
class Team;
class Footballer;

class TransferRoom{
private:

	League& league;
	std::vector <std::unique_ptr<Footballer>> freeAgents;
	bool transferOpen = false;

public:
    //Transfer room constructor
	TransferRoom(League& league);
	//Boþtaki oyuncuyu Transfer Room vektörüne ekler
	void addFreeAgent(std::unique_ptr<Footballer> player);
	//Boþtaki oyuncularý yazdýrýr
	void listFreeAgents() const;

	//Takýmdan takýma transfer, fonksiyon düzeltilecek sözleþme ile transfer ayrýlacak.
	bool transferPlayer(const std::string& fromTeam, const std::string& toTeam, const std::string& playerName, Money fee);
	//Boþtaki oyuncuyu transfer
	bool transferFreeAgent(const std::string& teamName, const std::string& playerName);
	//Sözleþme anlaþmalarýnda çaðýrýlan fonksiyon
	bool negotiateContract(Team* team, Footballer* player);

	//Takýmlardan sözleþmesi biten topçularý toplar
	void collectFreeAgentsFromTeams();
	//Sene sonu bütün takýmlardaki oyuncularýn kontrat sürelerini 1 yýl azaltýr
	void updatePlayersContractYearsInTeams();

	//Transfer room açýk ise true döner
	bool isOpen() const;
	//Transfer room'u açar
	void openWindow();
	//Transfer room'u kapatýr
	void closeWindow();

	
};