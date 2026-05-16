#include "fm/data/SavePolicy.h"

#include <stdexcept>

std::string_view toStableCode(AutoSaveFrequency frequency) {
    switch (frequency) {
    case AutoSaveFrequency::ManualOnly:
        return "manual_only";
    case AutoSaveFrequency::Daily:
        return "daily";
    case AutoSaveFrequency::Every3Days:
        return "every_3_days";
    case AutoSaveFrequency::Weekly:
        return "weekly";
    case AutoSaveFrequency::Every2Weeks:
        return "every_2_weeks";
    }

    throw std::runtime_error("unsupported auto save frequency");
}

std::optional<AutoSaveFrequency> autoSaveFrequencyFromStableCode(std::string_view stableCode) {
    if (stableCode == "manual_only") {
        return AutoSaveFrequency::ManualOnly;
    }
    if (stableCode == "daily") {
        return AutoSaveFrequency::Daily;
    }
    if (stableCode == "every_3_days") {
        return AutoSaveFrequency::Every3Days;
    }
    if (stableCode == "weekly") {
        return AutoSaveFrequency::Weekly;
    }
    if (stableCode == "every_2_weeks") {
        return AutoSaveFrequency::Every2Weeks;
    }
    return std::nullopt;
}

std::string_view toDisplayText(AutoSaveFrequency frequency) {
    switch (frequency) {
    case AutoSaveFrequency::ManualOnly:
        return "Manual only";
    case AutoSaveFrequency::Daily:
        return "Daily";
    case AutoSaveFrequency::Every3Days:
        return "Every 3 days";
    case AutoSaveFrequency::Weekly:
        return "Weekly";
    case AutoSaveFrequency::Every2Weeks:
        return "Every 2 weeks";
    }

    throw std::runtime_error("unsupported auto save frequency");
}

int intervalDays(AutoSaveFrequency frequency) {
    switch (frequency) {
    case AutoSaveFrequency::ManualOnly:
        return 0;
    case AutoSaveFrequency::Daily:
        return 1;
    case AutoSaveFrequency::Every3Days:
        return 3;
    case AutoSaveFrequency::Weekly:
        return 7;
    case AutoSaveFrequency::Every2Weeks:
        return 14;
    }

    throw std::runtime_error("unsupported auto save frequency");
}

