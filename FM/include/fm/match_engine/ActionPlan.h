#pragma once

#include"fm/match_engine/PitchGeometry.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>
#include<vector>

enum class ActionPlanType {
    None,
    CarryToCrossingZone,
    CutInside,
    AdvanceToNextLine,
    EscapePressure,
    DrawPressure,
    HoldForSupport
};

struct ActionPlan {
    ActionPlanType type = ActionPlanType::None;
    PitchPoint objectiveTarget;
    double startSecond = 0.0;
    double maxDurationSeconds = 0.0;
    double reassessmentIntervalSeconds = 1.0;
    double lastReassessmentSecond = 0.0;
};

bool isActive(const ActionPlan& plan, double currentSecond);

enum class ReassessmentTrigger {
    None,
    ReceivedBall,
    EnteredNewZone,
    ReachedObjective,
    PressureChanged,
    DefenderInTackleRange,
    PassingWindowOpened,
    PassingWindowClosed,
    ShootingWindowOpened,
    TeammateDangerousRunStarted,
    BallControlWorsened,
    MaxDurationExpired,
    PeriodicScanDue
};

struct ReassessmentContext {
    double currentSecond = 0.0;
    bool receivedBall = false;
    bool enteredNewZone = false;
    bool reachedObjective = false;
    bool pressureChanged = false;
    bool defenderInTackleRange = false;
    bool passingWindowOpened = false;
    bool passingWindowClosed = false;
    bool shootingWindowOpened = false;
    bool teammateDangerousRunStarted = false;
    bool ballControlWorsened = false;
};

std::vector<ReassessmentTrigger> evaluateReassessmentTriggers(
    const ActionPlan& plan,
    const ReassessmentContext& context);

struct PerceptionContext {
    PlayerAttributes attributes;
    double pressure = 0.0;
    double ballControlDifficulty = 0.0;
    double optionQuality = 0.0;
    std::uint64_t seed = 0;
};

class PerceptionModel {
public:
    bool perceivesOption(const PerceptionContext& context) const;
    double perceptionScore(const PerceptionContext& context) const;
};
