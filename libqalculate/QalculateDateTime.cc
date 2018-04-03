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
#include "limits"
#include <sstream>
#include <stdlib.h>
#include <iomanip>

static const bool has_leap_second[] = {
// 30/6, 31/12
true, true, //1972
false, true,
false, true,
false, true,
false, true,
false, true,
false, true,
false, true,
false, false, //1980
true, false,
true, false,
true, false,
false, false,
true, false,
false, false,
false, true,
false, false,
false, true,
false, true, //1990
false, false,
true, false,
true, false,
true, false,
false, true,
false, false,
true, false,
false, true,
false, false,
false, false, //2000
false, false,
false, false,
false, false,
false, false,
false, true,
false, false,
false, false,
false, true,
false, false,
false, false, //2010
false, false,
true, false,
false, false,
false, false,
true, false,
false, true //2016
};
#define LS_FIRST_YEAR 1972
#define LS_LAST_YEAR 2016
#define INITIAL_LS 10

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

int countLeapSeconds(const QalculateDateTime &date1, const QalculateDateTime &date2) {
	if(date1 > date2) return -countLeapSeconds(date2, date1);
	if(date1.year() > LS_LAST_YEAR) return 0;
	if(date2.year() < LS_FIRST_YEAR) return 0;
	size_t halfyear1 = 0;
	if(date1.year() >= LS_FIRST_YEAR) {
		halfyear1 = (date1.year() - LS_FIRST_YEAR) * 2;
		if(date1.month() > 6) halfyear1++;
	}
	size_t halfyear2 = 0;
	if(date2.year() >= LS_FIRST_YEAR) {
		halfyear2 = (date2.year() - LS_FIRST_YEAR) * 2;
		if(date2.month() <= 6) halfyear2--;
	}
	if(date1.second().isGreaterThanOrEqualTo(60) && date1.minute() == 59 && date1.hour() == 23 && ((date1.month() == 12 && date1.day() == 31) || (date1.month() == 6 && date1.day() == 30))) halfyear1++;
	if(date2.second().isGreaterThanOrEqualTo(60) && date2.minute() == 59 && date2.hour() == 23 && ((date2.month() == 12 && date2.day() == 31) || (date2.month() == 6 && date2.day() == 30))) halfyear2--;
	int i_ls = 0;
	for(size_t i = halfyear1; i <= halfyear2 && i < sizeof(has_leap_second); i++) {
		if(has_leap_second[i]) {
			i_ls++;
		}
	}
	return i_ls;
}
QalculateDateTime nextLeapSecond(const QalculateDateTime &date) {
	if(date.year() > LS_LAST_YEAR) return QalculateDateTime();
	size_t halfyear = 0;
	if(date.year() >= LS_FIRST_YEAR) {
		halfyear = (date.year() - LS_FIRST_YEAR) * 2;
		if(date.month() > 6) halfyear++;
	}
	if(date.second().isGreaterThanOrEqualTo(60) && date.minute() == 59 && date.hour() == 23 && ((date.month() == 12 && date.day() == 31) || (date.month() == 6 && date.day() == 30))) halfyear++;
	for(size_t i = halfyear; i < sizeof(has_leap_second); i++) {
		if(has_leap_second[i]) {
			QalculateDateTime dt;
			if(i % 2 == 1) dt.set(LS_FIRST_YEAR + i / 2, 12, 31);
			else dt.set(LS_FIRST_YEAR + i / 2, 6, 30);
			dt.setTime(23, 59, 60);
			return dt;
		}
	}
	return QalculateDateTime();
}
QalculateDateTime prevLeapSecond(const QalculateDateTime &date) {
	if(date.year() < LS_FIRST_YEAR) return QalculateDateTime();
	size_t halfyear = sizeof(has_leap_second);
	if(date.year() <= LS_LAST_YEAR) {
		halfyear = (date.year() - LS_FIRST_YEAR) * 2;
		if(date.month() <= 6) halfyear--;
	}
	if(date.second().isGreaterThanOrEqualTo(60) && date.minute() == 59 && date.hour() == 23 && ((date.month() == 12 && date.day() == 31) || (date.month() == 6 && date.day() == 30))) halfyear--;
	for(int i = halfyear; i >= 0; i--) {
		if(has_leap_second[(size_t) i]) {
			QalculateDateTime dt;
			if(i % 2 == 1) dt.set(LS_FIRST_YEAR + i / 2, 12, 31);
			else dt.set(LS_FIRST_YEAR + i / 2, 6, 30);
			dt.setTime(23, 59, 60);
			return dt;
		}
	}
	return QalculateDateTime();
}

