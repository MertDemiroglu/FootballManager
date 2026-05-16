#include "fm/core/SaveScheduler.h"

namespace {
    int daysBeforeYear(int year) {
        const int completedYears = year - 1;
        return completedYears * 365 + completedYears / 4 - completedYears / 100 + completedYears / 400;
    }

    int dayOfYear(const Date& date) {
        int result = date.getDay();
        for (int month = 1; month < static_cast<int>(date.getMonth()); ++month) {
            result += Date::daysInMonth(date.getYear(), static_cast<Month>(month));
        }
        return result;
    }

    int serialDay(const Date& date) {
        return daysBeforeYear(date.getYear()) + dayOfYear(date);
    }
}

AutoSaveFrequency SaveScheduler::getAutoSaveFrequency() const {
    return frequency;
}

void SaveScheduler::setAutoSaveFrequency(AutoSaveFrequency newFrequency) {
    frequency = newFrequency;
}

std::optional<Date> SaveScheduler::getLastAutoSaveDate() const {
    return lastAutoSaveDate;
}

void SaveScheduler::setLastAutoSaveDate(std::optional<Date> date) {
    lastAutoSaveDate = date;
}

void SaveScheduler::requestSave(SaveReason reason) {
    pendingSave = true;
    pendingReason = reason;
}

bool SaveScheduler::hasPendingSave() const {
    return pendingSave;
}

std::optional<SaveReason> SaveScheduler::getPendingReason() const {
    return pendingReason;
}

bool SaveScheduler::shouldScheduledAutosave(const Date& currentDate) const {
    const int days = intervalDays(frequency);
    if (days <= 0) {
        return false;
    }
    if (!lastAutoSaveDate.has_value()) {
        return true;
    }

    return serialDay(currentDate) - serialDay(*lastAutoSaveDate) >= days;
}

void SaveScheduler::markSaved(const Date& currentDate) {
    lastAutoSaveDate = currentDate;
    pendingSave = false;
    pendingReason = std::nullopt;
}

