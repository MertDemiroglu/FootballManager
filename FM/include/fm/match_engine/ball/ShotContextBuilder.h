#pragma once

#include"fm/match_engine/ball/ShotTypes.h"

class ShotContextBuilder {
public:
    ShotContext build(const ShotContextBuildRequest& request) const;
};
