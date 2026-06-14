#include"fm/match/TeamSelectionService.h"

#include"fm/roster/Footballer.h"
#include"fm/roster/Formation.h"
#include"fm/roster/PlayerRoleFit.h"
#include"fm/roster/Team.h"

#include<algorithm>
#include<array>
#include<cmath>
#include<limits>
#include<numeric>
#include<unordered_set>
#include<vector>

namespace {
struct PositionBucketEntry {
    const Footballer* player = nullptr;
    PlayerId id = 0;
};

struct NaturalPositionIndex {
    std::array<std::vector<PositionBucketEntry>, static_cast<std::size_t>(PlayerPosition::Striker) + 1> byPosition{};
    std::size_t squadSize = 0;
};

struct SlotOption {
    PlayerId playerId = 0;
    double effectivePower = 0.0;
    RoleFitTier fitTier = RoleFitTier::Invalid;
};

struct AssignmentResult {
    std::vector<PlayerId> orderedPlayerIdsByTemplate;
    int assignedSlotCount = 0;
    double totalEffectivePower = 0.0;
    int totalFitScore = 0;
};

struct FormationSelectionCandidate {
    FormationId formation = FormationId::FourThreeThree;
    std::vector<PlayerId> orderedPlayerIdsByTemplate;
    std::vector<TeamSheetSlotAssignment> orderedAssignments;
    int assignedSlotCount = 0;
    double totalEffectivePower = 0.0;
    int totalFitScore = 0;
    bool fullySatisfied = false;
};

struct RankedRoleCandidate {
    const Footballer* player = nullptr;
    PlayerId playerId = 0;
    double baseSlotScore = 0.0;
};

struct RoleTierBuckets {
    std::vector<RankedRoleCandidate> natural;
    std::vector<RankedRoleCandidate> close;
    std::vector<RankedRoleCandidate> adapted;
    std::vector<RankedRoleCandidate> emergency;
};

struct FormationRoleEntry {
    FormationSlotRole role = FormationSlotRole::Unknown;
    std::size_t demand = 0;
    RoleTierBuckets tierBuckets;
    std::vector<SlotOption> expandedOptions;
};

struct FormationRolePlan {
    std::vector<FormationSlotRole> activeSlots;
    std::vector<FormationRoleEntry> roleEntries;
    std::array<int, static_cast<std::size_t>(FormationSlotRole::Striker) + 1> roleToEntryIndex{};

    FormationRolePlan() {
        roleToEntryIndex.fill(-1);
    }
};

enum class SubstituteBucket {
    Goalkeeper,
    CenterBack,
    FullBack,
    WingBack,
    CentralMidfielder,
    LeftMidfielder,
    RightMidfielder,
    Winger,
    Striker
};

struct SubstituteCandidate {
    const Footballer* player = nullptr;
    PlayerId playerId = 0;
    double overallScore = 0.0;
};

bool isNearlyEqual(double lhs, double rhs) {
    return std::fabs(lhs - rhs) < 1e-9;
}

bool lexicographicallyBetterIds(const std::vector<PlayerId>& lhs, const std::vector<PlayerId>& rhs) {
    const std::size_t size = std::min(lhs.size(), rhs.size());
    for (std::size_t i = 0; i < size; ++i) {
        const PlayerId leftId = lhs[i] == 0 ? std::numeric_limits<PlayerId>::max() : lhs[i];
        const PlayerId rightId = rhs[i] == 0 ? std::numeric_limits<PlayerId>::max() : rhs[i];
        if (leftId < rightId) {
            return true;
        }
        if (leftId > rightId) {
            return false;
        }
    }

    return lhs.size() < rhs.size();
}

bool isBetterAssignment(const AssignmentResult& lhs, const AssignmentResult& rhs) {
    if (lhs.assignedSlotCount != rhs.assignedSlotCount) {
        return lhs.assignedSlotCount > rhs.assignedSlotCount;
    }

    if (!isNearlyEqual(lhs.totalEffectivePower, rhs.totalEffectivePower)) {
        return lhs.totalEffectivePower > rhs.totalEffectivePower;
    }

    if (lhs.totalFitScore != rhs.totalFitScore) {
        return lhs.totalFitScore > rhs.totalFitScore;
    }

    return lexicographicallyBetterIds(lhs.orderedPlayerIdsByTemplate, rhs.orderedPlayerIdsByTemplate);
}

bool isGoalkeeperRole(FormationSlotRole slotRole) {
    return slotRole == FormationSlotRole::Goalkeeper;
}

enum class FitnessAvailabilityBand {
    FullAvailable,
    NormalAvailable,
    RiskyAvailable,
    EmergencyOnly,
    Unavailable
};

FitnessAvailabilityBand classifyFitnessAvailability(int fitness) {
    if (fitness >= 85) {
        return FitnessAvailabilityBand::FullAvailable;
    }
    if (fitness >= 70) {
        return FitnessAvailabilityBand::NormalAvailable;
    }
    if (fitness >= 55) {
        return FitnessAvailabilityBand::RiskyAvailable;
    }
    if (fitness >= 40) {
        return FitnessAvailabilityBand::EmergencyOnly;
    }

    return FitnessAvailabilityBand::Unavailable;
}

bool isAvailableForNormalTier(FitnessAvailabilityBand availability, RoleFitTier tier) {
    if (availability == FitnessAvailabilityBand::Unavailable) {
        return false;
    }

    // Emergency-only players are intentionally hidden from normal tiers and only
    // considered when the role expansion reaches emergency depth.
    if (availability == FitnessAvailabilityBand::EmergencyOnly) {
        return tier == RoleFitTier::Emergency;
    }

    return true;
}

double fitnessSelectionMultiplier(int fitness) {
    if (fitness >= 95) return 1.03;
    if (fitness >= 85) return 1.00;
    if (fitness >= 75) return 0.97;
    if (fitness >= 65) return 0.88;
    if (fitness >= 55) return 0.76;
    if (fitness >= 40) return 0.60;
    return 0.45;
}

double formSelectionMultiplier(int form) {
    if (form >= 85) return 1.07;
    if (form >= 70) return 1.03;
    if (form >= 55) return 1.00;
    if (form >= 40) return 0.96;
    return 0.92;
}

double moraleSelectionMultiplier(int morale) {
    if (morale >= 85) return 1.04;
    if (morale >= 70) return 1.02;
    if (morale >= 55) return 1.00;
    if (morale >= 40) return 0.98;
    return 0.95;
}

double calculateConditionMultiplier(const Footballer& player) {
    const PlayerConditionState& condition = player.getConditionState();
    return fitnessSelectionMultiplier(condition.getFitness())
        * formSelectionMultiplier(condition.getForm())
        * moraleSelectionMultiplier(condition.getMorale());
}

double calculateConditionAwarePowerForSlot(const Footballer& player, FormationSlotRole slotRole) {
    // role-adjusted base power * fitness * form * morale.
    return calculateEffectivePowerForSlot(player, slotRole) * calculateConditionMultiplier(player);
}

double calculateConditionAwarePowerWithoutRoleFit(const Footballer& player) {
    return static_cast<double>(player.totalPower()) * calculateConditionMultiplier(player);
}

TacticalSetup defaultTacticalSetupForTeam(const Team& team) {
    const TacticalPreferences& preferences = team.getHeadCoach().getTacticalPreferences();
    return TacticalSetup{
        preferences.preferredMentality,
        preferences.preferredTempo
    };
}

std::vector<TeamSheetSlotAssignment> buildOrderedAssignmentsFromTemplate(FormationId formation,
                                                                         const std::vector<PlayerId>& orderedPlayerIdsByTemplate) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    const std::size_t slotCount = std::min(slotTemplate.size(), orderedPlayerIdsByTemplate.size());

