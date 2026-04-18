#pragma once
#include<vector>
#include<memory>
#include<string>
#include<algorithm>
#include<unordered_map>

#include"fm/common/Types.h"

class World;
class Team;
class Footballer;

class TransferRoom{
private:

	World* world = nullptr;
	std::vector <std::unique_ptr<Footballer>> freeAgents;
	std::unordered_map<LeagueId, bool> transferWindowByLeagueId;

public:
	TransferRoom();

	explicit TransferRoom(World& world);

	void bindWorld(World& world);

	//Boþtaki oyuncuyu Transfer Room vektörüne ekler
	void addFreeAgent(std::unique_ptr<Footballer> player);
	//Boþtaki oyuncularý yazdýrýr
	void listFreeAgents() const;

	//Takýmdan takýma transfer, fonksiyon düzeltilecek sözleþme ile transfer ayrýlacak.
	bool transferPlayer(LeagueId fromLeagueId, TeamId fromTeamId, LeagueId toLeagueId, TeamId toTeamId, PlayerId playerId, Money fee);
	//Boþtaki oyuncuyu transfer
	bool transferFreeAgent(LeagueId toLeagueId, TeamId toTeamId, PlayerId playerId);
	//Sözleþme anlaþmalarýnda çaðýrýlan fonksiyon
	bool negotiateContract(Team* team, Footballer* player);


	//Takýmlardan sözleþmesi biten topçularý toplar
	void collectFreeAgentsFromTeams();
	void collectFreeAgentsFromLeague(LeagueId leagueId);
	//Sene sonu bütün takýmlardaki oyuncularýn kontrat sürelerini 1 yýl azaltýr
	void updatePlayersContractYearsInTeams();
	void updatePlayersContractYearsInLeague(LeagueId leagueId);


	bool isOpen() const;
	bool isOpenForLeague(LeagueId leagueId) const;
	void openWindow();
	void openWindowForLeague(LeagueId leagueId);
	void closeWindow();
	void closeWindowForLeague(LeagueId leagueId);
	
};