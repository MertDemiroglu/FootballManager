#include"fm/match_engine/offside/OffsideLineModel.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<vector>

namespace {
    double progress(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }
}

OffsideLineResult OffsideLineModel::evaluate(
    const TeamSimState& defendingTeam,
    PitchPoint ballPosition,
    AttackingDirection attackingDirection) const {
    std::vector<std::pair<double, PlayerId>> defenderProgress;
    defenderProgress.reserve(defendingTeam.players.size());
    for (const PlayerSimState& defender : defendingTeam.players) {
        defenderProgress.push_back({ progress(defender.position, attackingDirection), defender.playerId });
    }

    if (defenderProgress.size() < 2) {
        return OffsideLineResult{};
    }

    std::sort(
        defenderProgress.begin(),
        defenderProgress.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.first > rhs.first;
        });

    const double secondLastDefenderProgress = defenderProgress[1].first;
    const double ballProgress = progress(ballPosition, attackingDirection);
    OffsideLineResult result;
    result.valid = true;
    result.lineProgress = std::max(secondLastDefenderProgress, ballProgress);
    result.secondLastDefenderId = defenderProgress[1].second;
    result.secondLastDefenderProgress = secondLastDefenderProgress;
    return result;
}

