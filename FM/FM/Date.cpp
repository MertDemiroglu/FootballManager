#include"Date.h"

Date::Date(int year, Month month, int day) : month(month), year(year), dayOfWeek(computeDayOfWeek(year, month, day)), day(day) {

}

int Date::getYear() const {
	return year;
}

Month Date::getMonth() const {
	return month;
}

int Date::getDayOfWeek() const {
	return dayOfWeek;
}

int Date::getDay() const {
	return day;
}

bool Date::isLeapYear() const {
	return isLeapYear(year);
}

bool Date::isLeapYear(int y) {
	if (y % 400 == 0) return true;
	if (y % 100 == 0) return false;
	return (y % 4 == 0);
}

int Date::computeDayOfWeek(int y, Month m, int d) {

	int monthInt = static_cast<int>(m);

	static int table[] = { 0,3,2,5,0,3,5,1,4,6,2,4 };

	if (monthInt < 3)
		y -= 1;

	int result = (y + y / 4 - y / 100 + y / 400 + table[monthInt - 1] + d) % 7;

	if (result == 0)
		return 7;

	return result; 
}

int Date::daysInMonth() const {
	return daysInMonth(year, month);
}

int Date::daysInMonth(int y, Month m) {
	switch (m) {
	    case Month::January:   return 31;
		case Month::February:  return isLeapYear(y) ? 29 : 28;
		case Month::March:     return 31;
		case Month::April:     return 30;
		case Month::May:       return 31;
		case Month::June:      return 30;
		case Month::July:      return 31;
		case Month::August:    return 31;
		case Month::September: return 30;
		case Month::October:   return 31;
		case Month::November:  return 30;
		case Month::December:  return 31;
		default: return 30;
	}
}

void Date::advanceDay() {

	day++;

	const int monthDayCount = daysInMonth();

	if (day > monthDayCount) {
		day = 1;

		month = static_cast<Month>(static_cast<int>(month) + 1);
		if (static_cast<int>(month) > 12) {
			month = Month::January;
			year++;
		}
	}
	dayOfWeek++;

	if (dayOfWeek > 7) {
		dayOfWeek = 1;
	}
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

bool Date::isNewWeek() const {
	return dayOfWeek == 1;
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