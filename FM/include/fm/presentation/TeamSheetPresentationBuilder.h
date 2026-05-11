#pragma once

#include "fm/presentation/PresentationDtos.h"
#include "fm/presentation/TeamPresentationBuilder.h"

class Team;
struct MatchLineupSnapshot;
struct TeamSheet;

class TeamSheetPresentationBuilder {
public:
    explicit TeamSheetPresentationBuilder(const TeamPresentationBuilder& teamBuilder);

    TeamSheetViewDto buildFromTeamSheet(const TeamSheet& sheet, const Team* team) const;
    TeamSheetViewDto buildFromMatchLineupSnapshot(const MatchLineupSnapshot& snapshot, const Team* team) const;

private:
    const TeamPresentationBuilder& teamBuilder;
};
