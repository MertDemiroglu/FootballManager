#pragma once
#include<string>
class Game;
class Team;
class Footballer;

enum class EventTargetType {
    League, Team, Player
};

enum class EventPriority {
    Critical = 100, High = 80, Medium = 50, Low = 20
};

class GameEvents {
public:

    //Virtual destructor
    virtual ~GameEvents() = default;

    //Event'in oyun akisini durdurup durdurmadigini kontrol eder
    virtual bool isBlocking() const = 0;

    //Event cozumu
    virtual void resolve(Game& game) = 0;

    //Event'in onceligini verir
    virtual EventPriority getPriority() const = 0;

    //Event'in nereyi etkiledigini verir
    virtual EventTargetType getTargetType() const = 0;

    //Event'i gonderen takimin pointerini verir
    virtual const std::string&  getSendingTeam() const = 0;

    //Event'i alan takimin pointerini verir
    virtual const std::string& getReceivingTeam() const = 0;;

    //Hedefteki oyuncunun pointerini verir
    virtual const std::string&  getPlayer() const = 0;;

    //Eventten etkilenen takimin kontrolunu yapar
    virtual bool affectsTeam(const Team* team) const = 0;

};