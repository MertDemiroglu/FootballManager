#include"FootballerFactory.h"
#include"Midfielder.h"
#include"Goalkeeper.h"
#include"Defender.h"
#include"Forward.h"

std::unique_ptr<Footballer> FootballerFactory::create(const std::string& name, int age, const std::string& position, const std::string& team, int s1, int s2, int s3, int s4, int s5) {
    if (position == "Goalkeeper")
        return std::make_unique<Goalkeeper>(name, team, age, s1, s2, s3, s4, s5);

    else if (position == "Defender")
        return std::make_unique<Defender>(name, team, age, s1, s2, s3, s4, s5);

    else if (position == "Midfielder")
        return std::make_unique<Midfielder>(name, team, age, s1, s2, s3, s4, s5);

    else if (position == "Forward")
        return std::make_unique<Forward>(name, team, age, s1, s2, s3, s4, s5);

    return nullptr;
}