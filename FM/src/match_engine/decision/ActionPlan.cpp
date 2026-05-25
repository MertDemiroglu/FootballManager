#include"fm/match_engine/decision/ActionPlan.h"

#include"../DeterministicRandom.h"

#include<algorithm>

namespace {
    double clampScore(double value) {
        return std::clamp(value, 0.0, 100.0);
    }

    double mentalPerceptionScore(const PlayerAttributes& attributes) {
        return attributes.mental.vision * 0.35
            + attributes.mental.decisions * 0.25
            + attributes.mental.composure * 0.20
            + attributes.mental.teamwork * 0.20;
    }
}

bool isActive(const ActionPlan& plan, double currentSecond) {
    if (plan.type == ActionPlanType::None || currentSecond < plan.startSecond) {
        return false;
    }

    if (plan.maxDurationSeconds > 0.0
        && currentSecond - plan.startSecond >= plan.maxDurationSeconds) {
        return false;
    }

    return true;
}

std::vector<ReassessmentTrigger> evaluateReassessmentTriggers(
    const ActionPlan& plan,
    const ReassessmentContext& context) {
    std::vector<ReassessmentTrigger> triggers;

    if (context.receivedBall) {
        triggers.push_back(ReassessmentTrigger::ReceivedBall);
    }
    if (context.enteredNewZone) {
        triggers.push_back(ReassessmentTrigger::EnteredNewZone);
    }
    if (context.reachedObjective) {
        triggers.push_back(ReassessmentTrigger::ReachedObjective);
    }
    if (context.pressureChanged) {
        triggers.push_back(ReassessmentTrigger::PressureChanged);
    }
    if (context.defenderInTackleRange) {
        triggers.push_back(ReassessmentTrigger::DefenderInTackleRange);
    }
    if (context.passingWindowOpened) {
        triggers.push_back(ReassessmentTrigger::PassingWindowOpened);
    }
    if (context.passingWindowClosed) {
        triggers.push_back(ReassessmentTrigger::PassingWindowClosed);
    }
    if (context.shootingWindowOpened) {
        triggers.push_back(ReassessmentTrigger::ShootingWindowOpened);
    }
    if (context.teammateDangerousRunStarted) {
        triggers.push_back(ReassessmentTrigger::TeammateDangerousRunStarted);
    }
    if (context.ballControlWorsened) {
        triggers.push_back(ReassessmentTrigger::BallControlWorsened);
    }

    if (plan.type != ActionPlanType::None
        && plan.maxDurationSeconds > 0.0
        && context.currentSecond - plan.startSecond >= plan.maxDurationSeconds) {
        triggers.push_back(ReassessmentTrigger::MaxDurationExpired);
    }

    if (plan.type != ActionPlanType::None
        && plan.reassessmentIntervalSeconds > 0.0
        && context.currentSecond - plan.lastReassessmentSecond >= plan.reassessmentIntervalSeconds) {
        triggers.push_back(ReassessmentTrigger::PeriodicScanDue);
    }

    return triggers;
}

bool PerceptionModel::perceivesOption(const PerceptionContext& context) const {
    return perceptionScore(context) >= 50.0;
}

double PerceptionModel::perceptionScore(const PerceptionContext& context) const {
    const double randomInfluence =
        (matchEngineDeterministicUnitInterval(context.seed) - 0.5) * 10.0;

    return clampScore(
        context.optionQuality * 0.55
        + mentalPerceptionScore(context.attributes) * 0.45
        - context.pressure * 0.22
        - context.ballControlDifficulty * 0.18
        + randomInfluence);
}
