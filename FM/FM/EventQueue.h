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
    //Queue'dan event alýr
	std::unique_ptr<GameEvents> popEvent();
	//Event queue doluluðu kontrol eder
	bool empty() const;

};