int dateTimeZone(time_t rawtime) {
	struct tm tmdate = *localtime(&rawtime);
	std::ostringstream os;
	os << std::put_time(&tmdate, "%z");
	std::string s = os.str();
	int h = s2i(s.substr(0, 3));
	int m = s2i(s.substr(3));
	return h * 60 + m;
}
int dateTimeZone(const QalculateDateTime &dt, bool b_utc) {
	struct tm tmdate;
	time_t rawtime;
	if(dt.year() > 2200) {
		if(isLeapYear(dt.year())) tmdate.tm_year = 2200 - 1900;
		else tmdate.tm_year = 2199 - 1900;
	} else if(dt.year() < 0) {
		if(isLeapYear(dt.year())) tmdate.tm_year = -1900;
		else tmdate.tm_year = 1 - 1900;
	} else {
		tmdate.tm_year = dt.year() - 1900;
	}
	tmdate.tm_mon = dt.month() - 1;
	tmdate.tm_mday = dt.day();
	tmdate.tm_hour = dt.hour();
	tmdate.tm_min = dt.minute();
	Number nsect(dt.second());
	nsect.trunc();
	tmdate.tm_sec = nsect.intValue();
	rawtime = mktime(&tmdate);
	if(rawtime == (time_t) -1) {
		if(dt.year() > 2038) {
			if(isLeapYear(dt.year())) tmdate.tm_year = 2036 - 1900;
			else tmdate.tm_year = 2038 - 1900;
			rawtime = mktime(&tmdate);
		} else if(dt.year() < 1970 || (!b_utc && dt.year() == 1970)) {
			if(dt.year() < 0) {
				rawtime = (time_t) -62167219200LL;
			} else {
				Number nstamp = dt.timestamp(true);
				nstamp.trunc();
				rawtime = (time_t) nstamp.llintValue();
			}
		}
	}
	if(b_utc) rawtime += dateTimeZone(rawtime) * 60;
	return dateTimeZone(rawtime);
}

