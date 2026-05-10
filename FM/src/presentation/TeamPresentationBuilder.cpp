#include "fm/presentation/TeamPresentationBuilder.h"

#include "fm/roster/Team.h"

#include <cctype>
#include <cstdlib>
#include <string>

namespace {
    constexpr const char* fallbackPrimaryColor = "#22c55e";
    constexpr const char* fallbackSecondaryColor = "#0f172a";
    constexpr const char* fallbackTextColor = "#f8fafc";
    constexpr const char* darkTextColor = "#071016";

    std::string trim(std::string value) {
        const auto isNotSpace = [](unsigned char ch) {
            return !std::isspace(ch);
        };

        while (!value.empty() && !isNotSpace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && !isNotSpace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
        return value;
    }

    std::string safeColorText(const std::string& value, const char* fallback) {
        const std::string trimmed = trim(value);
        return trimmed.empty() ? std::string(fallback) : trimmed;
    }

    bool parseHexPair(const std::string& value, std::size_t offset, int& result) {
        if (offset + 2 > value.size()) {
            return false;
        }

        const std::string token = value.substr(offset, 2);
        char* end = nullptr;
        const long parsed = std::strtol(token.c_str(), &end, 16);
        if (!end || *end != '\0' || parsed < 0 || parsed > 255) {
            return false;
        }

        result = static_cast<int>(parsed);
        return true;
    }

    std::string readableTextColorForPrimary(const std::string& primaryColor) {
        const std::string value = trim(primaryColor);
        if (value.size() != 7 || value.front() != '#') {
            return fallbackTextColor;
        }

        int red = 0;
        int green = 0;
        int blue = 0;
        if (!parseHexPair(value, 1, red) || !parseHexPair(value, 3, green) || !parseHexPair(value, 5, blue)) {
            return fallbackTextColor;
        }

        const int luminance = static_cast<int>(
            (0.2126 * red) +
            (0.7152 * green) +
            (0.0722 * blue));
        return luminance >= 110 ? darkTextColor : fallbackTextColor;
    }
}

TeamVisualDto TeamPresentationBuilder::buildTeamVisual(const Team* team) const {
    TeamVisualDto dto;
    dto.teamId = team ? team->getId() : 0;
    dto.name = team ? team->getName() : std::string();
    dto.primaryColor = team ? safeColorText(team->getPrimaryColor(), fallbackPrimaryColor) : fallbackPrimaryColor;
    dto.secondaryColor = team ? safeColorText(team->getSecondaryColor(), fallbackSecondaryColor) : fallbackSecondaryColor;
    dto.textColor = readableTextColorForPrimary(dto.primaryColor);
    return dto;
}