    std::vector<TeamSheetSlotAssignment> orderedAssignments;
    orderedAssignments.reserve(slotCount);
    for (std::size_t i = 0; i < slotCount; ++i) {
        const PlayerId playerId = orderedPlayerIdsByTemplate[i];
        if (playerId == 0) {
            continue;
        }

        orderedAssignments.push_back(TeamSheetSlotAssignment{ i, slotTemplate[i], playerId });
    }

    return orderedAssignments;
}

void completeUnderfilledCandidate(FormationSelectionCandidate& candidate,
                                  const Team& team,
                                  std::size_t starterTargetCount) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(candidate.formation);
    const std::size_t slotCount = std::min(starterTargetCount, slotTemplate.size());
    if (slotCount == 0) {
        return;
    }

    if (candidate.orderedPlayerIdsByTemplate.size() < slotCount) {
        candidate.orderedPlayerIdsByTemplate.resize(slotCount, 0);
    }

    std::vector<const Footballer*> squadPlayers;
    squadPlayers.reserve(team.getPlayers().size());
    for (const auto& player : team.getPlayers()) {
        if (!player) {
            continue;
        }

        squadPlayers.push_back(player.get());
    }

    if (squadPlayers.empty()) {
        return;
    }

    int assignedCount = 0;
    std::vector<PlayerId> usedPlayerIds;
    usedPlayerIds.reserve(slotCount);
    for (std::size_t i = 0; i < slotCount; ++i) {
        const PlayerId playerId = candidate.orderedPlayerIdsByTemplate[i];
        if (playerId == 0) {
            continue;
        }

        ++assignedCount;
        usedPlayerIds.push_back(playerId);
    }

    if (assignedCount >= static_cast<int>(slotCount) || usedPlayerIds.size() >= squadPlayers.size()) {
        candidate.assignedSlotCount = assignedCount;
        candidate.fullySatisfied = candidate.assignedSlotCount == static_cast<int>(slotCount);
        candidate.orderedAssignments = buildOrderedAssignmentsFromTemplate(candidate.formation, candidate.orderedPlayerIdsByTemplate);
        return;
    }

    const auto isPlayerUnused = [&usedPlayerIds](PlayerId playerId) {
        return std::find(usedPlayerIds.begin(), usedPlayerIds.end(), playerId) == usedPlayerIds.end();
    };

    for (std::size_t slotIdx = 0; slotIdx < slotCount; ++slotIdx) {
        if (candidate.orderedPlayerIdsByTemplate[slotIdx] != 0) {
            continue;
        }
        if (usedPlayerIds.size() >= squadPlayers.size()) {
            break;
        }

        const FormationSlotRole slotRole = slotTemplate[slotIdx];
        const bool slotIsGoalkeeper = isGoalkeeperRole(slotRole);
        const Footballer* selectedPlayer = nullptr;

        if (slotIsGoalkeeper) {
            std::vector<const Footballer*> gkCandidates;
            for (const Footballer* player : squadPlayers) {
                if (!isPlayerUnused(player->getId())) {
                    continue;
                }
                if (player->getPlayerPosition() != PlayerPosition::Goalkeeper) {
                    continue;
                }

                gkCandidates.push_back(player);
            }

            std::sort(gkCandidates.begin(), gkCandidates.end(), [slotRole](const Footballer* lhs, const Footballer* rhs) {
                const double lhsScore = calculateConditionAwarePowerForSlot(*lhs, slotRole);
                const double rhsScore = calculateConditionAwarePowerForSlot(*rhs, slotRole);
                if (!isNearlyEqual(lhsScore, rhsScore)) {
                    return lhsScore > rhsScore;
                }

                return lhs->getId() < rhs->getId();
            });

            if (!gkCandidates.empty()) {
                selectedPlayer = gkCandidates.front();
            }
        } else {
            std::vector<const Footballer*> usableOutfieldCandidates;
            for (const Footballer* player : squadPlayers) {
                if (!isPlayerUnused(player->getId())) {
                    continue;
                }
                if (player->getPlayerPosition() == PlayerPosition::Goalkeeper) {
                    continue;
                }

                const RoleFitTier fitTier = getRoleFitTier(player->getPlayerPosition(), slotRole);
                if (fitTier == RoleFitTier::Invalid) {
                    continue;
                }

                usableOutfieldCandidates.push_back(player);
            }

            std::sort(usableOutfieldCandidates.begin(), usableOutfieldCandidates.end(), [slotRole](const Footballer* lhs, const Footballer* rhs) {
                const double lhsScore = calculateConditionAwarePowerForSlot(*lhs, slotRole);
                const double rhsScore = calculateConditionAwarePowerForSlot(*rhs, slotRole);
                if (!isNearlyEqual(lhsScore, rhsScore)) {
                    return lhsScore > rhsScore;
                }

                const int lhsTierScore = getRoleFitTierScore(getRoleFitTier(lhs->getPlayerPosition(), slotRole));
                const int rhsTierScore = getRoleFitTierScore(getRoleFitTier(rhs->getPlayerPosition(), slotRole));
                if (lhsTierScore != rhsTierScore) {
                    return lhsTierScore > rhsTierScore;
                }

                return lhs->getId() < rhs->getId();
            });

            if (!usableOutfieldCandidates.empty()) {
                selectedPlayer = usableOutfieldCandidates.front();
            } else {
                std::vector<const Footballer*> forcedOutfieldCandidates;
                for (const Footballer* player : squadPlayers) {
                    if (!isPlayerUnused(player->getId())) {
                        continue;
                    }
                    if (player->getPlayerPosition() == PlayerPosition::Goalkeeper) {
                        continue;
                    }

                    forcedOutfieldCandidates.push_back(player);
                }

                std::sort(forcedOutfieldCandidates.begin(), forcedOutfieldCandidates.end(), [](const Footballer* lhs, const Footballer* rhs) {
                    const double lhsScore = calculateConditionAwarePowerWithoutRoleFit(*lhs);
                    const double rhsScore = calculateConditionAwarePowerWithoutRoleFit(*rhs);
                    if (!isNearlyEqual(lhsScore, rhsScore)) {
                        return lhsScore > rhsScore;
                    }

                    return lhs->getId() < rhs->getId();
                });

                if (!forcedOutfieldCandidates.empty()) {
                    selectedPlayer = forcedOutfieldCandidates.front();
                }
            }
        }

        if (selectedPlayer) {
            candidate.orderedPlayerIdsByTemplate[slotIdx] = selectedPlayer->getId();
            usedPlayerIds.push_back(selectedPlayer->getId());
            ++assignedCount;
        }
    }

    candidate.assignedSlotCount = assignedCount;
    candidate.fullySatisfied = candidate.assignedSlotCount == static_cast<int>(slotCount);
    candidate.orderedAssignments = buildOrderedAssignmentsFromTemplate(candidate.formation, candidate.orderedPlayerIdsByTemplate);
}

