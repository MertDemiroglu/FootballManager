#pragma once

//statelerin belirlenmesi için aylar kullanýlacak
enum class Month {
	January = 1, February, March, April, May, June,July, August, September, October, November, December
};
class Date {
private:
	Month month;
	int year, week, day;


public:
	//Date constructor 
	Date(int year, Month month, int week, int day);
	//Yýl döndürür
	int getYear() const;
	//Ay döndürür
	Month getMonth() const;
	//Hafta döndürür
	int getWeek() const;
	//Gün döndürür
	int getDay() const;
	

	//gün ilerletir arttýrýr bu sýrada ay ve yýlýn artmasýnýda saðlar
	void advanceDay();

	//Yaz transfer döneminin baþlayýp baþlamadýðýnýn kontrolü
	bool isSummerTransferWindow() const;
	//Kýþ transfer döneminin baþlayýp baþlamadýðýnýn kontrolü
	bool isWinterTransferWindow() const;

	//Yeni bir aya girilip girilmediðinin kontrolü (Yylýk eventler için)
	bool isNewMonth() const;
	//Yeni yýl kontrolü (Yýllýk eventler için)
	bool isNewYear() const;
};