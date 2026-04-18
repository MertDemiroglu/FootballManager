#pragma once

#include"fm/competition/League.h"
#include"fm/match/MatchReport.h"

class PlayerConditionService {
private:
	static int fitnessDeltaFromMinutes(int minutesPlayed);
	static int formDeltaFromMatch(const MatchPlayerReport& playerReport, const MatchReport& report);
	static int moraleDeltaFromMatch(const MatchPlayerReport& playerReport, const MatchReport& report);
	static int teamResultSign(TeamId teamId, const MatchReport& report);

public:
	void applyMatchEffects(const MatchReport& report, League& league) const;
	void applyDailyRecovery(League& league);
};