NaturalPositionIndex buildNaturalPositionIndex(const Team& team) {
    NaturalPositionIndex index;

    // Single squad pass per buildTeamSheet: place every player directly into their
    // natural-position bucket as a pointer (no player-object copies).
    for (const auto& player : team.getPlayers()) {
        if (!player) {
            continue;
        }

        const Footballer* playerPtr = player.get();
        const PlayerPosition naturalPosition = playerPtr->getPlayerPosition();
        if (!isKnownPlayerPosition(naturalPosition)) {
            continue;
        }

        index.byPosition[static_cast<std::size_t>(naturalPosition)].push_back(PositionBucketEntry{ playerPtr, playerPtr->getId() });
        ++index.squadSize;
    }

    for (std::size_t posIndex = static_cast<std::size_t>(PlayerPosition::Goalkeeper);
         posIndex <= static_cast<std::size_t>(PlayerPosition::Striker);
         ++posIndex) {
        std::vector<PositionBucketEntry>& bucket = index.byPosition[posIndex];
        std::sort(bucket.begin(), bucket.end(), [](const PositionBucketEntry& lhs, const PositionBucketEntry& rhs) {
            return lhs.id < rhs.id;
        });
    }

    return index;
}

int bucketFitTier(SubstituteBucket bucket, PlayerPosition position) {
    switch (bucket) {
    case SubstituteBucket::Goalkeeper:
        return position == PlayerPosition::Goalkeeper ? 0 : -1;

    case SubstituteBucket::CenterBack:
        if (position == PlayerPosition::CenterBack) return 0;
        if (position == PlayerPosition::DefensiveMidfielder) return 2;
        return -1;

    case SubstituteBucket::FullBack:
        if (position == PlayerPosition::LeftBack || position == PlayerPosition::RightBack) return 0;
        return -1;

    case SubstituteBucket::WingBack:
        if (position == PlayerPosition::LeftBack || position == PlayerPosition::RightBack) return 1;
        if (position == PlayerPosition::LeftMidfielder || position == PlayerPosition::RightMidfielder) return 2;
        return -1;

    case SubstituteBucket::CentralMidfielder:
        if (position == PlayerPosition::CentralMidfielder) return 0;
        if (position == PlayerPosition::DefensiveMidfielder) return 1;
        if (position == PlayerPosition::AttackingMidfielder) return 2;
        return -1;

    case SubstituteBucket::LeftMidfielder:
        if (position == PlayerPosition::LeftMidfielder) return 0;
        if (position == PlayerPosition::LeftWinger) return 1;
        if (position == PlayerPosition::LeftBack) return 2;
        return -1;

    case SubstituteBucket::RightMidfielder:
        if (position == PlayerPosition::RightMidfielder) return 0;
        if (position == PlayerPosition::RightWinger) return 1;
        if (position == PlayerPosition::RightBack) return 2;
        return -1;

    case SubstituteBucket::Winger:
        if (position == PlayerPosition::LeftWinger || position == PlayerPosition::RightWinger) return 0;
        if (position == PlayerPosition::LeftMidfielder || position == PlayerPosition::RightMidfielder) return 1;
        if (position == PlayerPosition::AttackingMidfielder) return 2;
        return -1;

    case SubstituteBucket::Striker:
        if (position == PlayerPosition::Striker) return 0;
        if (position == PlayerPosition::AttackingMidfielder) return 2;
        return -1;
    }

    return -1;
}

