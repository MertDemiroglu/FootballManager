#pragma once

#include "fm/common/Date.h"
#include "fm/data/SavePolicy.h"

#include <optional>

enum class SaveReason {
    Manual,
    ScheduledAutoSave,
    MatchCompleted,
    TransferAccepted,
    TransferOfferResolved,
    TeamSheetChanged,
    SeasonRollover
};

class SaveScheduler {
private:
    AutoSaveFrequency frequency = AutoSaveFrequency::Weekly;
    std::optional<Date> lastAutoSaveDate = std::nullopt;
    bool pendingSave = false;
    std::optional<SaveReason> pendingReason = std::nullopt;

public:
    AutoSaveFrequency getAutoSaveFrequency() const;
    void setAutoSaveFrequency(AutoSaveFrequency newFrequency);

    std::optional<Date> getLastAutoSaveDate() const;
    void setLastAutoSaveDate(std::optional<Date> date);

    void requestSave(SaveReason reason);
    bool hasPendingSave() const;
    std::optional<SaveReason> getPendingReason() const;

    bool shouldScheduledAutosave(const Date& currentDate) const;
    void markSaved(const Date& currentDate);
};

