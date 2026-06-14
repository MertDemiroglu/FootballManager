#pragma once

#include"fm/match_engine/offside/OffsideTypes.h"

class OffsideAwarenessModel {
public:
    OffsideAwarenessResult evaluate(const OffsideAwarenessRequest& request) const;
    OffsideRiskResult evaluateRisk(const OffsideRiskRequest& request) const;

private:
    OffsideAwarenessTuning tuning_;
};