double bucketScore(const Footballer& player, SubstituteBucket bucket) {
    switch (bucket) {
    case SubstituteBucket::Goalkeeper:
        return calculateConditionAwarePowerForSlot(player, FormationSlotRole::Goalkeeper);
    case SubstituteBucket::CenterBack:
        return calculateConditionAwarePowerForSlot(player, FormationSlotRole::CenterBack);
    case SubstituteBucket::FullBack:
        return std::max(calculateConditionAwarePowerForSlot(player, FormationSlotRole::LeftBack),
                        calculateConditionAwarePowerForSlot(player, FormationSlotRole::RightBack));
    case SubstituteBucket::WingBack:
        return std::max(calculateConditionAwarePowerForSlot(player, FormationSlotRole::LeftWingBack),
                        calculateConditionAwarePowerForSlot(player, FormationSlotRole::RightWingBack));
    case SubstituteBucket::CentralMidfielder:
        return calculateConditionAwarePowerForSlot(player, FormationSlotRole::CentralMidfielder);
    case SubstituteBucket::LeftMidfielder:
        return calculateConditionAwarePowerForSlot(player, FormationSlotRole::LeftMidfielder);
    case SubstituteBucket::RightMidfielder:
        return calculateConditionAwarePowerForSlot(player, FormationSlotRole::RightMidfielder);
    case SubstituteBucket::Winger:
        return std::max(calculateConditionAwarePowerForSlot(player, FormationSlotRole::LeftWinger),
                        calculateConditionAwarePowerForSlot(player, FormationSlotRole::RightWinger));
    case SubstituteBucket::Striker:
        return calculateConditionAwarePowerForSlot(player, FormationSlotRole::Striker);
    }

    return calculateConditionAwarePowerWithoutRoleFit(player);
}

bool isSelectedSubstitute(PlayerId playerId, const std::vector<PlayerId>& selectedSubstitutes) {
    return std::find(selectedSubstitutes.begin(), selectedSubstitutes.end(), playerId) != selectedSubstitutes.end();
}

std::vector<SubstituteCandidate> buildSubstituteCandidates(const Team& team,
                                                           const std::vector<PlayerId>& starterIds) {
    std::unordered_set<PlayerId> starterIdSet;
    starterIdSet.reserve(starterIds.size());
    for (PlayerId starterId : starterIds) {
        starterIdSet.insert(starterId);
    }

    std::vector<SubstituteCandidate> candidates;
    candidates.reserve(team.getPlayers().size());
    for (const auto& player : team.getPlayers()) {
        if (!player || starterIdSet.find(player->getId()) != starterIdSet.end()) {
            continue;
        }

        candidates.push_back(SubstituteCandidate{
            player.get(),
            player->getId(),
            calculateConditionAwarePowerWithoutRoleFit(*player)
        });
    }

    std::sort(candidates.begin(), candidates.end(), [](const SubstituteCandidate& lhs, const SubstituteCandidate& rhs) {
        if (!isNearlyEqual(lhs.overallScore, rhs.overallScore)) {
            return lhs.overallScore > rhs.overallScore;
        }
        return lhs.playerId < rhs.playerId;
    });

    return candidates;
}

