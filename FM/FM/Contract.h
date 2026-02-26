#pragma once
#include"Types.h"

class Contract {
private:
	Money wage;
	int yearsRemaining;
public:
	//Contract constructor
	Contract(Money wage, int years);
	//Kontrat süresi 1 yýl azaltýr
	void advanceYear();
	//Kontratýn bitip bitmediðini kontrol eder
	bool isExpired() const;
	//Yýllýk maaþý döner
	Money getWage() const;
};
