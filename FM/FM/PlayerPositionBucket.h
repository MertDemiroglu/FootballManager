#pragma once

#include <string>


enum class PlayerPositionBucket {
    Goalkeeper,
    Defender,
    Midfielder,
    Forward,
    Unknown
};

PlayerPositionBucket mapPlayerPositionToBucket(const std::string& position);
