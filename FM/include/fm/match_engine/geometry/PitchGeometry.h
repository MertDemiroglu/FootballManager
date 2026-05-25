#pragma once

struct PitchPoint {
    double x = 0.0;
    double y = 0.0;
};

struct PitchDimensions {
    double lengthMeters = 105.0;
    double widthMeters = 68.0;
};

// Core coordinates are meter-based. Home goal line is x=0.0, away goal
// line is x=105.0, and y runs left touchline to right touchline.
// UI can normalize these coordinates later.
class PitchGeometry {
public:
    static constexpr double LengthMeters = 105.0;
    static constexpr double WidthMeters = 68.0;

    static constexpr double PenaltyAreaDepthMeters = 16.5;
    static constexpr double PenaltyAreaWidthMeters = 40.32;
    static constexpr double SixYardBoxDepthMeters = 5.5;
    static constexpr double SixYardBoxWidthMeters = 18.32;
    static constexpr double PenaltySpotDistanceMeters = 11.0;
    static constexpr double GoalWidthMeters = 7.32;

    static bool isInsidePitch(PitchPoint point);
    static PitchPoint clampToPitch(PitchPoint point);

    static bool isInsideHomePenaltyArea(PitchPoint point);
    static bool isInsideAwayPenaltyArea(PitchPoint point);
    static bool isInsideEitherPenaltyArea(PitchPoint point);

    static bool isInsideHomeSixYardBox(PitchPoint point);
    static bool isInsideAwaySixYardBox(PitchPoint point);

    static PitchPoint homePenaltySpot();
    static PitchPoint awayPenaltySpot();

    static PitchPoint homeGoalCenter();
    static PitchPoint awayGoalCenter();

    static double distance(PitchPoint a, PitchPoint b);
};
