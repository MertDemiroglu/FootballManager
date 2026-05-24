#pragma once

#include"fm/match/MatchReport.h"
#include"fm/match_engine/MatchEngineInput.h"
#include"fm/match_engine/MatchEngineResult.h"

class MatchEngineReportAdapter {
public:
    MatchReport buildReport(
        const MatchEngineInput& input,
        const MatchEngineResult& result) const;
};
