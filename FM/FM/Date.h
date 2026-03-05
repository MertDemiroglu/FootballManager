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
	Date(int year, Month month, int day);
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

	//Haftanin hangi gunu oldugunu otomatik hesaplar
	static int computeDayOfWeek(int year, Month month, int day);

	//leap year kontrolu
	bool isLeapYear() const;
	//leap year kontrolu
	static bool isLeapYear(int year);

	//O aydaki gun sayisini verir
	int daysInMonth() const;
	//O aydaki gun sayisini verir
	static int daysInMonth(int year, Month month);

	//Map'in tarih siralamasi icin kullanacagi operator
	bool operator<(const Date& other) const;
};