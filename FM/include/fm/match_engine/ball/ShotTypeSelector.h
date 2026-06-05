#pragma once

#include"fm/match_engine/ball/ShotTypes.h"

class ShotTypeSelector {
public:
    ShotTypeSelectionResult select(const ShotContext& context) const;
};
