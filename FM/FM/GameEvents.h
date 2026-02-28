#pragma once

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

    //Event'in o    nceligini verir
    virtual EventPriority getPriority() const = 0;

    //Event'in nereyi etkiledigini verir
    virtual EventTargetType getTargetType() const = 0;

    //Event'i gonderen takimin pointerini verir
    virtual Team* getSendingTeam() const { return nullptr; }

    //Event'i alan takimin pointerini verir
    virtual Team* getReceivingTeam() const { return nullptr; }

    //Hedefteki oyuncunun pointerýný verir
    virtual Footballer* getPlayer() const { return nullptr; }

    //Eventten etkilenen takimin kontrolunu yapar
    virtual bool affectsTeam(const Team* team) const = 0;

};