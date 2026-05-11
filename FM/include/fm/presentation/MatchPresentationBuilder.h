#pragma once

#include "fm/presentation/PresentationDtos.h"
#include "fm/presentation/TeamSheetPresentationBuilder.h"

class League;
class PostMatchInteraction;
class PreMatchInteraction;

class MatchPresentationBuilder {
public:
    explicit MatchPresentationBuilder(const TeamSheetPresentationBuilder& sheetBuilder);

    PreMatchViewDto buildPreMatch(const PreMatchInteraction& interaction, const League* league) const;
    PostMatchViewDto buildPostMatch(const PostMatchInteraction& interaction, const League* league) const;

private:
    const TeamSheetPresentationBuilder& sheetBuilder;
};
