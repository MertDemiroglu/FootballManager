#include "fm/presentation/TeamSheetPresentationBuilder.h"

#include "fm/match/MatchReport.h"
#include "fm/match/TeamSheet.h"
#include "fm/match/TeamSheetEvaluationService.h"
#include "fm/roster/Footballer.h"
#include "fm/roster/Formation.h"
#include "fm/roster/FormationPitchLayout.h"
#include "fm/roster/FormationSlotRole.h"
#include "fm/roster/HeadCoach.h"
#include "fm/roster/Team.h"

#include <cctype>
#include <string>

namespace {
    std::string formationText(FormationId formation) {
        switch (formation) {
        case FormationId::FourFourTwo:
            return "4-4-2";
        case FormationId::FourThreeThree:
            return "4-3-3";
        case FormationId::ThreeFiveTwo:
            return "3-5-2";
        }

        return "Unknown";
    }

    std::string slotRoleText(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            return "GK";
        case FormationSlotRole::LeftBack:
            return "LB";
        case FormationSlotRole::CenterBack:
            return "CB";
        case FormationSlotRole::RightBack:
            return "RB";
        case FormationSlotRole::LeftWingBack:
            return "LWB";
        case FormationSlotRole::RightWingBack:
            return "RWB";
        case FormationSlotRole::DefensiveMidfielder:
            return "DM";
        case FormationSlotRole::CentralMidfielder:
            return "CM";
        case FormationSlotRole::AttackingMidfielder:
            return "AM";
        case FormationSlotRole::LeftMidfielder:
            return "LM";
        case FormationSlotRole::RightMidfielder:
            return "RM";
        case FormationSlotRole::LeftWinger:
            return "LW";
        case FormationSlotRole::RightWinger:
            return "RW";
        case FormationSlotRole::Striker:
            return "ST";
        case FormationSlotRole::Unknown:
            break;
        }

        return "?";
    }

    std::string surnameFromName(const std::string& value) {
        std::size_t end = value.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }
        if (end == 0) {
            return "Empty";
        }

        std::size_t start = end;
        while (start > 0 && !std::isspace(static_cast<unsigned char>(value[start - 1]))) {
            --start;
        }
        return value.substr(start, end - start);
    }

    LineupSummaryDto makeSummaryDto(const LineupSummary& summary) {
        LineupSummaryDto dto;
        dto.assignedPlayers = summary.assignedPlayers;
        dto.averageOverall = summary.averageOverall;
        dto.averageOverallText = summary.averageOverall.has_value()
            ? std::to_string(*summary.averageOverall)
            : "--";
        return dto;
    }

    FormationPitchCoordinate coordinateForLineupRow(
        FormationId formationId,
        std::size_t slotIndex,
        std::size_t slotCount,
        FormationSlotRole role) {
        const std::optional<FormationPitchCoordinate> coordinate =
            getFormationPitchCoordinate(formationId, slotIndex, role);
        if (coordinate.has_value()) {
            return *coordinate;
        }
        return getFallbackFormationPitchCoordinate(slotIndex, slotCount, role);
    }

    LineupPlayerViewDto makePlayerDto(
        int slotIndex,
        FormationId formationId,
        std::size_t slotCount,
        FormationSlotRole role,
        PlayerId playerId,
        const Team* team) {
        const std::string roleText = slotRoleText(role);
        const FormationPitchCoordinate coordinate =
            coordinateForLineupRow(formationId, static_cast<std::size_t>(slotIndex), slotCount, role);
        const Footballer* player = team && playerId != 0 ? team->findPlayerById(playerId) : nullptr;
        const std::string playerName = player ? player->getName() : "Empty";

        LineupPlayerViewDto dto;
        dto.slotIndex = slotIndex;
        dto.slotRoleKey = static_cast<int>(role);
        dto.playerId = playerId;
        dto.hasPlayer = playerId != 0;
        dto.playerName = playerName;
        dto.surname = surnameFromName(playerName);
        dto.positionText = roleText;
        dto.roleText = roleText;
        dto.slotRoleText = roleText;
        dto.overall = player ? player->totalPower() : 0;
        dto.pitchX = coordinate.x;
        dto.pitchY = coordinate.y;
        dto.displayOrder = coordinate.displayOrder;
        return dto;
    }
}

TeamSheetPresentationBuilder::TeamSheetPresentationBuilder(const TeamPresentationBuilder& teamBuilder)
    : teamBuilder(teamBuilder) {
}

TeamSheetViewDto TeamSheetPresentationBuilder::buildFromTeamSheet(const TeamSheet& sheet, const Team* team) const {
    TeamSheetViewDto dto;
    dto.team = teamBuilder.buildTeamVisual(team);
    dto.coachName = team ? team->getHeadCoach().getName() : "-";
    dto.formationText = formationText(sheet.formation);
    dto.summary = team
        ? makeSummaryDto(TeamSheetEvaluationService::summarize(sheet, *team))
        : LineupSummaryDto{};
    if (!dto.summary.averageOverall.has_value() && dto.summary.averageOverallText.empty()) {
        dto.summary.averageOverallText = "--";
    }

    dto.players.reserve(sheet.startingAssignments.size());
    for (std::size_t i = 0; i < sheet.startingAssignments.size(); ++i) {
        const TeamSheetSlotAssignment& assignment = sheet.startingAssignments[i];
        dto.players.push_back(makePlayerDto(
            static_cast<int>(i),
            sheet.formation,
            sheet.startingAssignments.size(),
            assignment.slotRole,
            assignment.playerId,
            team));
    }
    return dto;
}

TeamSheetViewDto TeamSheetPresentationBuilder::buildFromMatchLineupSnapshot(
    const MatchLineupSnapshot& snapshot,
    const Team* team) const {
    TeamSheetViewDto dto;
    dto.team = teamBuilder.buildTeamVisual(team);
    dto.coachName = team ? team->getHeadCoach().getName() : "-";
    dto.formationText = formationText(snapshot.formation);
    dto.summary = team
        ? makeSummaryDto(TeamSheetEvaluationService::summarizePlayerIds(snapshot.startingPlayerIds, *team))
        : LineupSummaryDto{};
    if (!dto.summary.averageOverall.has_value() && dto.summary.averageOverallText.empty()) {
        dto.summary.averageOverallText = "--";
    }

    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(snapshot.formation);
    dto.players.reserve(snapshot.startingPlayerIds.size());
    for (std::size_t i = 0; i < snapshot.startingPlayerIds.size(); ++i) {
        const FormationSlotRole role = i < slotTemplate.size() ? slotTemplate[i] : FormationSlotRole::Unknown;
        dto.players.push_back(makePlayerDto(
            static_cast<int>(i),
            snapshot.formation,
            snapshot.startingPlayerIds.size(),
            role,
            snapshot.startingPlayerIds[i],
            team));
    }
    return dto;
}
