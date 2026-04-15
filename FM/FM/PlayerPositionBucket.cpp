#include "PlayerPositionBucket.h"

#include <algorithm>
#include <cctype>

namespace {
    std::string normalizePosition(const std::string& value) {
        std::string normalized;
        normalized.reserve(value.size());

        for (unsigned char c : value) {
            if (!std::isspace(c)) {
                normalized.push_back(static_cast<char>(std::toupper(c)));
            }
        }

        return normalized;
    }
}

PlayerPositionBucket mapPlayerPositionToBucket(const std::string& position) {
    const std::string normalized = normalizePosition(position);

    if (normalized == "GK" || normalized == "GOALKEEPER") {
        return PlayerPositionBucket::Goalkeeper;
    }

    if (normalized == "DEF" || normalized == "DF" || normalized == "DEFENDER") {
        return PlayerPositionBucket::Defender;
    }

    if (normalized == "MID" || normalized == "MF" || normalized == "MIDFIELDER") {
        return PlayerPositionBucket::Midfielder;
    }

    if (normalized == "FWD" || normalized == "FW" || normalized == "FORWARD") {
        return PlayerPositionBucket::Forward;
    }

    return PlayerPositionBucket::Unknown;
}
