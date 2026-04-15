#include"FootballerFactory.h"
#include"Midfielder.h"
#include"Goalkeeper.h"
#include"Defender.h"
#include"Forward.h"
#include "PlayerPosition.h"

std::unique_ptr<Footballer> FootballerFactory::create(const std::string& name, int age, const std::string& position, const std::string& team, int s1, int s2, int s3, int s4, int s5) {
    PlayerPosition parsedPosition;
    try {
        parsedPosition = parsePlayerPosition(position);
    }
    catch (...) {
        return nullptr;
    }

    switch (parsedPosition) {
    case PlayerPosition::Goalkeeper:
        return std::make_unique<Goalkeeper>(name, team, age, s1, s2, s3, s4, s5);

    case PlayerPosition::CenterBack:
    case PlayerPosition::LeftBack:
    case PlayerPosition::RightBack:
        return std::make_unique<Defender>(name, parsedPosition, team, age, s1, s2, s3, s4, s5);

    case PlayerPosition::DefensiveMidfielder:
    case PlayerPosition::CentralMidfielder:
    case PlayerPosition::AttackingMidfielder:
    case PlayerPosition::LeftWinger:
    case PlayerPosition::RightWinger:
        return std::make_unique<Midfielder>(name, parsedPosition, team, age, s1, s2, s3, s4, s5);

    case PlayerPosition::Striker:
        return std::make_unique<Forward>(name, parsedPosition, team, age, s1, s2, s3, s4, s5);

    case PlayerPosition::Unknown:
        break;
    }

    return nullptr;
}