bool hasCandidateForBucket(const std::vector<SubstituteCandidate>& candidates,
                           const std::vector<PlayerId>& selectedSubstitutes,
                           SubstituteBucket bucket) {
    return std::any_of(candidates.begin(), candidates.end(), [&](const SubstituteCandidate& candidate) {
        return !isSelectedSubstitute(candidate.playerId, selectedSubstitutes)
            && bucketFitTier(bucket, candidate.player->getPlayerPosition()) >= 0;
    });
}

bool tryAddSubstituteForBucket(const std::vector<SubstituteCandidate>& candidates,
                               SubstituteBucket bucket,
                               std::vector<PlayerId>& selectedSubstitutes,
                               std::size_t targetSubstituteCount) {
    if (selectedSubstitutes.size() >= targetSubstituteCount) {
        return false;
    }

    const SubstituteCandidate* bestCandidate = nullptr;
    for (int tier = 0; tier <= 2 && !bestCandidate; ++tier) {
        for (const SubstituteCandidate& candidate : candidates) {
            if (isSelectedSubstitute(candidate.playerId, selectedSubstitutes)) {
                continue;
            }
            if (bucketFitTier(bucket, candidate.player->getPlayerPosition()) != tier) {
                continue;
            }

            if (!bestCandidate) {
                bestCandidate = &candidate;
                continue;
            }

            const double lhsScore = bucketScore(*candidate.player, bucket);
            const double rhsScore = bucketScore(*bestCandidate->player, bucket);
            if (!isNearlyEqual(lhsScore, rhsScore)) {
                if (lhsScore > rhsScore) {
                    bestCandidate = &candidate;
                }
                continue;
            }

            if (candidate.playerId < bestCandidate->playerId) {
                bestCandidate = &candidate;
            }
        }
    }

    if (!bestCandidate) {
        return false;
    }

    selectedSubstitutes.push_back(bestCandidate->playerId);
    return true;
}

void fillWithBestRemainingSubstitutes(const std::vector<SubstituteCandidate>& candidates,
                                      std::vector<PlayerId>& selectedSubstitutes,
                                      std::size_t targetSubstituteCount) {
    for (const SubstituteCandidate& candidate : candidates) {
        if (selectedSubstitutes.size() >= targetSubstituteCount) {
            return;
        }
        if (isSelectedSubstitute(candidate.playerId, selectedSubstitutes)) {
            continue;
        }

        selectedSubstitutes.push_back(candidate.playerId);
    }
}

std::vector<PlayerId> buildSubstitutePlayerIds(const Team& team,
                                               FormationId formationId,
                                               const std::vector<PlayerId>& starterIds) {
    const std::vector<SubstituteCandidate> candidates = buildSubstituteCandidates(team, starterIds);
    const std::size_t targetSubstituteCount = std::min<std::size_t>(kMaxSubstituteCount, candidates.size());

    std::vector<PlayerId> selectedSubstitutes;
    selectedSubstitutes.reserve(targetSubstituteCount);

    const auto addBucket = [&](SubstituteBucket bucket) {
        (void)tryAddSubstituteForBucket(candidates, bucket, selectedSubstitutes, targetSubstituteCount);
    };
    const auto addBackFourDefender = [&]() {
        if (hasCandidateForBucket(candidates, selectedSubstitutes, SubstituteBucket::FullBack)) {
            addBucket(SubstituteBucket::FullBack);
        } else {
            addBucket(SubstituteBucket::CenterBack);
        }
    };

    addBucket(SubstituteBucket::Goalkeeper);
    addBucket(SubstituteBucket::Goalkeeper);

    switch (formationId) {
    case FormationId::ThreeFiveTwo:
        addBucket(SubstituteBucket::CenterBack);
        addBucket(SubstituteBucket::CenterBack);
        addBucket(SubstituteBucket::CentralMidfielder);
        addBucket(SubstituteBucket::CentralMidfielder);
        addBucket(SubstituteBucket::WingBack);
        addBucket(SubstituteBucket::WingBack);
        addBucket(SubstituteBucket::Striker);
        addBucket(SubstituteBucket::Striker);
        break;

    case FormationId::FourThreeThree:
        addBackFourDefender();
        addBackFourDefender();
        addBucket(SubstituteBucket::CenterBack);
        addBucket(SubstituteBucket::CentralMidfielder);
        addBucket(SubstituteBucket::CentralMidfielder);
        addBucket(SubstituteBucket::Winger);
        addBucket(SubstituteBucket::Winger);
        addBucket(SubstituteBucket::Striker);
        break;

    case FormationId::FourFourTwo:
    default:
        addBackFourDefender();
        addBackFourDefender();
        addBucket(SubstituteBucket::CenterBack);
        addBucket(SubstituteBucket::LeftMidfielder);
        addBucket(SubstituteBucket::RightMidfielder);
        addBucket(SubstituteBucket::CentralMidfielder);
        addBucket(SubstituteBucket::Striker);
        addBucket(SubstituteBucket::Striker);
        break;
    }

    fillWithBestRemainingSubstitutes(candidates, selectedSubstitutes, targetSubstituteCount);
    return selectedSubstitutes;
}

