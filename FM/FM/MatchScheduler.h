#pragma once
#include <string>
#include <vector>

#include "Date.h"

class Game;
class EventQueue;

class MatchScheduler {
public:

    void update(Game& game, EventQueue& queue);

};