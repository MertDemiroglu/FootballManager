#pragma once
#include <string>
#include <vector>

#include "Date.h"

class Game;
class EventQueue;

class MatchScheduler {
public:
    //Her gun o gunun maclarini uretir ve event vektorune gonderir
    void update(Game& game, EventQueue& queue);

};