QalculateDateTime::QalculateDateTime() : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {}
QalculateDateTime::QalculateDateTime(long int initialyear, int initialmonth, int initialday) : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {set(initialyear, initialmonth, initialday);}
QalculateDateTime::QalculateDateTime(const Number &initialtimestamp) : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {set(initialtimestamp);}
QalculateDateTime::QalculateDateTime(string date_string) : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {set(date_string);}
QalculateDateTime::QalculateDateTime(const QalculateDateTime &date) : i_year(date.year()), i_month(date.month()), i_day(date.day()), i_hour(date.hour()), i_min(date.minute()), n_sec(date.second()), b_time(date.timeIsSet()) {}
void QalculateDateTime::setToCurrentDate() {
	struct tm tmdate;
	time_t rawtime;
	::time(&rawtime);
	tmdate = *localtime(&rawtime);
	set(tmdate.tm_year + 1900, tmdate.tm_mon + 1, tmdate.tm_mday);
}
void QalculateDateTime::setToCurrentTime() {
	set(::time(NULL));
}
bool QalculateDateTime::operator > (const QalculateDateTime &date2) const {
	if(i_year != date2.year()) return i_year > date2.year();
	if(i_month != date2.month()) return i_month > date2.month();
	if(i_day != date2.day()) return i_day > date2.day();
	if(i_hour != date2.hour()) return i_hour > date2.hour();
	if(i_min != date2.minute()) return i_min > date2.minute();
	return n_sec.isGreaterThan(date2.second());
}
bool QalculateDateTime::operator < (const QalculateDateTime &date2) const {
	if(i_year != date2.year()) return i_year < date2.year();
	if(i_month != date2.month()) return i_month < date2.month();
	if(i_day != date2.day()) return i_day < date2.day();
	if(i_hour != date2.hour()) return i_hour < date2.hour();
	if(i_min != date2.minute()) return i_min < date2.minute();
	return n_sec.isLessThan(date2.second());
}
bool QalculateDateTime::operator >= (const QalculateDateTime &date2) const {
	return !(*this < date2);
}
bool QalculateDateTime::operator <= (const QalculateDateTime &date2) const {
	return !(*this > date2);
}
bool QalculateDateTime::operator != (const QalculateDateTime &date2) const {
	return i_year != date2.year() || i_month != date2.month() || i_day > date2.day() || i_hour != date2.hour() || i_min != date2.minute() || !n_sec.equals(date2.second());
}
bool QalculateDateTime::operator == (const QalculateDateTime &date2) const {
	return i_year == date2.year() && i_month == date2.month() && i_day == date2.day() && i_hour == date2.hour() && i_min == date2.minute() && n_sec.equals(date2.second());
}
bool QalculateDateTime::isFutureDate() const {
	QalculateDateTime current_date;
	if(!b_time && i_hour == 0 && i_min == 0 && n_sec.isZero()) {
		current_date.setToCurrentDate();
	} else {
		current_date.setToCurrentTime();
	}
	return *this > current_date;
}
bool QalculateDateTime::isPastDate() const {
	QalculateDateTime current_date;
	if(!b_time && i_hour == 0 && i_min == 0 && n_sec.isZero()) {
		current_date.setToCurrentDate();
	} else {
		current_date.setToCurrentTime();
	}
	return *this < current_date;
}
bool QalculateDateTime::set(long int newyear, int newmonth, int newday) {
	if(newmonth < 1 || newmonth > 12) return false;
	if(newday < 1 || newday > daysPerMonth(newmonth, newyear)) return false;
	i_year = newyear;
	i_month = newmonth;
	i_day = newday;
	i_hour = 0;
	i_min = 0;
	n_sec.clear();
	b_time = false;
	return true;
}
bool QalculateDateTime::set(const Number &newtimestamp) {
	if(!newtimestamp.isReal() || newtimestamp.isInterval()) return false;
	QalculateDateTime tmbak(*this);
	i_year = 1970;
	i_month = 1;
	i_day = 1;
	i_hour = 0;
	i_min = 0;
	n_sec.clear();
	b_time = true;
	if(!addSeconds(newtimestamp, false, false) || !addMinutes(dateTimeZone(*this, true), false, false)) {
		set(tmbak);
		return false;
	}
	return true;
}
bool QalculateDateTime::set(string str) {

	long int newyear = 0, newmonth = 0, newday = 0;

	remove_blank_ends(str);
	if(equalsIgnoreCase(str, _("now")) || equalsIgnoreCase(str, "now")) {
		setToCurrentTime();
		return true;
	} else if(equalsIgnoreCase(str, _("today")) || equalsIgnoreCase(str, "today")) {
		setToCurrentDate();
		return true;
	} else if(equalsIgnoreCase(str, _("tomorrow")) || equalsIgnoreCase(str, "tomorrow")) {
		setToCurrentDate();
		addDays(1);
		return true;
	} else if(equalsIgnoreCase(str, _("yesterday")) || equalsIgnoreCase(str, "yesterday")) {
		setToCurrentDate();
		addDays(-1);
		return true;
	}
	bool b_t = false, b_tz = false;
	size_t i_t = str.find("T");
	int newhour = 0, newmin = 0, newsec = 0;
	int itz = 0;
	if(i_t != string::npos && i_t < str.length() - 1 && is_in(NUMBERS, str[i_t + 1])) {
		b_t = true;
		string time_str = str.substr(i_t + 1);
		str.resize(i_t);
		char tzstr[10] = "";
		if(sscanf(time_str.c_str(), "%2u:%2u:%2u%9s", &newhour, &newmin, &newsec, tzstr) < 2) {
			if(sscanf(time_str.c_str(), "%2u%2u%2u%9s", &newhour, &newmin, &newsec, tzstr) < 2) {
#ifndef _WIN32
				struct tm tmdate;
				if(strptime(time_str.c_str(), "%X", &tmdate) || strptime(time_str.c_str(), "%EX", &tmdate)) {
					newhour = tmdate.tm_hour;
					newmin = tmdate.tm_min;
					newsec = tmdate.tm_sec;
				} else {
					return false;
				}
#else
				return false;
#endif
			}
		}
		string stz = tzstr;
		remove_blanks(stz);
		if(stz == "Z" || stz == "GMT" || stz == "UTC") {
			b_tz = true;
		} else if(stz.length() > 1 && (stz[0] == '-' || stz[0] == '+')) {
			unsigned int tzh = 0, tzm = 0;
			if(sscanf(stz.c_str() + sizeof(char), "%2u:%2u", &tzh, &tzm) > 0) {
				itz = tzh * 60 + tzm;
				if(str[0] == '-') itz = -itz;
				b_tz = true;
			}
		}
	}
	if(newhour >= 24 || newmin >= 60 || newsec > 60 || (newsec == 60 && (newhour != 23 || newmin != 59))) return false;
	gsub(SIGN_MINUS, MINUS, str);
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
	if(!set(newyear, newmonth, newday)) return false;
	if(b_t) {
		b_time = true;
		i_hour = newhour;
		i_min = newmin;
		n_sec = newsec;
		if(b_tz) {
			addMinutes(dateTimeZone(*this, true) - itz, false, false);
		}
	}
	return true;
}
void QalculateDateTime::set(const QalculateDateTime &date) {
	i_year = date.year();
	i_month = date.month();
	i_day = date.day();
	i_hour = date.hour();
	i_min = date.minute();
	n_sec = date.second();
	b_time = date.timeIsSet();
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
	if(b_time || !n_sec.isZero() || i_hour != 0 || i_min != 0) {
		str += "T";
		if(i_hour < 10) str += "0";
		str += i2s(i_hour);
		str += ":";
		if(i_min < 10) str += "0";
		str += i2s(i_min);
		str += ":";
		Number nsect(n_sec);
		nsect.trunc();
		if(nsect.isLessThan(10)) str += "0";
		str += nsect.print();
	}
	return str;
}
string QalculateDateTime::toLocalString() const {
	if(i_year > INT_MAX + 1900L || i_year < INT_MIN + 1900) return toISOString();
	struct tm tmdate;
	tmdate.tm_year = i_year - 1900;
	tmdate.tm_mon = i_month - 1;
	tmdate.tm_mday = i_day;
	if(b_time || !n_sec.isZero() || i_hour != 0 || i_min != 0) {
		tmdate.tm_hour = i_hour;
		tmdate.tm_min = i_min;
		Number nsect(n_sec);
		nsect.trunc();
		tmdate.tm_sec = nsect.intValue();
	} else {
		tmdate.tm_hour = 0;
		tmdate.tm_min = 0;
		tmdate.tm_sec = 0;
	}
	char *buffer = (char*) malloc(100 * sizeof(char));
	if(!strftime(buffer, 100, "%xT%X", &tmdate)) {
		return toISOString();
	}
	string str = buffer;
	free(buffer);
	return str;
}
string QalculateDateTime::print(const PrintOptions &po) const {
	if(po.is_approximate && !n_sec.isInteger()) *po.is_approximate = true;
	string str;
	if(po.time_zone == TIME_ZONE_LOCAL) {
		if(po.date_time_format == DATE_TIME_FORMAT_LOCALE) str = toLocalString();
		else str = toISOString();
	} else {
		QalculateDateTime dtz(*this);
		if(po.time_zone == TIME_ZONE_UTC) {
			dtz.addMinutes(-dateTimeZone(*this, false), false, false);
		} else {
			dtz.addMinutes(-dateTimeZone(*this, false) + po.custom_time_zone, false, false);
		}
		if(po.date_time_format == DATE_TIME_FORMAT_LOCALE) str = dtz.toLocalString();
		else str = dtz.toISOString();
		if(po.time_zone == TIME_ZONE_UTC) {
			str += "Z";
		} else {
			if(po.custom_time_zone < 0) str += '+';
			if(::abs(po.custom_time_zone) / 60 < 10) str += "0";
			str += i2s(po.custom_time_zone / 60);
			str += ":";
			if(po.custom_time_zone % 60 < 10) str += "0";
			str += i2s(po.custom_time_zone % 60);
		}
	}
	if(po.use_unicode_signs && i_year < 0 && str.length() > 0 && str[0] == MINUS_CH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) {
		str.replace(0, 1, SIGN_MINUS);
	}
	return str;
}
long int QalculateDateTime::year() const {
	return i_year;
}
long int QalculateDateTime::month() const {
	return i_month;
}
long int QalculateDateTime::day() const {
	return i_day;
}
long int QalculateDateTime::hour() const {
	return i_hour;
}
long int QalculateDateTime::minute() const {
	return i_min;
}
const Number &QalculateDateTime::second() const {
	return n_sec;
}
void QalculateDateTime::setYear(long int newyear) {i_year = newyear;}
bool QalculateDateTime::timeIsSet() const {return b_time;}
bool QalculateDateTime::setTime(long int ihour, long int imin, const Number &nsec) {
	i_hour = ihour;
	i_min = imin;
	n_sec = nsec;
	b_time = true;
	return true;
}
bool QalculateDateTime::addHours(const Number &nhours) {
	Number nmins(nhours);
	nmins *= 60;
	return addMinutes(nmins, true, true);
}
bool QalculateDateTime::addMinutes(const Number &nminutes, bool remove_leap_second, bool convert_to_utc) {
	if(!nminutes.isReal() || nminutes.isInterval()) return false;
	if(!nminutes.isInteger()) {
		Number newmins(nminutes);
		newmins.trunc();
		QalculateDateTime dtbak(*this);
		if(!addMinutes(newmins, remove_leap_second, convert_to_utc)) return false;
		Number nsec(nminutes);
		nsec.frac();
		nsec *= 60;
		if(!addSeconds(nsec, false, false)) {
			set(dtbak);
			return false;
		}
		return true;
	}
	QalculateDateTime dtbak(*this);
	if(convert_to_utc) {
		if(!addMinutes(-dateTimeZone(*this, false), false, false)) return false;
	}
	if(remove_leap_second && n_sec.isGreaterThanOrEqualTo(60)) {
		n_sec--;
	}
	Number nmins(nminutes);
	nmins /= 60;
	Number nhours(nmins);
	nhours.trunc();
	nmins.frac();
	nmins *= 60;
	i_min = i_min + nmins.lintValue();
	if(i_min > 60) {
		i_min -= 60;
		nhours++;
	} else if(i_min < 0) {
		i_min += 60;
		nhours--;
	}
	nhours /= 24;
	Number ndays(nhours);
	ndays.trunc();
	nhours.frac();
	nhours *= 24;
	i_hour = i_hour + nhours.lintValue();
	if(i_hour >= 24) {
		i_hour -= 24;
		ndays++;
	} else if(i_hour < 0) {
		i_hour += 24;
		ndays--;
	}
	if(!addDays(ndays)) {
		set(dtbak);
		return false;
	}
	if(convert_to_utc) {
		if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
	}
	return true;
}
bool QalculateDateTime::addDays(const Number &ndays) {
	if(!ndays.isReal() || ndays.isInterval()) return false;
	if(!ndays.isInteger()) {
		Number newdays(ndays);
		newdays.trunc();
		QalculateDateTime dtbak(*this);
		if(!addDays(newdays)) return false;
		Number nmin(ndays);
		nmin.frac();
		nmin *= 1440;
		if(!addMinutes(nmin, true, true)) {
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
				Number secfrac(i_hour * 3600 + i_min * 60);
				secfrac += n_sec;
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
				Number secfrac(i_hour * 3600 + i_min * 60);
				secfrac += n_sec;
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
		if(nday.isZero()) return true;
		long int idoy = yearday();
		if(nday.isNegative()) {
			if(nday.isLessThan(idoy - 1)) {
				nday *= daysPerYear(i_year);
			} else {
				Number idayfrac(idoy - 1);
				Number secfrac(i_hour * 3600 + i_min * 60);
				secfrac += n_sec;
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
				Number secfrac(i_hour * 3600 + i_min * 60);
				secfrac += n_sec;
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
bool QalculateDateTime::addSeconds(const Number &seconds, bool count_leap_seconds, bool convert_to_utc) {
	if(!seconds.isReal() || seconds.isInterval()) return false;
	if(seconds.isZero()) return true;
	QalculateDateTime dtbak(*this);
	if(convert_to_utc) {
		if(!addMinutes(-dateTimeZone(*this, false), false, false)) return false;
	}
	b_time = true;
	if(count_leap_seconds) {
		if(seconds.isPositive()) {
			Number nsum(i_hour * 3600 + i_min * 60);
			nsum += n_sec;
			nsum += seconds;
			if(nsum.isLessThan(SECONDS_PER_DAY)) {
				if(!addSeconds(seconds, false, false)) {set(dtbak); return false;}
				if(convert_to_utc) {
					if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
				}
				return true;
			}
		} else {
			Number nsum(i_hour * 3600 + i_min * 60);
			nsum += n_sec;
			nsum += seconds;
			if(nsum.isPositive()) {
				if(!addSeconds(seconds, false, false)) {set(dtbak); return false;}
				if(convert_to_utc) {
					if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
				}
				return true;
			}
		}
		Number nr_frac(n_sec);
		nr_frac.frac();
		n_sec.trunc();
		Number secnew(seconds);
		secnew += nr_frac;
		nr_frac = secnew;
		nr_frac.frac();
		secnew.trunc();
		if(secnew.isZero()) {
			n_sec += nr_frac;
			if(convert_to_utc) {
				if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
			}
			return true;
		}
		if(secnew.isNegative() || nr_frac.isNegative()) {
			if(nr_frac.isNegative()) {
				nr_frac++;
				secnew--;
			}
			QalculateDateTime dt_nls = prevLeapSecond(*this);
			while(dt_nls.year() != 0) {
				Number n_sto = secondsTo(dt_nls, false, false);
				n_sto--;
				if(n_sto.isLessThan(secnew)) {
					secnew += nr_frac;
					break;
				} else if(n_sto.equals(secnew)) {
					set(dt_nls);
					n_sec += nr_frac;
					if(convert_to_utc) {
						if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
					}
					return true;
				}
				set(dt_nls);
				n_sec--;
				secnew -= n_sto;
				secnew++;
				dt_nls = prevLeapSecond(*this);
			}
			secnew += nr_frac;
			if(!addSeconds(secnew, false, false)) {set(dtbak); return false;}
			if(convert_to_utc) {
				if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
			}
			return true;
		} else {
			if(i_hour == 23 && i_min == 59 && n_sec == 60) {
				secnew--;
			}
			QalculateDateTime dt_nls = nextLeapSecond(*this);
			while(dt_nls.year() != 0) {
				Number n_sto = secondsTo(dt_nls, false, false);
				if(n_sto.isGreaterThan(secnew)) {
					secnew += nr_frac;
					break;
				} else if(n_sto.equals(secnew)) {
					set(dt_nls);
					n_sec += nr_frac;
					if(convert_to_utc) {
						if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
					}
					return true;
				}
				set(dt_nls);
				addDays(1);
				i_hour = 0;
				i_min = 0;
				n_sec.clear();
				secnew -= n_sto;
				secnew--;
				dt_nls = nextLeapSecond(*this);
			}
			secnew += nr_frac;
			if(!addSeconds(secnew, false, false)) {set(dtbak); return false;}
			if(convert_to_utc) {
				if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
			}
			return true;
		}
	}
	if(!n_sec.add(seconds)) {set(dtbak); return false;}
	if(n_sec.isNegative()) {
		if(n_sec.isLessThan(-60)) {
			n_sec /= 60;
			Number nmins(n_sec);
			nmins.trunc();
			n_sec.frac();
			n_sec *= 60;
			if(n_sec.isNegative()) {
				n_sec += 60;
				nmins--;
			}
			if(!addMinutes(nmins, false, false)) {set(dtbak); return false;}
		} else {
			n_sec += 60;
			if(!addMinutes(-1, false, false)) {set(dtbak); return false;}
		}
	} else if(n_sec.isGreaterThanOrEqualTo(60)) {
		n_sec /= 60;
		Number nmins(n_sec);
		nmins.trunc();
		n_sec.frac();
		n_sec *= 60;
		if(!addMinutes(nmins, false, false)) {set(dtbak); return false;}
	}
	if(convert_to_utc) {
		if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
	}
	return true;
}
bool QalculateDateTime::add(const QalculateDateTime &date) {
	QalculateDateTime dtbak(*this);
	if(date.timeIsSet()) b_time = true;
	if(!addYears(date.year()) || !addMonths(date.month()) || !addDays(date.day())) {
		set(dtbak);
		return false;
	}
	if(!date.second().isZero() || date.hour() != 0 || date.minute() != 0) {
		Number nsec(date.hour() * 3600 + date.minute() * 60);
		nsec += date.second();
		if(!addSeconds(nsec, true, true)) {
			set(dtbak);
			return false;
		}
	}
	return true;
}
int QalculateDateTime::weekday() const {
	Number nr(daysTo(QalculateDateTime(2017, 7, 31)));
	if(nr.isInfinite()) return -1;
	nr.negate();
	nr.trunc();
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
Number QalculateDateTime::timestamp(bool reverse_utc) const {
	QalculateDateTime date(nr_zero);
	return date.secondsTo(*this, false, !reverse_utc);
}
Number QalculateDateTime::secondsTo(const QalculateDateTime &date, bool count_leap_seconds, bool convert_to_utc) const {
	if(convert_to_utc) {
		QalculateDateTime dt1(*this), dt2(date);
		dt1.addMinutes(-dateTimeZone(dt1, false), false, false);
		dt2.addMinutes(-dateTimeZone(dt2, false), false, false);
		return dt1.secondsTo(dt2, count_leap_seconds, false);
	}
	Number nr = daysTo(date, 1, true, !count_leap_seconds);
	nr *= SECONDS_PER_DAY;
	if(count_leap_seconds) {
		nr += countLeapSeconds(*this, date);
	}
	return nr;
}
Number QalculateDateTime::daysTo(const QalculateDateTime &date, int basis, bool date_func, bool remove_leap_seconds) const {
	
	Number nr;
	
	if(basis < 0 || basis > 4) basis = 1;
	
	bool neg = false;
	bool isleap = false;
	long int days, years;
	
	long int day1 = i_day, month1 = i_month, year1 = i_year;
	long int day2 = date.day(), month2 = date.month(), year2 = date.year();
	Number t1(n_sec), t2(date.second());
	if(remove_leap_seconds) {
		if(t1.isGreaterThanOrEqualTo(60)) t1--;
		if(t2.isGreaterThanOrEqualTo(60)) t2--;
	}
	t1 += i_hour * 3600 + i_min * 60;
	t2 += date.hour() * 3600 + date.minute() * 60;

	if(year1 > year2 || (year1 == year2 && month1 > month2) || (year1 == year2 && month1 == month2 && day1 > day2) || (basis == 1 && date_func && year1 == year2 && month1 == month2 && day1 == day2 && t1.isGreaterThan(t2))) {
		int year3 = year1, month3 = month1, day3 = day1;
		year1 = year2; month1 = month2; day1 = day2;
		year2 = year3; month2 = month3; day2 = day3;
		Number t3(t1);
		t1 = t2; t2 = t3;
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
			if(basis == 1 && !t1.equals(t2)) {
				t2 -= t1;
				t2 /= 86400;
				nr += t2;
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
Number QalculateDateTime::yearsTo(const QalculateDateTime &date, int basis, bool date_func, bool remove_leap_seconds) const {
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
			Number t1(n_sec), t2(date.second());
			if(remove_leap_seconds) {
				if(t1.isGreaterThanOrEqualTo(60)) t1--;
				if(t2.isGreaterThanOrEqualTo(60)) t2--;
			}
			t1 += i_hour * 3600 + i_min * 60;
			t2 += date.hour() * 3600 + date.minute() * 60;
			if(year1 > year2) {
				int year3 = year1, month3 = month1, day3 = day1;
				year1 = year2; month1 = month2; day1 = day2;
				year2 = year3; month2 = month3; day2 = day3;
				Number t3(t1);
				t1 = t2; t2 = t3;
				neg = true;
			}
			t1 /= 86400;
			t2 /= 86400;
			for(int month = 12; month > month1; month--) {
				nr += daysPerMonth(month, year1);
			}
			nr += daysPerMonth(month1, year1) - day1 + 1;
			nr -= t1;
			for(int month = 1; month < month2; month++) {
				nr += daysPerMonth(month, year2);
			}
			nr += day2 - 1;
			nr += t2;
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

