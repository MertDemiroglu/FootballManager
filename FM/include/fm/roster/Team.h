#pragma once

#include"fm/roster/Footballer.h"
#include"fm/transfer/TransferRoom.h"
#include"fm/common/Types.h"
#include"fm/finance/TeamFinance.h"
#include"fm/roster/Contract.h"
#include"fm/roster/HeadCoach.h"
#include"fm/match/TeamSheet.h"

#include<vector>
#include<string>
#include<memory>
#include<optional>
#include<ostream>
#include<algorithm>
#include<iostream>
#include<unordered_map>


class Team {
private:

	TeamId id;
	TeamFinance finance;
	std::string name;
	std::string primaryColor;
	std::string secondaryColor;
	std::vector<std::unique_ptr<Footballer>> players;
	std::unordered_map<PlayerId, Footballer*> playerIndexById;
	HeadCoach headCoach;
    std::optional<TeamSheet> selectedTeamSheet;

	void rebuildPlayerIndexById();

public:

	//Team constructor
	explicit Team(const std::string& name);
	Team(TeamId id, const std::string& name);

	TeamId getId() const;

	//takimin adini verir
	const std::string& getName() const;
	const std::string& getPrimaryColor() const;
	const std::string& getSecondaryColor() const;
	void setTeamColors(const std::string& primaryColor, const std::string& secondaryColor);

	//oyuncu sayisini verir
	std::size_t playerCount() const;

	//Oyuncuyu takima ekler
	void addPlayer(std::unique_ptr<Footballer> player);

	//Oyuncuyu serbest birakir (pointer'ini birakir)
	std::unique_ptr<Footballer> releasePlayer(PlayerId playerId);


	//Oyuncuyu ID'sine gore bulur pointer'ini verir
	Footballer* findPlayerById(PlayerId id);
	const Footballer* findPlayerById(PlayerId id) const;
	const std::vector<std::unique_ptr<Footballer>>& getPlayers() const;

	//Transfer icin yeterli para olup olmadigini kontrol eder
	bool canAffordTransfer(Money amount) const;
	//Maas icin yeterli para olup olmadigini kontrol eder
	bool canAffordWage(Money amount) const;

	//Takima para ekler
	void earn(Money amount);
	//Takimdan para azaltir
	void spend(Money amount);
	void setBudgetSnapshot(Money totalBudget, Money transferBudget, Money wageBudget);
	void setBudgetSnapshot(Money totalBudget, Money transferBudget, Money wageBudget, ClubFinancialStrategy strategy);
	void setBudgetSnapshot(
		Money totalBudget,
		Money transferBudget,
		Money wageBudget,
		ClubFinancialStrategy strategy,
		ClubFinancialHealth health);
	Money getTotalBudget() const;
	Money getTransferBudget() const;
	Money getWageBudget() const;
	ClubFinancialStrategy getFinancialStrategy() const;
	ClubFinancialHealth getFinancialHealth() const;
	void spendTransferFee(Money fee);
	void receiveTransferFee(Money fee);
	Money calculateCurrentAnnualWageSpend() const;
	Money getAvailableWageBudget() const;

	//Butceyi belirler
	void setBudgets();
	//Aylik maas odemesini yapar
	void payWagesMonthly();

	//takimin ortalama gucunu hesaplar
	int calculateTeamRating() const;

	const HeadCoach& getHeadCoach() const;
	HeadCoach& getHeadCoach();
	void setHeadCoach(HeadCoach newHeadCoach);

    bool hasSelectedTeamSheet() const;
    const TeamSheet* getSelectedTeamSheet() const;
    TeamSheet* getSelectedTeamSheet();
    void setSelectedTeamSheet(TeamSheet teamSheet);
    void clearSelectedTeamSheet();

	//Kontrati biten oyunculari bir vektore koyup verir
	std::vector<std::unique_ptr<Footballer>>collectExpiredContracts();

	//kontrat surelerini gunceller ( 1 yil azaltir )
	void updateContracts();

	//player stats fonksiyonlari------------------------------
	void resetPlayerSeasonStats(int newSeasonYear);
	void archiveAndResetPlayerSeasonStats(int newSeasonYear);
	//--------------------------------------------------------

	void print(std::ostream& os) const;
};
std::ostream& operator<<(std::ostream& os, const Team& team);
