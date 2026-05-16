#pragma once

#include <optional>
#include <string>
#include <string_view>

enum class AutoSaveFrequency {
    ManualOnly,
    Daily,
    Every3Days,
    Weekly,
    Every2Weeks
};

std::string_view toStableCode(AutoSaveFrequency frequency);
std::optional<AutoSaveFrequency> autoSaveFrequencyFromStableCode(std::string_view stableCode);
std::string_view toDisplayText(AutoSaveFrequency frequency);
int intervalDays(AutoSaveFrequency frequency);

