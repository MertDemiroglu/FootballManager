#pragma once

class Game;
class Team;
class Footballer;
class Types;

enum class EventTargetType {
    League, Team, Player
};

enum class EventPriority {
    Critical = 100, High = 80, Medium = 50, Low = 20
};
class GameEvents {
public:

    //Event'in oyun akýþýný durdurup durdurmadýðýný kontrol eder
    virtual bool isBlocking() const = 0;

    //Problemin çözümü
    virtual void resolve(Game& game) = 0;

    //Event'in önceliðini döndürür
    virtual EventPriority getPriority() const = 0;

    //Virtual destructor
    virtual ~GameEvents() = default;

    //Event'in nereyi etkilediðini döndürür
    virtual EventTargetType getTargetType() const = 0;

    //Event'i gönderen takýmun pointerýný döndürür
    virtual Team* getSendingTeam() const = 0;

    //Event'i alan takýmýn pointerýný döndürür
    virtual Team* getReceivingTeam() const = 0;

    //Hedefteki oyuncunun pointerýný verir
    virtual Footballer* getPlayer() const = 0;

    //Eventten etkilenen takýmýn kontrolünü yapar
    virtual bool affectsTeam(const Team* team) const = 0;
};