void appendBucketSources(FormationSlotRole slotRole, RoleFitTier tier, std::vector<PlayerPosition>& outSources) {
    outSources.clear();

    switch (slotRole) {
    case FormationSlotRole::Goalkeeper:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::Goalkeeper);
        break;

    case FormationSlotRole::CenterBack:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::CenterBack);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::DefensiveMidfielder);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::RightBack);
        }
        if (tier == RoleFitTier::Emergency) outSources.push_back(PlayerPosition::CentralMidfielder);
        break;

    case FormationSlotRole::LeftBack:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::LeftBack);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::CenterBack);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::RightBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::RightBack:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::RightBack);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::CenterBack);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::LeftWingBack:
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::LeftWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
        }
        break;

    case FormationSlotRole::RightWingBack:
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::RightBack);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
        }
        break;

    case FormationSlotRole::DefensiveMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::DefensiveMidfielder);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::CentralMidfielder);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Emergency) outSources.push_back(PlayerPosition::AttackingMidfielder);
        break;

    case FormationSlotRole::CentralMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::CentralMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::Striker);
        }
        break;

    case FormationSlotRole::AttackingMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::AttackingMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::CentralMidfielder);
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Adapted) outSources.push_back(PlayerPosition::Striker);
        if (tier == RoleFitTier::Emergency) outSources.push_back(PlayerPosition::DefensiveMidfielder);
        break;

    case FormationSlotRole::LeftMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::LeftMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::CentralMidfielder);
            outSources.push_back(PlayerPosition::LeftWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        break;

    case FormationSlotRole::RightMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::RightMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::CentralMidfielder);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightBack);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::LeftWinger);
        }
        break;

    case FormationSlotRole::LeftWinger:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::LeftWinger);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::LeftMidfielder);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::Striker);
            outSources.push_back(PlayerPosition::LeftBack);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::RightWinger:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::RightWinger);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::Striker);
            outSources.push_back(PlayerPosition::RightBack);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::Striker:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::Striker);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::AttackingMidfielder);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        break;

    case FormationSlotRole::Unknown:
        break;
    }
}

void collectTierCandidatesForRole(const NaturalPositionIndex& positionIndex,
                                  FormationSlotRole slotRole,
                                  RoleFitTier targetTier,
                                  std::vector<RankedRoleCandidate>& outCandidates) {
    outCandidates.clear();

    std::vector<PlayerPosition> sourcePositions;
    appendBucketSources(slotRole, targetTier, sourcePositions);

    for (PlayerPosition sourcePosition : sourcePositions) {
        const std::vector<PositionBucketEntry>& sourceBucket = positionIndex.byPosition[static_cast<std::size_t>(sourcePosition)];
        for (const PositionBucketEntry& candidateRef : sourceBucket) {
            const RoleFitTier actualTier = getRoleFitTier(candidateRef.player->getPlayerPosition(), slotRole);
            if (actualTier != targetTier) {
                continue;
            }
            const FitnessAvailabilityBand fitnessAvailability = classifyFitnessAvailability(candidateRef.player->getConditionState().getFitness());
            if (!isAvailableForNormalTier(fitnessAvailability, targetTier)) {
                continue;
            }

            outCandidates.push_back(RankedRoleCandidate{
                candidateRef.player,
                candidateRef.id,
                calculateConditionAwarePowerForSlot(*candidateRef.player, slotRole)
            });
        }
    }

    std::sort(outCandidates.begin(), outCandidates.end(), [](const RankedRoleCandidate& lhs, const RankedRoleCandidate& rhs) {
        if (!isNearlyEqual(lhs.baseSlotScore, rhs.baseSlotScore)) {
            return lhs.baseSlotScore > rhs.baseSlotScore;
        }
        return lhs.playerId < rhs.playerId;
    });
}

void appendUniqueCandidates(std::vector<SlotOption>& options,
                            const std::vector<RankedRoleCandidate>& tierCandidates,
                            RoleFitTier tier,
                            std::size_t cap) {
    std::size_t added = 0;
    for (const RankedRoleCandidate& candidate : tierCandidates) {
        if (added >= cap) {
            break;
        }

        const bool alreadyPresent = std::any_of(options.begin(), options.end(), [&](const SlotOption& existing) {
            return existing.playerId == candidate.playerId;
        });
        if (alreadyPresent) {
            continue;
        }

        options.push_back(SlotOption{ candidate.playerId, candidate.baseSlotScore, tier });
        ++added;
    }
}

