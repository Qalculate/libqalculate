/*
    Qalculate    

    Copyright (C) 2018  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "QalculateDateTime.h"
#include "support.h"
#include "util.h"
#include "Calculator.h"

#define SECONDS_PER_DAY 86400

bool isLeapYear(long int year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
int daysPerYear(long int year, int basis = 1) {
	switch(basis) {
		case 0: {
			return 360;
		}
		case 1: {
			if(isLeapYear(year)) {
				return 366;
			} else {
				return 365;
			}
		}
		case 2: {
			return 360;
		}		
		case 3: {
			return 365;
		} 
		case 4: {
			return 360;
		}
	}
	return -1;
}

int daysPerMonth(int month, long int year) {
	switch(month) {
		case 1: {} case 3: {} case 5: {} case 7: {} case 8: {} case 10: {} case 12: {
			return 31;
		}
		case 2:	{
			if(isLeapYear(year)) return 29;
			else return 28;
		}
		default: {
			return 30;
		}
	}
}

QalculateDateTime::QalculateDateTime() : i_year(0), i_month(1), i_day(1), b_time(false) {}
QalculateDateTime::QalculateDateTime(long int initialyear, int initialmonth, int initialday) : i_year(0), i_month(1), i_day(1), b_time(false) {set(initialyear, initialmonth, initialday);}
QalculateDateTime::QalculateDateTime(const Number &initialtimestamp) : i_year(0), i_month(1), i_day(1), b_time(false) {set(initialtimestamp);}
QalculateDateTime::QalculateDateTime(string date_string) : i_year(0), i_month(1), i_day(1), b_time(false) {set(date_string);}
QalculateDateTime::QalculateDateTime(const QalculateDateTime &date) : i_year(date.year()), i_month(date.month()), i_day(date.day()), n_time(date.timeValue()), b_time(date.timeIsSet()) {}
void QalculateDateTime::setToCurrentDate() {
	set((long int) ::time(NULL));
}
bool QalculateDateTime::operator > (const QalculateDateTime &date2) const {
	if(i_year != date2.year()) return i_year > date2.year();
	if(i_month != date2.month()) return i_month > date2.month();
	return i_day > date2.day();
}
bool QalculateDateTime::operator < (const QalculateDateTime &date2) const {
	if(i_year != date2.year()) return i_year < date2.year();
	if(i_month != date2.month()) return i_month < date2.month();
	return i_day < date2.day();
}
bool QalculateDateTime::operator >= (const QalculateDateTime &date2) const {
	return !(*this < date2);
}
bool QalculateDateTime::operator <= (const QalculateDateTime &date2) const {
	return !(*this > date2);
}
bool QalculateDateTime::operator != (const QalculateDateTime &date2) const  {
	return i_year != date2.year() || i_month != date2.month() || i_day > date2.day();
}
bool QalculateDateTime::operator == (const QalculateDateTime &date2) const {
	return i_year == date2.year() && i_month == date2.month() && i_day == date2.day();
}
bool QalculateDateTime::isFutureDate() const {
	QalculateDateTime current_date;
	current_date.setToCurrentDate();
	return *this > current_date;
}
bool QalculateDateTime::isPastDate() const {
	QalculateDateTime current_date;
	current_date.setToCurrentDate();
	return *this < current_date;
}
bool QalculateDateTime::set(long int newyear, int newmonth, int newday) {
	if(newmonth < 1 || newmonth > 12) return false;
	if(newday < 1 || newday > daysPerMonth(newmonth, newyear)) return false;
	i_year = newyear;
	i_month = newmonth;
	i_day = newday;
	return true;
}
bool QalculateDateTime::set(const Number &newtimestamp) {
	i_year = 1970;
	i_month = 1;
	i_day = 1;
	b_time = true;
	return addSeconds(newtimestamp);
}
bool QalculateDateTime::set(string str) {

	long int newyear = 0, newmonth = 0, newday = 0;

	remove_blank_ends(str);
	if(equalsIgnoreCase(str, _("now")) || equalsIgnoreCase(str, "now")) {
		set((long int) ::time(NULL));
		return true;
	} else if(equalsIgnoreCase(str, _("today")) || equalsIgnoreCase(str, "today")) {
		struct tm tmdate;
		time_t rawtime;
		::time(&rawtime);
		tmdate = *localtime(&rawtime);
		b_time = false;
		newyear = tmdate.tm_year + 1900;
		newmonth = tmdate.tm_mon + 1;
		newday = tmdate.tm_mday;
		return set(newyear, newmonth, newday);
	} else if(equalsIgnoreCase(str, _("tomorrow")) || equalsIgnoreCase(str, "tomorrow")) {
		struct tm tmdate;
		time_t rawtime;
		::time(&rawtime);
		tmdate = *localtime(&rawtime);
		b_time = false;
		newyear = tmdate.tm_year + 1900;
		newmonth = tmdate.tm_mon + 1;
		newday = tmdate.tm_mday;
		if(set(newyear, newmonth, newday)) return false;
		addDays(1);
		return true;
	} else if(equalsIgnoreCase(str, _("yesterday")) || equalsIgnoreCase(str, "yesterday")) {
		struct tm tmdate;
		time_t rawtime;
		::time(&rawtime);
		tmdate = *localtime(&rawtime);
		b_time = false;
		newyear = tmdate.tm_year + 1900;
		newmonth = tmdate.tm_mon + 1;
		newday = tmdate.tm_mday;
		if(set(newyear, newmonth, newday)) return false;
		addDays(-1);
		return true;
	}
	
	if(sscanf(str.c_str(), "%ld-%lu-%lu", &newyear, &newmonth, &newday) != 3) {
		if(sscanf(str.c_str(), "%4ld%2lu%2lu", &newyear, &newmonth, &newday) != 3) {
#ifndef _WIN32
			struct tm tmdate;
			if(strptime(str.c_str(), "%x", &tmdate) || strptime(str.c_str(), "%Ex", &tmdate)) {
				newyear = tmdate.tm_year + 1900;
				newmonth = tmdate.tm_mon + 1;
				newday = tmdate.tm_mday;
			} else {
#endif
				if(sscanf(str.c_str(), "%ld/%ld/%ld", &newmonth, &newday, &newyear) != 3) {
					if(sscanf(str.c_str(), "%2ld%2lu%2lu", &newyear, &newmonth, &newday) != 3) {
						char c1, c2;
						if(sscanf(str.c_str(), "%ld%1c%ld%1c%ld", &newday, &c1, &newmonth, &c2, &newyear) != 5) {
							return false;
						}
					}
					if(newday > 31) {
						long int i = newday;
						newday = newyear;
						newyear = i;
					}
					if(newmonth > 12) {
						long int i = newday;
						newday = newmonth;
						newmonth = i;
					}
				}
				if(newmonth > 12) {
					long int i = newday;
					newday = newmonth;
					newmonth = i;
				}
				if(newday > 31) {
					long int i = newday;
					newday = newyear;
					newyear = i;
				}
				time_t rawtime;
				::time(&rawtime);
				if(newyear >= 0 && newyear < 100) {
					if(newyear + 70 > localtime(&rawtime)->tm_year) newyear += 1900;
					else newyear += 2000;
				}
			}
#ifndef _WIN32
		}
#endif
	}
	return set(newyear, newmonth, newday);
}
void QalculateDateTime::set(const QalculateDateTime &date) {
	i_year = date.year();
	i_month = date.month();
	i_day = date.day();
	n_time = date.timeValue();
}
string QalculateDateTime::toISOString() const {
	string str = i2s(i_year);
	str += "-";
	if(i_month < 10) {
		str += "0";
	}
	str += i2s(i_month);
	str += "-";
	if(i_day < 10) {
		str += "0";
	}
	str += i2s(i_day);
	if(b_time || !n_time.isZero()) {
		str += "T";
		Number n_rem(n_time);
		n_rem.round();
		Number n_hour(n_rem);
		n_hour /= 3600;
		n_hour.trunc();
		if(n_hour.isLessThan(10)) str += "0";
		str += n_hour.print();
		n_hour *= 3600;
		n_rem -= n_hour;
		Number n_min(n_rem);
		n_min /= 60;
		n_min.trunc();
		str += ":";
		if(n_min.isLessThan(10)) str += "0";
		str += n_min.print();
		n_min *= 60;
		n_rem -= n_min;
		if(!n_rem.isZero()) {
			str += ":";
			if(n_rem.isLessThan(10)) str += "0";
			str += n_rem.print();
		}
		str += "Z" << endl;
	}
	return str;
}
string QalculateDateTime::toLocalString() const {
	struct tm tmdate;
	tmdate.tm_year = i_year - 1900;
	tmdate.tm_mon = i_month - 1;
	tmdate.tm_mday = i_day;
	Number ntimei(n_time);
	ntimei.trunc();
	if(b_time) {
		long int i_time = ntimei.lintValue();
		tmdate.tm_hour = i_time / 3600;
		i_time = i_time % 3600;
		tmdate.tm_min = i_time / 60;
		i_time = i_time % 60;
		tmdate.tm_sec = i_time;
	}
	char *buffer = (char*) malloc(100 * sizeof(char));
	if(!strftime(buffer, 100, "%xT%XZ", &tmdate)) {
		return toISOString();
	}
	string str = buffer;
	free(buffer);
	return str;
}
string QalculateDateTime::print(const PrintOptions &po) const {
	if(po.is_approximate && !n_time.isInteger()) *po.is_approximate = true;
	if(po.date_time_format == DATE_TIME_FORMAT_LOCALE) return toLocalString();
	return toISOString();
}
long int QalculateDateTime::year() const {return i_year;}
long int QalculateDateTime::month() const {return i_month;}
long int QalculateDateTime::day() const {return i_day;}
const Number &QalculateDateTime::timeValue() const {return n_time;}
bool QalculateDateTime::timeIsSet() const {return b_time;}
bool QalculateDateTime::addDays(const Number &ndays) {
	if(!ndays.isReal() || ndays.isInterval()) return false;
	if(!ndays.isInteger()) {
		Number newdays(ndays);
		newdays.trunc();
		QalculateDateTime dtbak(*this);
		if(!addDays(newdays)) return false;
		Number nsec(ndays);
		nsec.frac();
		nsec *= SECONDS_PER_DAY;
		if(!addSeconds(nsec)) {
			set(dtbak);
			return false;
		}
		return true;
	}
	bool overflow;
	long int days = ndays.lintValue(&overflow);
	if(overflow) return false;
	long int newday = i_day, newmonth = i_month, newyear = i_year;
	if(days > 0) {
		if(i_day > 0 && (unsigned long int) days + i_day > (unsigned long int) LONG_MAX) return false;
		newday += days;
		bool check_aborted = days > 1000000L;
		while(newday > daysPerYear(newyear)) {
			newday -= daysPerYear(newyear);
			newyear++;
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newday > daysPerMonth(newmonth, newyear)) {
			newday -= daysPerMonth(newmonth, newyear);
			newmonth++;
			if(newmonth > 12) {
				newyear++;
				newmonth = 1;
			}
		}
	} else if(days < 0) {
		newday += days;
		bool check_aborted = days < -1000000L;
		while(-newday > daysPerYear(newyear - 1)) {
			newyear--;
			newday += daysPerYear(newyear);
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newday < 1) {
			newmonth--;
			if(newmonth < 1) {
				newyear--;
				newmonth = 12;
			}
			newday += daysPerMonth(newmonth, newyear);
		}
	}
	i_day = newday;
	i_month = newmonth;
	i_year = newyear;
	return true;
}
bool QalculateDateTime::addMonths(const Number &nmonths) {
	if(!nmonths.isReal() || nmonths.isInterval()) return false;
	if(!nmonths.isInteger()) {
		Number newmonths(nmonths);
		newmonths.trunc();
		QalculateDateTime dtbak(*this);
		if(!addYears(newmonths)) return false;
		Number nday(nmonths);
		nday.frac();
		if(nday.isNegative()) {
			if(nday.isLessThan(i_day - 1)) {
				nday *= daysPerYear(i_year);
			} else {
				Number idayfrac(i_day - 1);
				Number secfrac(n_time);
				secfrac /= 86400;
				idayfrac += secfrac;
				idayfrac /= daysPerMonth(i_month, i_year); 
				nday -= idayfrac;
				nday *= daysPerMonth(i_month == 1 ? 12 : i_month - 1, i_year);
				idayfrac *= daysPerMonth(i_month, i_year);
				nday += idayfrac;
			}
		} else {
			if(nday.isLessThan(daysPerYear(i_year) - i_day)) {
				nday *= daysPerYear(i_year);
			} else {
				Number idayfrac(i_day - 1);
				Number secfrac(n_time);
				secfrac /= 86400;
				idayfrac -= secfrac;
				idayfrac /= daysPerMonth(i_month, i_year); 
				nday -= idayfrac;
				nday *= daysPerMonth(i_month == 12 ? 1 : i_month + 1, i_year);
				idayfrac *= daysPerMonth(i_month, i_year);
				nday += idayfrac;
			}
		}
		if(!addDays(nday)) {
			set(dtbak);
			return false;
		}
		return true;
	}
	bool overflow;
	long int months = nmonths.lintValue(&overflow);
	if(overflow) return false;
	if(i_year > 0 && months > 0 && (unsigned long int) months / 12 + i_year > (unsigned long int) LONG_MAX) return false;
	if(i_year < 0 && months < 0 && (unsigned long int) (-months / 12) - i_year > (unsigned long int) LONG_MAX) return false;
	i_year += months / 12;
	i_month += months % 12;
	if(i_month > 12) {
		i_month -= 12;
		i_year += 1;
	} else if(i_month < 1) {
		i_month += 12;
		i_year -= 1;
	}
	if(i_day > daysPerMonth(i_month, i_year)) {
		i_day -= daysPerMonth(i_month, i_year);
		i_month++;
		if(i_month > 12) {
			i_month -= 12;
			i_year += 1;
		}
	}
	return true;
}
bool QalculateDateTime::addYears(const Number &nyears) {
	if(!nyears.isReal() || nyears.isInterval()) return false;
	if(!nyears.isInteger()) {
		Number newyears(nyears);
		newyears.trunc();
		QalculateDateTime dtbak(*this);
		if(!addYears(newyears)) return false;
		Number nday(nyears);
		nday.frac();
		long int idoy = yearday();
		if(nday.isNegative()) {
			if(nday.isLessThan(idoy - 1)) {
				nday *= daysPerYear(i_year);
			} else {
				Number idayfrac(idoy - 1);
				Number secfrac(n_time);
				secfrac /= 86400;
				idayfrac += secfrac;
				idayfrac /= daysPerYear(i_year); 
				nday -= idayfrac;
				nday *= daysPerYear(i_year - 1);
				idayfrac *= daysPerYear(i_year);
				nday += idayfrac;
			}
		} else {
			if(nday.isLessThan(daysPerYear(i_year) - idoy)) {
				nday *= daysPerYear(i_year);
			} else {
				Number idayfrac(idoy - 1);
				Number secfrac(n_time);
				secfrac /= 86400;
				idayfrac -= secfrac;
				idayfrac /= daysPerYear(i_year); 
				nday -= idayfrac;
				nday *= daysPerYear(i_year + 1);
				idayfrac *= daysPerYear(i_year);
				nday += idayfrac;
			}
		}
		if(!addDays(nday)) {
			set(dtbak);
			return false;
		}
		return true;
	}
	bool overflow;
	long int years = nyears.lintValue(&overflow);
	if(overflow) return false;
	if(i_year > 0 && years > 0 && (unsigned long int) years + i_year > (unsigned long int) LONG_MAX) return false;
	if(i_year < 0 && years < 0 && (unsigned long int) (-years) - i_year > (unsigned long int) LONG_MAX) return false;
	i_year += years;
	if(i_day > daysPerMonth(i_month, i_year)) {
		i_day -= daysPerMonth(i_month, i_year);
		i_month++;
		if(i_month > 12) {
			i_month -= 12;
			i_year += 1;
		}
	}
	return true;
}
bool QalculateDateTime::addSeconds(const Number &seconds) {
	Number new_time(n_time);
	if(!new_time.add(seconds)) return false;
	if(new_time.isNegative()) {
		if(new_time.isLessThan(-SECONDS_PER_DAY)) {
			Number ndays(new_time);
			ndays /= SECONDS_PER_DAY;
			ndays.trunc();
			bool overflow = false;
			long int new_days = ndays.lintValue(&overflow);
			if(overflow) return false;
			ndays *= SECONDS_PER_DAY;
			new_time -= ndays;
			if(!new_time.isZero()) {
				if(new_days == LONG_MIN) return false;
				new_days--;
			}
			if(!addDays(new_days)) return false;
		} else {
			if(!addDays(-1)) return false;
		}
		if(!new_time.isZero()) {
			new_time = SECONDS_PER_DAY;
			new_time.add(new_time);
		}
	} else if(new_time.isGreaterThanOrEqualTo(SECONDS_PER_DAY)) {
		Number ndays(new_time);
		ndays /= SECONDS_PER_DAY;
		ndays.trunc();
		bool overflow = false;
		long int new_days = ndays.lintValue(&overflow);
		if(overflow || !addDays(new_days)) return false;
		ndays *= SECONDS_PER_DAY;
		new_time -= ndays;
	}
	n_time = new_time;
	b_time = true;
	return true;
}
bool QalculateDateTime::add(const QalculateDateTime &date) {
	QalculateDateTime dtbak(*this);
	if(!addYears(date.year()) || !addMonths(date.month()) || !addDays(date.day()) || !addSeconds(date.timeValue())) {
		set(dtbak);
		return false;
	}
	return true;
}
int QalculateDateTime::weekday() const {
	Number nr(daysTo(QalculateDateTime(2017, 7, 31)));
	if(nr.isInfinite()) return -1;
	nr.negate();
	nr.rem(Number(7, 1));
	if(nr.isNegative()) return 8 + nr.intValue();
	return nr.intValue() + 1;
}
int QalculateDateTime::week(bool start_sunday) const {
	if(start_sunday) {
		int yday = yearday();
		QalculateDateTime date1(i_year, 1, 1);
		int wday = date1.weekday() + 1;
		if(wday < 0) return -1;
		if(wday == 8) wday = 1;
		yday += (wday - 2);
		int week = yday / 7 + 1;
		if(week > 52) week = 1;
		return week;
	}
	if(i_month == 12 && i_day >= 29 && weekday() <= i_day - 28) {
		return 1;
	} else {
		QalculateDateTime date(i_year, i_month, i_day);
		week_rerun:
		int week1;
		int day1 = date.yearday();
		QalculateDateTime date1(date.year(), 1, 1);
		int wday = date1.weekday();
		if(wday < 0) return -1;
		day1 -= (8 - wday);
		if(wday <= 4) {
			week1 = 1;
		} else {
			week1 = 0;
		}
		if(day1 > 0) {
			day1--;
			week1 += day1 / 7 + 1;
		}
		if(week1 == 0) {
			date.set(date.year() - 1, 12, 31);
			goto week_rerun;
		}
		return week1;
	}
}
int QalculateDateTime::yearday() const {
	int yday = 0;
	for(long int i = 1; i < i_month; i++) {
		yday += daysPerMonth(i, i_year);
	}
	return yday + i_day;
}
Number QalculateDateTime::timestamp() const {
	QalculateDateTime date(nr_zero);
	Number nr(date.daysTo(*this));
	nr *= (60 * 60 * 24);
	return nr;
}
Number QalculateDateTime::daysTo(const QalculateDateTime &date, int basis, bool date_func) const {
	
	Number nr;
	
	if(basis < 0 || basis > 4) basis = 1;
	
	bool neg = false;
	bool isleap = false;
	long int days, years;
	
	long int day1 = i_day, month1 = i_month, year1 = i_year;
	long int day2 = date.day(), month2 = date.month(), year2 = date.year();

	if(year1 > year2 || (year1 == year2 && month1 > month2) || (year1 == year2 && month1 == month2 && day1 > day2)) {
		int year3 = year1, month3 = month1, day3 = day1;
		year1 = year2; month1 = month2; day1 = day2;
		year2 = year3; month2 = month3; day2 = day3;
		neg = true;
	}

	years = year2  - year1;
	days = day2 - day1;

	isleap = isLeapYear(year1);

	switch(basis) {
		case 0: {
			nr.set(years, 1, 0);
			nr *= 12;
			nr += (month2 - month1);
			nr *= 30;
			nr += days;
			if(date_func) {
				if(month1 == 2 && ((day1 == 28 && !isleap) || (day1 == 29 && isleap)) && !(month2 == month1 && day1 == day2 && year1 == year2)) {
					if(isleap) nr -= 1;
					else nr -= 2;
				} else if(day1 == 31 && day2 < 31) {
					nr++;
				}
			} else {
				if(month1 == 2 && month2 != 2 && year1 == year2) {
					if(isleap) nr -= 1;
					else nr -= 2;
				}
			}
			break;
		}
		case 1: {}
		case 2: {}
		case 3: {
			int month4 = month2;
			bool b;
			if(years > 0) {
				month4 = 12;
				b = true;
			} else {
				b = false;
			}
			nr.set(days, 1, 0);
			for(; month1 < month4 || b; month1++) {
				if(month1 > month4 && b) {
					b = false;
					month1 = 1;
					month4 = month2;
					if(month1 == month2) break;
				}
				if(!b) {
					nr += daysPerMonth(month1, year2);
				} else {
					nr += daysPerMonth(month1, year1);
				}
			}
			if(years == 0) break;
			bool check_aborted = years > 10000L;
			for(year1 += 1; year1 < year2; year1++) {
				if(check_aborted && CALCULATOR && CALCULATOR->aborted()) {
					nr.setPlusInfinity();
					return nr;
				}
				if(isLeapYear(year1)) nr += 366;
				else nr += 365;
			} 
			break;
		} 
		case 4: {
			nr.set(years, 1, 0);
			nr *= 12;
			nr += (month2 - month1);
			if(date_func) {
				if(day2 == 31 && day1 < 31) days--;
				if(day1 == 31 && day2 < 31) days++;
			}
			nr *= 30;
			nr += days;
			break;
		}
	}
	if(neg) nr.negate();
	return nr;
}
Number QalculateDateTime::yearsTo(const QalculateDateTime &date, int basis, bool date_func) const {
	Number nr;
	if(basis < 0 || basis > 4) basis = 1;
	if(basis == 1) {
		if(date.year() == i_year) {
			nr.set(daysTo(date, basis, date_func));
			nr.divide(daysPerYear(i_year, basis));
		} else {
			bool neg = false;
			long int day1 = i_day, month1 = i_month, year1 = i_year;
			long int day2 = date.day(), month2 = date.month(), year2 = date.year();
			if(year1 > year2) {
				int year3 = year1, month3 = month1, day3 = day1;
				year1 = year2; month1 = month2; day1 = day2;
				year2 = year3; month2 = month3; day2 = day3;
				neg = true;
			}
			for(int month = 12; month > month1; month--) {
				nr += daysPerMonth(month, year1);
			}
			nr += daysPerMonth(month1, year1) - day1 + 1;
			for(int month = 1; month < month2; month++) {
				nr += daysPerMonth(month, year2);
			}
			nr += day2 - 1;
			bool check_aborted = (year2 - year1) > 10000L;
			Number days_of_years;
			for(int year = year1; year <= year2; year++) {
				if(check_aborted && CALCULATOR && CALCULATOR->aborted()) {
					nr.setPlusInfinity();
					return nr;
				}
				days_of_years += daysPerYear(year, basis);
				if(year != year1 && year != year2) {
					nr += daysPerYear(year, basis);
				}
			}
			days_of_years /= year2 + 1 - year1;
			nr /= days_of_years;
			if(neg) nr.negate();
		}
	} else {
		nr.set(daysTo(date, basis, date_func));
		nr.divide(daysPerYear(0, basis));
	}
	return nr;
}

