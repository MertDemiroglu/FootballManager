#pragma once

#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/DecisionContext.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

struct ActionScoreBreakdown {
    double retentionValue = 0.0;
    double progressionValue = 0.0;
    double chanceValue = 0.0;
    double tacticalFit = 0.0;
    double roleFit = 0.0;
    double skillFit = 0.0;
    double pressureCost = 0.0;
    double turnoverRiskCost = 0.0;
    double phaseFit = 0.0;
    double finalScore = 0.0;
};

struct ActionScoringContext {
    PlayerDecisionContext player;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
};

class ActionScoringModel {
public:
    ActionCandidate buildPassCandidate(
        const PassOption& option,
        const ActionScoringContext& context) const;

    ActionCandidate buildCarryCandidate(
        const CarryOption& option,
        const ActionScoringContext& context) const;

    ActionCandidate buildShotCandidate(
        const ShotOption& option,
        const ActionScoringContext& context) const;

    ActionCandidate buildHoldCandidate(const ActionScoringContext& context) const;
    ActionCandidate buildClearCandidate(const ActionScoringContext& context) const;
};
