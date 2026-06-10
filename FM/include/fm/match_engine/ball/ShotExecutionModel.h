#pragma once

#include"fm/match_engine/ball/ShotTypes.h"

struct ShotExecutionRequest {
    ShotContext context;
    ShotType shotType = ShotType::ControlledFinish;
    ShotTargetSelectionResult intendedTarget;
};

class ShotExecutionModel {
public:
    ShotExecutionResult execute(const ShotExecutionRequest& request) const;
};
