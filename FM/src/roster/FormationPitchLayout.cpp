#include"fm/roster/FormationPitchLayout.h"

#include<array>
#include<algorithm>

namespace {
    using CoordinateArray = std::array<FormationPitchCoordinate, 11>;

    constexpr CoordinateArray kFourFourTwoCoordinates{ {
        { 0.50, 0.92, 0 },
        { 0.16, 0.72, 1 },
        { 0.38, 0.72, 2 },
        { 0.62, 0.72, 3 },
        { 0.84, 0.72, 4 },
        { 0.16, 0.48, 5 },
        { 0.38, 0.48, 6 },
        { 0.62, 0.48, 7 },
        { 0.84, 0.48, 8 },
        { 0.38, 0.20, 9 },
        { 0.62, 0.20, 10 },
    } };

    constexpr CoordinateArray kFourThreeThreeCoordinates{ {
        { 0.50, 0.92, 0 },
        { 0.16, 0.72, 1 },
        { 0.38, 0.72, 2 },
        { 0.62, 0.72, 3 },
        { 0.84, 0.72, 4 },
        { 0.30, 0.52, 5 },
        { 0.50, 0.48, 6 },
        { 0.70, 0.52, 7 },
        { 0.22, 0.22, 8 },
        { 0.50, 0.17, 9 },
        { 0.78, 0.22, 10 },
    } };

    constexpr CoordinateArray kThreeFiveTwoCoordinates{ {
        { 0.50, 0.92, 0 },
        { 0.30, 0.72, 1 },
        { 0.50, 0.72, 2 },
        { 0.70, 0.72, 3 },
        { 0.12, 0.50, 4 },
        { 0.34, 0.48, 5 },
        { 0.50, 0.52, 6 },
        { 0.66, 0.48, 7 },
        { 0.88, 0.50, 8 },
        { 0.40, 0.20, 9 },
        { 0.60, 0.20, 10 },
    } };

    const CoordinateArray& coordinatesForFormation(FormationId formationId) {
        switch (formationId) {
        case FormationId::FourFourTwo:
            return kFourFourTwoCoordinates;
        case FormationId::FourThreeThree:
            return kFourThreeThreeCoordinates;
        case FormationId::ThreeFiveTwo:
            return kThreeFiveTwoCoordinates;
        }

        return kFourFourTwoCoordinates;
    }

    double yForRole(FormationSlotRole slotRole) {
        switch (slotRole) {
        case FormationSlotRole::Goalkeeper:
            return 0.92;
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            return 0.72;
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
            return 0.50;
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
        case FormationSlotRole::Striker:
            return 0.22;
        case FormationSlotRole::Unknown:
            return 0.50;
        }

        return 0.50;
    }
}

std::optional<FormationPitchCoordinate> getFormationPitchCoordinate(
    FormationId formationId,
    std::size_t slotIndex,
    FormationSlotRole slotRole) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formationId);
    const CoordinateArray& coordinates = coordinatesForFormation(formationId);
    if (slotTemplate.size() != coordinates.size() || slotIndex >= slotTemplate.size()) {
        return std::nullopt;
    }
    if (slotTemplate[slotIndex] != slotRole) {
        return std::nullopt;
    }

    return coordinates[slotIndex];
}

FormationPitchCoordinate getFallbackFormationPitchCoordinate(
    std::size_t slotIndex,
    std::size_t slotCount,
    FormationSlotRole slotRole) {
    const std::size_t safeSlotCount = std::max<std::size_t>(slotCount, 1);
    const double spacing = 1.0 / static_cast<double>(safeSlotCount + 1);
    const double x = spacing * static_cast<double>(slotIndex + 1);
    return FormationPitchCoordinate{ std::clamp(x, 0.10, 0.90), yForRole(slotRole), static_cast<int>(slotIndex) };
}
