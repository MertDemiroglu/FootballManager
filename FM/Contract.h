#pragma once
#include"Types.h"

class Contract {
private:
	PlayerId playerId;
	TeamId teamId;
	Money wage;
	int yearsRemaining;
public:
	//Contract constructor
	Contract(PlayerId playerId, TeamId teamId, Money wage, int years);

	//Kontrat suresini 1 yýl azaltir
	void advanceYear();
	//Kontratin bitip bitmedigini kontrol eder
	bool isExpired() const;
	//Yillik maasi verir
	Money getWage() const;


	//Oyuncunun ID'sini verir
	PlayerId getPlayerId() const;
	//Oyuncunun takiminin ID'sini verir
	TeamId getTeamId() const;
	//Oyuncunun takiminin ID'sini set eder
	void setTeamId(TeamId newTeamId);
};
