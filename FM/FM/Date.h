#pragma once

//statelerin belirlenmesi için aylar kullanýlacak
enum class Month {
	January = 1, February, March, April, May, June,July, August, September, October, November, December
};
class Date {
private:
	Month month;
	int year, dayOfWeek, day;


public:
	//Date constructor 
	Date(int year, Month month, int day, int dayOfWeek);
	//Yil'i veri
	int getYear() const;
	//Ay'i verir
	Month getMonth() const;
	//Haftanin hangi gunu oldugunu verir
	int getDayOfWeek() const;
	//Gün döndürür
	int getDay() const;
	

	//gün ilerletir arttýrýr bu sýrada ay ve yýlýn artmasýnýda saðlar
	void advanceDay();

	//Yaz transfer döneminin baþlayýp baþlamadýðýnýn kontrolü
	bool isSummerTransferWindow() const;
	//Kýþ transfer döneminin baþlayýp baþlamadýðýnýn kontrolü
	bool isWinterTransferWindow() const;

	//Yeni bir aya girilip girilmediðinin kontrolü (Yillik eventler icin)
	bool isNewMonth() const;
	//Yeni yil kontrolu (Yillik eventler icin)
	bool isNewYear() const;
	//Yeni hafta kontrolu (Haftalik eventler icin)
	bool isNewWeek() const;


	bool isLeapYear() const;
	static bool isLeapYear(int year);
	int daysInMonth() const;
	static int daysInMonth(int year, Month month);

	//Map'in tarih siralamasi icin kullanacagi operator
	bool operator<(const Date& other) const;
};