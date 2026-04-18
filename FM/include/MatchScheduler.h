#pragma once
#include<string>
#include<vector>

#include"fm/common/Date.h"

class World;
class EventQueue;

class MatchScheduler {
private:
    int generatedMatchEvents = 0;
public:
    //Her gun o gunun maclarini uretir ve event vektorune gonderir
    void update(World& world, const Date& currentDate, EventQueue& queue);
    //debug
    int debugGeneratedMatchEvents() const { return generatedMatchEvents; }
};