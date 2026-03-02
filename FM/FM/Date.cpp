#include"Date.h"

Date::Date(int year, Month month, int week, int day) : year(year), month(month), week(week), day(day){}

int Date::getYear() const {
	return year;
}
Month Date::getMonth() const {
	return month;
}
int Date::getWeek() const {
	return week;
}
int Date::getDay() const {
	return day;
}
void Date::advanceDay() {
	day++;
	if (day > 30) {
		day = 1;
		month = static_cast<Month>(static_cast<int>(month) + 1);
		if (static_cast<int>(month) > 12) {
			month = Month::January;
			year++;
		}
	}
	week = (day - 1) / 7 + 1;
}
bool Date::isSummerTransferWindow() const {
	if (month == Month::June || month == Month::July || month == Month::August) {
		return 1;
	}
	return 0;
}
bool Date::isWinterTransferWindow() const {
	if (month == Month::January) {
		return 1;
	}
	return 0;
}
bool Date::isNewMonth() const {
	if (day == 1) {
		return 1;
	}
	return 0;
}
bool Date::isNewYear() const {
	if (day == 1 && month == Month::January) {
		return 1;
	}
	return 0;
}

bool Date::operator<(const Date& other) const {
	if (year != other.year) {
		return year < other.year;
	}
	if (month != other.month) {
		return static_cast<int>(month) < static_cast<int>(other.month);
	}

	return day < other.day;
}