void buildDemandAwareExpandedOptions(FormationRoleEntry& entry) {
    entry.expandedOptions.clear();
    entry.expandedOptions.reserve(entry.demand + 2);

    // Demand-aware tier gating: only open lower tiers when higher tiers cannot
    // cover this role demand for the current formation.
    const std::size_t naturalCap = std::min(entry.tierBuckets.natural.size(), entry.demand + 1);
    appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.natural, RoleFitTier::Natural, naturalCap);

    std::size_t covered = entry.expandedOptions.size();
    if (covered < entry.demand) {
        const std::size_t deficit = entry.demand - covered;
        const std::size_t closeCap = std::min(entry.tierBuckets.close.size(), deficit + 1);
        appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.close, RoleFitTier::Close, closeCap);
        covered = entry.expandedOptions.size();
    }

    if (covered < entry.demand) {
        const std::size_t deficit = entry.demand - covered;
        const std::size_t adaptedCap = std::min(entry.tierBuckets.adapted.size(), deficit);
        appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.adapted, RoleFitTier::Adapted, adaptedCap);
        covered = entry.expandedOptions.size();
    }

    if (covered < entry.demand) {
        const std::size_t emergencyCap = std::min<std::size_t>(entry.tierBuckets.emergency.size(), 1);
        appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.emergency, RoleFitTier::Emergency, emergencyCap);
    }

    std::sort(entry.expandedOptions.begin(), entry.expandedOptions.end(), [](const SlotOption& lhs, const SlotOption& rhs) {
        if (!isNearlyEqual(lhs.effectivePower, rhs.effectivePower)) {
            return lhs.effectivePower > rhs.effectivePower;
        }

        const int lhsTierScore = getRoleFitTierScore(lhs.fitTier);
        const int rhsTierScore = getRoleFitTierScore(rhs.fitTier);
        if (lhsTierScore != rhsTierScore) {
            return lhsTierScore > rhsTierScore;
        }

        return lhs.playerId < rhs.playerId;
    });
}

FormationRolePlan buildFormationRolePlan(const std::vector<FormationSlotRole>& templateSlots,
                                         std::size_t slotLimit,
                                         const NaturalPositionIndex& positionIndex) {
    FormationRolePlan rolePlan;
    const std::size_t slotCount = std::min<std::size_t>(slotLimit, templateSlots.size());
    rolePlan.activeSlots.assign(templateSlots.begin(), templateSlots.begin() + slotCount);

    for (FormationSlotRole slotRole : rolePlan.activeSlots) {
        const std::size_t roleIndex = static_cast<std::size_t>(slotRole);
        int& roleEntryIndex = rolePlan.roleToEntryIndex[roleIndex];
        if (roleEntryIndex == -1) {
            roleEntryIndex = static_cast<int>(rolePlan.roleEntries.size());
            rolePlan.roleEntries.push_back(FormationRoleEntry{ slotRole, 1, {}, {} });
        } else {
            ++rolePlan.roleEntries[static_cast<std::size_t>(roleEntryIndex)].demand;
        }
    }

    // Prepare only active roles for this formation; roles not used by this formation
    // are not touched.
    for (FormationRoleEntry& entry : rolePlan.roleEntries) {
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Natural, entry.tierBuckets.natural);
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Close, entry.tierBuckets.close);
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Adapted, entry.tierBuckets.adapted);
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Emergency, entry.tierBuckets.emergency);

        buildDemandAwareExpandedOptions(entry);
    }

    return rolePlan;
}

AssignmentResult buildBestAssignmentForRolePlan(const FormationRolePlan& rolePlan,
                                                std::size_t squadSize) {
    const std::size_t slotCount = rolePlan.activeSlots.size();
    std::vector<std::vector<SlotOption>> slotOptions(slotCount);

    // Reuse shared role-level candidate pools for duplicate slots of the same role.
    for (std::size_t slotIdx = 0; slotIdx < slotCount; ++slotIdx) {
        const FormationSlotRole slotRole = rolePlan.activeSlots[slotIdx];
        const int roleEntryIndex = rolePlan.roleToEntryIndex[static_cast<std::size_t>(slotRole)];
        if (roleEntryIndex < 0) {
            continue;
        }

        slotOptions[slotIdx] = rolePlan.roleEntries[static_cast<std::size_t>(roleEntryIndex)].expandedOptions;
    }

    std::vector<std::size_t> orderedSlotIndices(slotCount);
    std::iota(orderedSlotIndices.begin(), orderedSlotIndices.end(), 0);

    // Scarcity-first internal solving order.
    std::sort(orderedSlotIndices.begin(), orderedSlotIndices.end(), [&slotOptions](std::size_t lhs, std::size_t rhs) {
        if (slotOptions[lhs].size() != slotOptions[rhs].size()) {
            return slotOptions[lhs].size() < slotOptions[rhs].size();
        }
        return lhs < rhs;
    });

    std::vector<double> optimisticBestByDepth(slotCount, 0.0);
    for (std::size_t depth = 0; depth < orderedSlotIndices.size(); ++depth) {
        const std::size_t slotIdx = orderedSlotIndices[depth];
        if (!slotOptions[slotIdx].empty()) {
            optimisticBestByDepth[depth] = slotOptions[slotIdx].front().effectivePower;
        }
    }

    std::vector<double> optimisticSuffix(slotCount + 1, 0.0);
    for (std::size_t depth = slotCount; depth > 0; --depth) {
        optimisticSuffix[depth - 1] = optimisticSuffix[depth] + optimisticBestByDepth[depth - 1];
    }

    AssignmentResult best;
    best.orderedPlayerIdsByTemplate.assign(slotCount, 0);

    std::vector<PlayerId> currentAssigned(slotCount, 0);
    std::vector<PlayerId> usedPlayerIds;
    usedPlayerIds.reserve(slotCount);

    const auto backtrack = [&](auto&& self, std::size_t depth, int assignedCount, double totalPower, int fitScore) -> void {
        if (depth == orderedSlotIndices.size()) {
            AssignmentResult current;
            current.orderedPlayerIdsByTemplate = currentAssigned;
            current.assignedSlotCount = assignedCount;
            current.totalEffectivePower = totalPower;
            current.totalFitScore = fitScore;

            if (isBetterAssignment(current, best)) {
                best = std::move(current);
            }
            return;
        }

        const int remainingSlots = static_cast<int>(orderedSlotIndices.size() - depth);
        const int remainingPlayers = static_cast<int>(squadSize - usedPlayerIds.size());
        const int maxAdditionalAssignments = std::min(remainingSlots, remainingPlayers);
        if (assignedCount + maxAdditionalAssignments < best.assignedSlotCount) {
            return;
        }

        const double optimisticTotalPower = totalPower + optimisticSuffix[depth];
        if (assignedCount + maxAdditionalAssignments == best.assignedSlotCount && optimisticTotalPower + 1e-9 < best.totalEffectivePower) {
            return;
        }

        const std::size_t slotIdx = orderedSlotIndices[depth];
        for (const SlotOption& option : slotOptions[slotIdx]) {
            const bool alreadyUsed = std::find(usedPlayerIds.begin(), usedPlayerIds.end(), option.playerId) != usedPlayerIds.end();
            if (alreadyUsed) {
                continue;
            }

            usedPlayerIds.push_back(option.playerId);
            currentAssigned[slotIdx] = option.playerId;
            self(self,
                 depth + 1,
                 assignedCount + 1,
                 totalPower + option.effectivePower,
                 fitScore + getRoleFitTierScore(option.fitTier));
            currentAssigned[slotIdx] = 0;
            usedPlayerIds.pop_back();
        }

        self(self, depth + 1, assignedCount, totalPower, fitScore);
    };

    backtrack(backtrack, 0, 0, 0.0, 0);
    return best;
}

