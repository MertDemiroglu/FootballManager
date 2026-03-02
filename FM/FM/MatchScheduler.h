#pragma once
#include <string>
#include <vector>

#include "Date.h"

class Game;
class EventQueue;

class MatchScheduler {
private:
    int generatedMatchEvents = 0;
public:
    //Her gun o gunun maclarini uretir ve event vektorune gonderir
    void update(Game& game, EventQueue& queue);
    //debug
    int debugGeneratedMatchEvents() const { return generatedMatchEvents; }
};