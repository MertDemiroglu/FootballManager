#pragma once
#include <vector>
#include <memory>


class GameEvents;

class EventQueue {
private:
	std::vector<std::unique_ptr<GameEvents>> events;
public:
	
	//Queue'ya event ekler
	void pushEvent(std::unique_ptr<GameEvents> event);
    //Queue'dan event alir
	std::unique_ptr<GameEvents> popEvent();
	//Event queue dolulugu kontrol eder
	bool empty() const;
	//Bekleyen event sayisi
	std::size_t size() const;
};
