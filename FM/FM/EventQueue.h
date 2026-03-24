#pragma once
#include<vector>
#include<memory>
#include<variant>

#include"PlayMatchCommand.h"

class GameEvents;

class EventQueue {

public:
    //variant farkli tur eventlerin queue'ya alinmasini saglar
	using QueueItem = std::variant<std::unique_ptr<GameEvents>, PlayMatchCommand>;

    //Queue'ya game event ekler
    void pushEvent(std::unique_ptr<GameEvents> event);
    //Queue'ya match command ekler
    void pushCommand(const PlayMatchCommand& command);
    //Queue'dan FIFO sirasiyla item alir
    QueueItem popNext();
    //Event queue dolulugu kontrol eder
    bool empty() const;
    //Bekleyen event sayisi
    std::size_t size() const;

private:
	std::vector<QueueItem> items;
};
