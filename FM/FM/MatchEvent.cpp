#include"MatchEvent.h"
#include"Team.h"
#include"Game.h"
#include"MatchSimulation.h"

#include<utility>

MatchEvent::MatchEvent(TeamId homeId, TeamId awayId, const Date& matchDate, std::string homeName, std::string awayName)
    : homeId(homeId), awayId(awayId), matchDate(matchDate), homeName(std::move(homeName)), awayName(std::move(awayName)) {
}

bool MatchEvent::isBlocking() const { 
    return block; 
}

EventPriority MatchEvent::getPriority() const {
    return priority;
}
void MatchEvent::resolve(Game& game) {
    League& league = game.getLeague();
    const Team* homeTeam = league.findTeamById(homeId);
    const Team* awayTeam = league.findTeamById(awayId);
    if (!homeTeam || !awayTeam) {
        return;
    }
    
    const MatchResult result = MatchSimulation::buildStrengthBasedResult(*homeTeam, *awayTeam, matchDate);
    league.applyMatchResult(matchDate, homeId, awayId, result);
}
const std::string& MatchEvent::getSendingTeam() const {
    return homeName;
}
const std::string& MatchEvent::getReceivingTeam() const {
    return awayName;
}
EventTargetType MatchEvent::getTargetType() const {
    return type;
}
bool MatchEvent::affectsTeam(const Team* team) const {
    if (!team) {
        return false;
    }
    return team->getId() == homeId || team->getId() == awayId;
}

static const std::string kEmpty{};
const std::string& MatchEvent::getPlayer() const {
    return kEmpty;
}