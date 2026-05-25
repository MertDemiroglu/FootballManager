#pragma once

#include"fm/match_engine/decision/DecisionTuningProfile.h"

// PR87 prepares a central tuning layer without changing gameplay values.
// PR88/PR89 should gradually move pass, carry, and shot decision constants
// into this profile instead of scattering unexplained numbers through the
// simulation runner.

struct MatchEngineTuningProfile {
    RoleRiskProfile roleRisk;
    TacticalBiasProfile tacticalBias;
    PassDecisionTuning passDecision;
    CarryDecisionTuning carryDecision;
    ShotDecisionTuning shotDecision;
};
