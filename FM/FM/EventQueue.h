#pragma once
#include<vector>

#include"PlayMatchCommand.h"

class GameEvents;

class EventQueue {

public:
    //Queue'ya match command ekler
    void pushCommand(const PlayMatchCommand& command);
    //Queue'dan FIFO sirasiyla match command alir
    PlayMatchCommand popNext();
    //Event queue dolulugu kontrol eder
    bool empty() const;
    //Bekleyen event sayisi
    std::size_t size() const;

private:
	std::vector<PlayMatchCommand> items;
};