FormationSelectionCandidate evaluateFormationCandidate(FormationId formation,
                                                        const NaturalPositionIndex& positionIndex,
                                                        std::size_t starterTargetCount,
                                                        std::size_t squadSize) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    const FormationRolePlan rolePlan = buildFormationRolePlan(slotTemplate, starterTargetCount, positionIndex);
    AssignmentResult assignment = buildBestAssignmentForRolePlan(rolePlan, squadSize);

    FormationSelectionCandidate candidate;
    candidate.formation = formation;
    candidate.assignedSlotCount = assignment.assignedSlotCount;
    candidate.totalEffectivePower = assignment.totalEffectivePower;
    candidate.totalFitScore = assignment.totalFitScore;

    candidate.orderedPlayerIdsByTemplate = assignment.orderedPlayerIdsByTemplate;
    const std::size_t slotCount = candidate.orderedPlayerIdsByTemplate.size();
    candidate.orderedAssignments = buildOrderedAssignmentsFromTemplate(formation, candidate.orderedPlayerIdsByTemplate);

    candidate.fullySatisfied = candidate.assignedSlotCount == static_cast<int>(slotCount);
    return candidate;
}
} // namespace

TeamSheet TeamSelectionService::buildTeamSheet(const Team& team) const {
    return buildTeamSheet(team, FormationId::FourThreeThree);
}

TeamSheet TeamSelectionService::buildTeamSheet(const Team& team, FormationId formationId) const {
    const NaturalPositionIndex naturalPositionIndex = buildNaturalPositionIndex(team);
    const std::size_t starterTargetCount = std::min<std::size_t>(11, naturalPositionIndex.squadSize);

    const FormationId selectedFormation = isFormationSupported(formationId)
        ? formationId
        : getSupportedFormationIds().front();

    FormationSelectionCandidate candidate = evaluateFormationCandidate(selectedFormation,
                                                                       naturalPositionIndex,
                                                                       starterTargetCount,
                                                                       naturalPositionIndex.squadSize);

    const bool hasUnusedPlayers = candidate.assignedSlotCount < static_cast<int>(starterTargetCount)
        && candidate.assignedSlotCount < static_cast<int>(naturalPositionIndex.squadSize);
    if (hasUnusedPlayers) {
        completeUnderfilledCandidate(candidate, team, starterTargetCount);
    }

    std::vector<PlayerId> starterIds;
    starterIds.reserve(candidate.orderedAssignments.size());
    for (const TeamSheetSlotAssignment& assignment : candidate.orderedAssignments) {
        starterIds.push_back(assignment.playerId);
    }

    TeamSheet teamSheet;
    teamSheet.teamId = team.getId();
    teamSheet.coachId = team.getHeadCoach().getId();
    teamSheet.formation = candidate.formation;
    teamSheet.startingAssignments = candidate.orderedAssignments;
    teamSheet.startingPlayerIds = starterIds;
    teamSheet.substitutePlayerIds = buildSubstitutePlayerIds(team, candidate.formation, starterIds);
    teamSheet.tacticalSetup = defaultTacticalSetupForTeam(team);
    validateTeamSheetForTeam(teamSheet, team);
    return teamSheet;
}
