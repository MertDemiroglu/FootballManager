#include"fm/match_engine/reporting/PlayerRatingModel.h"

#include<algorithm>

namespace {
    bool isDefensiveRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
        case FormationSlotRole::DefensiveMidfielder:
            return true;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
        case FormationSlotRole::Striker:
            return false;
        }

        return false;
    }
}

double PlayerRatingModel::calculate(const PlayerRatingContext& context) const {
    const MatchPlayerSimulationStats& stats = context.stats;
    double rating = 6.0
        + static_cast<double>(stats.goals) * 1.0
        + static_cast<double>(stats.assists) * 0.50
        + static_cast<double>(stats.shots) * 0.03
        + static_cast<double>(stats.interceptions) * 0.06
        + std::min(0.25, static_cast<double>(stats.passesCompleted) * 0.001)
        - static_cast<double>(stats.yellowCards) * 0.15
        - static_cast<double>(stats.redCards) * 1.0;

    if (stats.passesAttempted >= 20) {
        const double completion =
            static_cast<double>(stats.passesCompleted) / static_cast<double>(stats.passesAttempted);
        if (completion >= 0.85) {
            rating += 0.12;
        } else if (completion < 0.60) {
            rating -= 0.10;
        }
    }

    if (context.teamGoalsFor > context.teamGoalsAgainst) {
        rating += 0.15;
    } else if (context.teamGoalsFor < context.teamGoalsAgainst) {
        rating -= 0.15;
    }

    if (isDefensiveRole(context.role)) {
        if (context.teamGoalsAgainst == 0) {
            rating += 0.20;
        } else {
            rating -= std::min(0.55, static_cast<double>(context.teamGoalsAgainst) * 0.10);
        }
    }

    const bool exceptional =
        stats.goals >= 3
        || (stats.goals >= 2 && stats.assists >= 1)
        || stats.assists >= 3;
    return std::clamp(rating, 1.0, exceptional ? 10.0 : 9.8);
}
