/*
    Qalculate

    Copyright (C) 2018  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Many of the calendar conversion algorithms used here are adapted from
    CALENDRICA 4.0 -- Common Lisp
    Copyright (C) E. M. Reingold and N. Dershowitz

*/

#include "QalculateDateTime.h"
#include "support.h"
#include "util.h"
#include "Calculator.h"
#include <limits.h>
#include <stdlib.h>
#ifndef _MSC_VER
#	include <sys/time.h>
#endif

using std::string;
using std::vector;

static const char *HEBREW_MONTHS[] = {"Nisan", "Iyar", "Sivan", "Tammuz", "Av", "Elul", "Tishrei", "Marcheshvan", "Kislev", "Tevet", "Shevat", "Adar", "Adar II"};
static const char *STANDARD_MONTHS[] = {N_("January"), N_("February"), N_("March"), N_("April"), N_("May"), N_("June"), N_("July"), N_("August"), N_("September"), N_("October"), N_("November"), N_("December")};
static const char *COPTIC_MONTHS[] = {N_("Thout"), N_("Paopi"), N_("Hathor"), N_("Koiak"), N_("Tobi"), N_("Meshir"), N_("Paremhat"), N_("Parmouti"), N_("Pashons"), N_("Paoni"), N_("Epip"), N_("Mesori"), N_("Pi Kogi Enavot")};
static const char *ETHIOPIAN_MONTHS[] = {N_("Mäskäräm"), N_("Ṭəqəmt"), N_("Ḫədar"), N_("Taḫśaś"), N_("Ṭərr"), N_("Yäkatit"), N_("Mägabit"), N_("Miyazya"), N_("Gənbo"), N_("Säne"), N_("Ḥamle"), N_("Nähase"), N_("Ṗagume")};
static const char *ISLAMIC_MONTHS[] = {N_("Muḥarram"), N_("Ṣafar"), N_("Rabī‘ al-awwal"), N_("Rabī‘ ath-thānī"), N_("Jumādá al-ūlá"), N_("Jumādá al-ākhirah"), N_("Rajab"), N_("Sha‘bān"), N_("Ramaḍān"), N_("Shawwāl"), N_("Dhū al-Qa‘dah"), N_("Dhū al-Ḥijjah")};
static const char *PERSIAN_MONTHS[] = {N_("Farvardin"), N_("Ordibehesht"), N_("Khordad"), N_("Tir"), N_("Mordad"), N_("Shahrivar"), N_("Mehr"), N_("Aban"), N_("Azar"), N_("Dey"), N_("Bahman"), N_("Esfand")};
static const char *INDIAN_MONTHS[] = {N_("Chaitra"), N_("Vaishākha"), N_("Jyēshtha"), N_("Āshādha"), N_("Shrāvana"), N_("Bhaadra"), N_("Āshwin"), N_("Kārtika"), N_("Agrahayana"), N_("Pausha"), N_("Māgha"), N_("Phalguna")};
static const char *CHINESE_ELEMENTS[] = {N_("Wood"), N_("Fire"), N_("Earth"), N_("Metal"), N_("Water")};
static const char *CHINESE_ANIMALS[] = {N_("Rat"), N_("Ox"), N_("Tiger"), N_("Rabbit"), N_("Dragon"), N_("Snake"), N_("Horse"), N_("Goat"), N_("Monkey"), N_("Rooster"), N_("Dog"), N_("Pig")};

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
		if(date2.month() <= 6) {
			if(halfyear2 == 0) return 0;
			halfyear2--;
		}
	}
	if(date1.second().isGreaterThanOrEqualTo(60) && date1.minute() == 59 && date1.hour() == 23 && ((date1.month() == 12 && date1.day() == 31) || (date1.month() == 6 && date1.day() == 30))) halfyear1++;
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
	size_t halfyear = sizeof(has_leap_second) - 1;
	if(date.year() <= LS_LAST_YEAR) {
		halfyear = (date.year() - LS_FIRST_YEAR) * 2;
		if(date.month() <= 6) {
			if(halfyear == 0) return QalculateDateTime();
			halfyear--;
		}
	}
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
#ifdef _WIN32
	TIME_ZONE_INFORMATION TimeZoneInfo;
	GetTimeZoneInformation(&TimeZoneInfo);
	return -(TimeZoneInfo.Bias + (tmdate.tm_isdst ? TimeZoneInfo.DaylightBias : TimeZoneInfo.StandardBias));
#else
	char buffer[10];
	if(!strftime(buffer, 10, "%z", &tmdate)) {
		return 0;
	}
	string s = buffer;
	int h = s2i(s.substr(0, 3));
	int m = s2i(s.substr(3));
	return h * 60 + m;
#endif
}
int dateTimeZone(const QalculateDateTime &dt, bool b_utc) {
	struct tm tmdate;
	time_t rawtime;
	if(dt.year() > 2038) {
		if(isLeapYear(dt.year())) tmdate.tm_year = 136;
		else tmdate.tm_year = 137;
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
	if(rawtime == (time_t) -1 && (dt.year() != 1969 || dt.month() != 12 || dt.day() != 31)) {
		if(isLeapYear(dt.year())) tmdate.tm_year = 72;
		else tmdate.tm_year = 71;
		rawtime = mktime(&tmdate);
	}
	if(b_utc && (rawtime >= 0 || localtime(&rawtime))) rawtime += dateTimeZone(rawtime) * 60;
	if(rawtime < 0 && !localtime(&rawtime)) {
		if(isLeapYear(dt.year())) tmdate.tm_year = 72;
		else tmdate.tm_year = 71;
		rawtime = mktime(&tmdate);
		if(b_utc) rawtime += dateTimeZone(rawtime) * 60;
	}
	return dateTimeZone(rawtime);
}

QalculateDateTime::QalculateDateTime() : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {}
QalculateDateTime::QalculateDateTime(long int initialyear, int initialmonth, int initialday) : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {set(initialyear, initialmonth, initialday);}
QalculateDateTime::QalculateDateTime(const Number &initialtimestamp) : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {set(initialtimestamp);}
QalculateDateTime::QalculateDateTime(string date_string) : i_year(0), i_month(1), i_day(1), i_hour(0), i_min(0), b_time(false) {set(date_string);}
QalculateDateTime::QalculateDateTime(const QalculateDateTime &date) : i_year(date.year()), i_month(date.month()), i_day(date.day()), i_hour(date.hour()), i_min(date.minute()), n_sec(date.second()), b_time(date.timeIsSet()), parsed_string(date.parsed_string) {}
void QalculateDateTime::setToCurrentDate() {
	parsed_string.clear();
	struct tm tmdate;
	time_t rawtime;
	::time(&rawtime);
	tmdate = *localtime(&rawtime);
	set(tmdate.tm_year + 1900, tmdate.tm_mon + 1, tmdate.tm_mday);
}
void QalculateDateTime::setToCurrentTime() {
	parsed_string.clear();
	struct timeval tv;
	gettimeofday(&tv, NULL);
	Number nr(tv.tv_usec, 0, -6);
	nr += tv.tv_sec;
	set(nr);
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
	parsed_string.clear();
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
	parsed_string.clear();
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

	string str_bak(str);

	remove_blank_ends(str);
	remove_duplicate_blanks(str);
	if(equalsIgnoreCase(str, _("now")) || equalsIgnoreCase(str, "now")) {
		setToCurrentTime();
		parsed_string = str_bak;
		return true;
	} else if(equalsIgnoreCase(str, _("today")) || equalsIgnoreCase(str, "today")) {
		setToCurrentDate();
		parsed_string = str_bak;
		return true;
	} else if(equalsIgnoreCase(str, _("tomorrow")) || equalsIgnoreCase(str, "tomorrow")) {
		setToCurrentDate();
		addDays(1);
		parsed_string = str_bak;
		return true;
	} else if(equalsIgnoreCase(str, _("yesterday")) || equalsIgnoreCase(str, "yesterday")) {
		setToCurrentDate();
		addDays(-1);
		parsed_string = str_bak;
		return true;
	}
	bool b_t = false, b_tz = false;
	size_t i_t = str.find("T");
	if(i_t == string::npos && str.find(":") != string::npos) i_t = str.rfind(' ');
	int newhour = 0, newmin = 0, newsec = 0;
	int itz = 0;
	if(i_t != string::npos && i_t < str.length() - 1 && is_in(NUMBERS, str[i_t + 1])) {
		b_t = true;
		string time_str = str.substr(i_t + 1);
		str.resize(i_t);
		char tzstr[10] = "";
		if(sscanf(time_str.c_str(), "%2u:%2u:%2u%9s", &newhour, &newmin, &newsec, tzstr) < 3) {
			if(sscanf(time_str.c_str(), "%2u:%2u%9s", &newhour, &newmin, tzstr) < 2) {
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
		}
		string stz = tzstr;
		remove_blanks(stz);
		if(stz == "Z" || stz == "GMT" || stz == "UTC" || stz == "WET") {
			b_tz = true;
		} else if(stz == "CET" || stz == "WEST") {
			itz = 1 * 60;
			b_tz = true;
		} else if(stz == "CEST" || stz == "EET") {
			itz = 2 * 60;
			b_tz = true;
		} else if(stz == "EEST") {
			itz = 3 * 60;
			b_tz = true;
		} else if(stz == "CT" || stz == "CST") {
			itz = 8 * 60;
			b_tz = true;
		} else if(stz == "JST") {
			itz = 9 * 60;
			b_tz = true;
		} else if(stz == "EDT") {
			itz = -4 * 60;
			b_tz = true;
		} else if(stz == "EST") {
			itz = -5 * 60;
			b_tz = true;
		} else if(stz == "PDT" || stz == "MST") {
			itz = -7 * 60;
			b_tz = true;
		} else if(stz == "PST") {
			itz = -8 * 60;
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
	parsed_string = str_bak;
	return true;
}
void QalculateDateTime::set(const QalculateDateTime &date) {
	parsed_string = date.parsed_string;
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
		nsect.setApproximate(false);
		str += nsect.print();
	}
	return str;
}
string QalculateDateTime::toLocalString() const {
	if(i_year > INT_MAX || i_year < INT_MIN + 1900) return toISOString();
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
	char buffer[100];
	if(!strftime(buffer, 100, "%xT%X", &tmdate)) {
		return toISOString();
	}
	return buffer;
}
string QalculateDateTime::print(const PrintOptions &po) const {
	if(po.is_approximate && (!n_sec.isInteger() || n_sec.isApproximate())) *po.is_approximate = true;
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
			if(po.custom_time_zone < 0) str += '-';
			else str += '+';
			if(::abs(po.custom_time_zone) / 60 < 10) str += "0";
			str += i2s(::abs(po.custom_time_zone) / 60);
			str += ":";
			if(::abs(po.custom_time_zone) % 60 < 10) str += "0";
			str += i2s(::abs(po.custom_time_zone) % 60);
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
	parsed_string.clear();
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
	parsed_string.clear();
	if(!nminutes.isReal() || nminutes.isInterval()) return false;
	b_time = true;
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
	i_min += nmins.lintValue();
	if(i_min >= 60) {
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
	parsed_string.clear();
	if(!ndays.isReal() || ndays.isInterval()) return false;
	if(ndays.isZero()) return true;
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

	long int newmonth = i_month, newyear = i_year;
	Number newnday(ndays);
	newnday += i_day;
	if(ndays.isNegative()) {
		bool check_aborted = newnday.isLessThan(-1000000L);
		while(newnday.isLessThanOrEqualTo(-14609700L)) {
			newnday += 14609700L;
			newyear -= 40000L;
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newnday.isLessThanOrEqualTo(-146097L)) {
			newnday += 146097L;
			newyear -= 400;
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newnday.isLessThanOrEqualTo(-daysPerYear(newmonth <= 2 ? newyear - 1 : newyear))) {
			newnday += daysPerYear(newmonth <= 2 ? newyear - 1 : newyear);
			newyear--;
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newnday.isLessThan(1)) {
			newmonth--;
			if(newmonth < 1) {
				newyear--;
				newmonth = 12;
			}
			newnday += daysPerMonth(newmonth, newyear);
		}
	} else {
		bool check_aborted = newnday.isGreaterThan(1000000L);
		while(newnday.isGreaterThan(14609700L)) {
			newnday -= 14609700L;
			newyear += 40000;
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newnday.isGreaterThan(146097L)) {
			newnday -= 146097L;
			newyear += 400;
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newnday.isGreaterThan(daysPerYear(newmonth <= 2 ? newyear : newyear + 1))) {
			newnday -= daysPerYear(newmonth <= 2 ? newyear : newyear + 1);
			newyear++;
			if(check_aborted && CALCULATOR && CALCULATOR->aborted()) return false;
		}
		while(newnday.isGreaterThan(daysPerMonth(newmonth, newyear))) {
			newnday -= daysPerMonth(newmonth, newyear);
			newmonth++;
			if(newmonth > 12) {
				newyear++;
				newmonth = 1;
			}
		}
	}
	bool overflow = false;
	long int newday = newnday.lintValue(&overflow);
	if(overflow) return false;
	i_day = newday;
	i_month = newmonth;
	i_year = newyear;
	return true;
}
bool QalculateDateTime::addMonths(const Number &nmonths) {
	parsed_string.clear();
	if(!nmonths.isReal() || nmonths.isInterval()) return false;
	if(!nmonths.isInteger()) {
		Number newmonths(nmonths);
		newmonths.trunc();
		QalculateDateTime dtbak(*this);
		if(!addMonths(newmonths)) return false;
		Number nday(nmonths);
		nday.frac();
		if(nday.isNegative()) {
			nday.negate();
			nday *= daysPerMonth(i_month, i_year);
			if(nday.isGreaterThanOrEqualTo(i_day - 1)) {
				nday /= daysPerMonth(i_month, i_year);
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
			nday.negate();
		} else {
			nday *= daysPerMonth(i_month, i_year);
			if(nday.isGreaterThanOrEqualTo(daysPerMonth(i_month, i_year) - i_day)) {
				nday /= daysPerMonth(i_month, i_year);
				Number idayfrac(daysPerMonth(i_month, i_year) - i_day);
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
	bool overflow = false;
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
	parsed_string.clear();
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
			nday.negate();
			nday *= daysPerYear(i_year);
			if(nday.isGreaterThanOrEqualTo(idoy - 1)) {
				nday /= daysPerYear(i_year);
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
			nday.negate();
		} else {
			nday *= daysPerYear(i_year);
			if(nday.isGreaterThanOrEqualTo(daysPerYear(i_year) - idoy)) {
				nday /= daysPerYear(i_year);
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
	bool overflow = false;
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
	parsed_string.clear();
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
			if(nsum.isZero()) {
				i_hour = 0;
				i_min = 0;
				n_sec.clear(true);
				if(convert_to_utc) {
					if(!addMinutes(dateTimeZone(*this, true), false, false)) {set(dtbak); return false;}
				}
			} else if(nsum.isPositive()) {
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
				Number n_sto = secondsTo(dt_nls, true, false);
				n_sto--;
				if(n_sto.isLessThan(secnew)) {
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
				Number n_sto = secondsTo(dt_nls, true, false);
				if(n_sto.isGreaterThan(secnew)) {
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
	parsed_string.clear();
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




long int quotient(long int i, long int d) {
	i /= d;
	if((i < 0) != (d < 0)) i--;
	return i;
}
Number quotient(Number nr, long int d) {
	nr /= d;
	nr.floor();
	return nr;
}

void cal_div(const Number &nr_n, long int nr_d, Number &nr_q, Number &nr_r) {
	nr_q = nr_n; nr_q /= nr_d; nr_q.floor();
	nr_r = nr_n; nr_r.mod(nr_d);
}
void cal_div(const Number &nr_n, long int nr_d, Number &nr_q) {
	nr_q = nr_n; nr_q /= nr_d; nr_q.floor();
}
void cal_div(Number &nr_n, long int nr_d) {
	nr_n /= nr_d; nr_n.floor();
}

#define GREGORIAN_EPOCH 1
#define JULIAN_EPOCH -1 // (date_to_fixed(0, 12, 30, CALENDAR_GREGORIAN))
#define ISLAMIC_EPOCH 227015 // date_to_fixed(622, 7, 16, CALENDAR_JULIAN)
#define PERSIAN_EPOCH 226896 // date_to_fixed(622, 3, 19, CALENDAR_JULIAN)
#define JD_EPOCH (Number("-1721424.5"))
#define EGYPTIAN_EPOCH -272787L //jd_to_fixed(1448638))
#define HEBREW_EPOCH -1373427 //date_to_fixed(-3761, 10, 7, CALENDAR_JULIAN)
#define COPTIC_EPOCH 103605
#define ETHIOPIAN_EPOCH 2796
#define CHINESE_EPOCH -963099L //date_to_fixed(-2636, 2, 15, CALENDAR_GREGORIAN)
#define MEAN_TROPICAL_YEAR (Number("365.242189"))
#define MEAN_SIDEREAL_YEAR (Number("365.25636"))
#define MEAN_SYNODIC_MONTH (Number("29.530588861"))

Number jd_to_fixed(Number jd) {
	jd += JD_EPOCH;
	jd.floor();
	return jd;
}

bool cjdn_to_date(Number J, long int &y, long int &m, long int &d, CalendarSystem ct);
Number date_to_cjdn(long int j, long int m, long int d, CalendarSystem ct);
bool fixed_to_date(Number date, long int &y, long int &m, long int &d, CalendarSystem ct);
Number date_to_fixed(long int y, long int m, long int d, CalendarSystem ct);

long int gregorian_year_from_fixed(Number date) {
	Number d0, n400, d1, n100, d2, n4, d3, n1, year;
	d0 = date; d0 -= 1;
	cal_div(d0, 146097, n400, d1);
	cal_div(d1, 36524, n100, d2);
	cal_div(d2, 1461, n4, d3);
	cal_div(d3, 365, n1);
	if(!n100.equals(4) && !n1.equals(4)) year = 1;
	else year = 0;
	n400 *= 400; n100 *= 100; n4 *= 4; year += n400; year += n100; year += n4; year += n1;
	return year.lintValue();
}

Number gregorian_date_difference(long int y1, long int m1, long int d1, long int y2, long int m2, long int d2) {
	Number f1 = date_to_fixed(y2, m2, d2, CALENDAR_GREGORIAN);
	f1 -= date_to_fixed(y1, m1, d1, CALENDAR_GREGORIAN);
	return f1;
}

Number ephemeris_correction(Number tee) {
	tee.floor();
	Number year = gregorian_year_from_fixed(tee);
	if(year > 2150 || year < -500) {
		year -= 1820; year /= 100;
		Number x(year);
		Number t, a;
		t = -20; a += t;
		x *= year; t = 32; t *= x; a += t;
		a /= 86400;
		return a;
	} else if(year < 500) {
		year /= 100;
		Number x(year);
		Number t, a;
		t.setFloat(10583.6L); a += t;
		t.setFloat(-1014.41L); t *= x; a += t;
		x *= year; t.setFloat(33.78311L); t *= x; a += t;
		x *= year; t.setFloat(-5.952053L); t *= x; a += t;
		x *= year; t.setFloat(-0.1798452L); t *= x; a += t;
		x *= year; t.setFloat(0.022174192L); t *= x; a += t;
		x *= year; t.setFloat(0.0090316521L); t *= x; a += t;
		a /= 86400;
		return a;
	} else if(year < 1600) {
		year -= 1000; year /= 100;
		Number x(year);
		Number t, a;
		t.setFloat(1574.2L); a += t;
		t.setFloat(-556.01L); t *= x; a += t;
		x *= year; t.setFloat(71.23472L); t *= x; a += t;
		x *= year; t.setFloat(0.319781L); t *= x; a += t;
		x *= year; t.setFloat(-0.8503463L); t *= x; a += t;
		x *= year; t.setFloat(-0.005050998L); t *= x; a += t;
		x *= year; t.setFloat(0.0083572073L); t *= x; a += t;
		a /= 86400;
		return a;
	} else if(year < 1700) {
		year -= 1600;
		Number x(year);
		Number t, a;
		t = 120; a += t;
		t.setFloat(-0.9808L); t *= x; a += t;
		x *= year; t.setFloat(-0.01532L); t *= x; a += t;
		x *= year; t.setFloat(0.000140272128L); t *= x; a += t;
		a /= 86400;
		return a;
	} else if(year < 1800) {
		year -= 1700;
		Number x(year);
		Number t, a;
		t.setFloat(8.118780842L); a += t;
		t.setFloat(-0.005092142L); t *= x; a += t;
		x *= year; t.setFloat(0.003336121L); t *= x; a += t;
		x *= year; t.setFloat(-0.0000266484L); t *= x; a += t;
		a /= 86400;
		return a;
	} else if(year < 1900) {
		Number c = gregorian_date_difference(1900, 1, 1, year.lintValue(), 7, 1); c /= 36525;
		year = c;
		Number x(year);
		Number t, a;
		t.setFloat(-0.000009L); a += t;
		t.setFloat(0.003844L); t *= x; a += t;
		x *= year; t.setFloat(0.083563L); t *= x; a += t;
		x *= year; t.setFloat(0.865736L); t *= x; a += t;
		x *= year; t.setFloat(4.867575L); t *= x; a += t;
		x *= year; t.setFloat(15.845535L); t *= x; a += t;
		x *= year; t.setFloat(31.332267L); t *= x; a += t;
		x *= year; t.setFloat(38.291999L); t *= x; a += t;
		x *= year; t.setFloat(28.316289L); t *= x; a += t;
		x *= year; t.setFloat(11.636204L); t *= x; a += t;
		x *= year; t.setFloat(2.043794L); t *= x; a += t;
		return a;
	} else if(year < 1987) {
		Number c = gregorian_date_difference(1900, 1, 1, year.lintValue(), 7, 1); c /= 36525;
		year = c;
		Number x(year);
		Number t, a;
		t.setFloat(-0.00002L); a += t;
		t.setFloat(0.000297L); t *= x; a += t;
		x *= year; t.setFloat(0.025184L); t *= x; a += t;
		x *= year; t.setFloat(-0.181133L); t *= x; a += t;
		x *= year; t.setFloat(0.553040L); t *= x; a += t;
		x *= year; t.setFloat(-0.861938L); t *= x; a += t;
		x *= year; t.setFloat(0.677066L); t *= x; a += t;
		x *= year; t.setFloat(-0.212591L); t *= x; a += t;
		return a;
	} else if(year < 2006) {
		year -= 2000;
		Number x(year);
		Number t, a;
		t.setFloat(63.86L); a += t;
		t.setFloat(0.3345L); t *= x; a += t;
		x *= year; t.setFloat(-0.060374L); t *= x; a += t;
		x *= year; t.setFloat(0.0017275L); t *= x; a += t;
		x *= year; t.setFloat(0.000651814L); t *= x; a += t;
		x *= year; t.setFloat(0.00002373599L); t *= x; a += t;
		a /= 86400;
		return a;
	} else if(year <= 2050) {
		year -= 2000;
		Number x(year);
		Number t, a;
		t.setFloat(62.92L); a += t;
		t.setFloat(0.32217L); t *= x; a += t;
		x *= year; t.setFloat(0.005589L); t *= x; a += t;
		a /= 86400;
		return a;
	} else {
		Number year2(year); year2 -= 1820; year2 /= 100; year2.square(); year2 *= 32; year2 -= 20;
		year.negate(); year += 2150; year *= Number(5628, 10000); year += year2; year /= 86400;
		return year;
	}
}

Number dynamical_from_universal(Number tee_rom_u) {
	tee_rom_u += ephemeris_correction(tee_rom_u);
	return tee_rom_u;
}

#define J2000 Number("730120.5")


Number julian_centuries(Number tee) {
	tee = dynamical_from_universal(tee);
	tee -= J2000;
	tee /= 36525;
	return tee;
}

Number nutation(Number tee) {
	Number c = julian_centuries(tee);
	Number cap_a;
	Number t, x(c);
	t.setFloat(124.90L); cap_a += t;
	t.setFloat(-1934.134L); t *= x; cap_a += t;
	x *= c; t.setFloat(0.002063L); t *= x; cap_a += t;
	Number cap_b;
	x = c;
	t.setFloat(201.11L); cap_b += t;
	t.setFloat(72001.5377L); t *= x; cap_b += t;
	x *= c; t.setFloat(0.00057L); t *= x; cap_b += t;
	Number nr_pi; nr_pi.pi();
	t.setFloat(-0.004778L);
	cap_a *= nr_pi; cap_a /= 180; cap_a.sin(); cap_a *= t;
	t.setFloat(-0.0003667L);
	cap_b *= nr_pi; cap_b /= 180; cap_b.sin(); cap_b *= t;
	cap_a += cap_b;
	return cap_a;
}

Number aberration(Number tee) {
	Number c = julian_centuries(tee);
	Number t;
	t.setFloat(35999.01848L);
	c *= t;
	t.setFloat(177.63L);
	c += t;
	Number nr_pi; nr_pi.pi();
	c *= nr_pi; c /= 180;
	c.cos();
	t.setFloat(0.0000974L);
	c *= t;
	t.setFloat(0.005575L);
	c -= t;
	return c;
}

Number solar_longitude(Number tee) {
	Number c = julian_centuries(tee);
	Number lam; lam.setFloat(282.7771834L);
	Number t2; t2.setFloat(36000.76953744L); t2 *= c;
	Number t3;
	long int coefficients[] = {
	403406L, 195207L, 119433L, 112392L, 3891L, 2819L, 1721L,
	660L, 350L, 334L, 314L, 268L, 242L, 234L, 158L, 132L, 129L, 114L,
	99L, 93L, 86L, 78L, 72L, 68L, 64L, 46L, 38L, 37L, 32L, 29L, 28L, 27L, 27L,
	25L, 24L, 21L, 21L, 20L, 18L, 17L, 14L, 13L, 13L, 13L, 12L, 10L, 10L, 10L,
	10L, -1L
	};
	long double multipliers[] = {
	0.9287892L, 35999.1376958L, 35999.4089666L,
	35998.7287385L, 71998.20261L, 71998.4403L,
	36000.35726L, 71997.4812L, 32964.4678L,
	-19.4410L, 445267.1117L, 45036.8840L, 3.1008L,
	22518.4434L, -19.9739L, 65928.9345L,
	9038.0293L, 3034.7684L, 33718.148L, 3034.448L,
	-2280.773L, 29929.992L, 31556.493L, 149.588L,
	9037.750L, 107997.405L, -4444.176L, 151.771L,
	67555.316L, 31556.080L, -4561.540L,
	107996.706L, 1221.655L, 62894.167L,
	31437.369L, 14578.298L, -31931.757L,
	34777.243L, 1221.999L, 62894.511L,
	-4442.039L, 107997.909L, 119.066L, 16859.071L,
	-4.578L, 26895.292L, -39.127L, 12297.536L,
	90073.778L
	};
	long double addends[] = {
	270.54861L, 340.19128L, 63.91854L, 331.26220L,
	317.843L, 86.631L, 240.052L, 310.26L, 247.23L,
	260.87L, 297.82L, 343.14L, 166.79L, 81.53L,
	3.50L, 132.75L, 182.95L, 162.03L, 29.8L,
	266.4L, 249.2L, 157.6L, 257.8L, 185.1L, 69.9L,
	8.0L, 197.1L, 250.4L, 65.3L, 162.7L, 341.5L,
	291.6L, 98.5L, 146.7L, 110.0L, 5.2L, 342.6L,
	230.9L, 256.1L, 45.3L, 242.9L, 115.2L, 151.8L,
	285.3L, 53.3L, 126.6L, 205.7L, 85.9L,
	146.1L
	};
	Number x, y, z, nr_pi; nr_pi.pi();
	for(size_t i = 0; coefficients[i] >= 0; i++) {
		x = coefficients[i];
		y.setFloat(addends[i]);
		z.setFloat(multipliers[i]);
		z *= c;
		z += y;
		z *= nr_pi;
		z /= 180;
		z.sin();
		z *= x;
		t3 += z;
	}
	Number t; t.setFloat(0.000005729577951308232L);
	t3 *= t;
	lam += t2; lam += t3;
	lam += aberration(tee);
	lam += nutation(tee);
	lam.mod(360);
	return lam;
}

Number solar_longitude_after(Number longitude, Number tee) {
	Number rate = MEAN_TROPICAL_YEAR; rate /= 360;
	Number tau(longitude); tau -= solar_longitude(tee); tau.mod(360); tau *= rate; tau += tee;
	Number a(tau); a -= 5; if(tee > a) a = tee;
	Number b(tau); b += 5;
	Number along = solar_longitude(a);
	Number blong = solar_longitude(b);
	Number precexp(1, 1, -5);
	Number long_low(longitude); long_low -= precexp;
	Number long_high(longitude); long_high += precexp;
	if(long_low < 0) long_low += 360;
	if(long_high > 360) long_high -= 360;
	Number newlong;
	Number test(a);
	while(true) {
		if(CALCULATOR->aborted()) return nr_zero;
		test = b; test -= a; test /= 2; test += a;
		newlong = solar_longitude(test);
		if(long_high < long_low) {
			if(newlong >= long_low || newlong <= long_high) return test;
		} else {
			if(newlong >= long_low && newlong <= long_high) return test;
		}
		if(along > blong) {
			if((newlong > longitude && newlong < along) || (newlong < longitude && newlong < along)) b = test;
			else a = test;
		} else {
			if(newlong > longitude) b = test;
			else a = test;
		}
	}
}
Number obliquity(Number tee) {
	Number c = julian_centuries(tee);
	tee.setFloat(21.448L); tee /= 60; tee += 26; tee /= 60; tee += 23;
	Number t, x(c);
	t.setFloat(-46.8150L); t /= 3600; t *= x; tee += t;
	x *= c; t.setFloat(-0.00059L); t /= 3600; t *= x; tee += t;
	x *= c; t.setFloat(0.001813L); t /= 3600; t *= x; tee += t;
	return tee;
}

Number cal_poly(Number c, size_t n, ...) {
	va_list ap;
	va_start(ap, n);
	Number x(1, 1, 0), t, poly;
	for(size_t i = 0; i < n; i++) {
		t.setFloat(va_arg(ap, long double));
		t *= x;
		poly += t;
		x *= c;
	}
	va_end(ap);
	return poly;
}

Number equation_of_time(Number tee) {
	Number c = julian_centuries(tee);
	vector<long double> a;
	Number lam, anomaly, eccentricity;
	Number t, x(c);
	t.setFloat(280.46645L); lam += t;
	t.setFloat(36000.76983L); t *= x; lam += t;
	x *= c; t.setFloat(0.0003032L); t *= x; lam += t;
	x = c;
	t.setFloat(357.52910L); anomaly += t;
	t.setFloat(35999.05030L); t *= x; anomaly += t;
	x *= c; t.setFloat(-0.0001559L); t *= x; anomaly += t;
	x *= c; t.setFloat(-0.00000048L); t *= x; anomaly += t;
	x = c;
	t.setFloat(0.016708617L); eccentricity += t;
	t.setFloat(-0.000042037L); t *= x; eccentricity += t;
	x *= c; t.setFloat(-0.0000001236L); t *= x; eccentricity += t;
	Number varepsilon = obliquity(tee);
	Number nr_pi; nr_pi.pi();
	Number y(varepsilon); y /= 2; y *= nr_pi; y /= 180; y.tan(); y.square();
	Number equation(1, 2); equation /= nr_pi;
	Number e1(lam); e1 *= 2; e1 *= nr_pi; e1 /= 180; e1.sin(); e1 *= y;
	Number e2(anomaly); e2 *= nr_pi; e2 /= 180; e2.sin(); e2 *= eccentricity;
	Number e3(lam); e3 *= 2; e3 *= nr_pi; e3 /= 180; e3.cos(); e3 *= e2; e3 *= y; e3 *= 4; e2 *= -2;
	Number e4(lam); e4 *= 4; e4 *= nr_pi; e4 /= 180; e4.sin(); e4 *= y; e4 *= y; e4 /= -2;
	Number e5(anomaly); e5 *= 2; e5 *= nr_pi; e5 /= 180; e5.sin(); e5 *= eccentricity; e5 *= eccentricity; e5 *= -5; e5 /= 4;
	e1 += e2; e1 += e3; e1 += e4; e1 += e5; equation *= e1;
	bool b_neg = equation.isNegative();
	equation.abs();
	if(equation < nr_half) {
		if(b_neg) equation.negate();
		return equation;
	} else {
		if(b_neg) return nr_minus_half;
		return nr_half;
	}
}
Number zone_from_longitude(Number phi) {
	phi /= 360;
	return phi;
}
Number universal_from_local(Number tee_ell, Number longitude) {
	tee_ell -= zone_from_longitude(longitude);
	return tee_ell;
}
Number local_from_apparent(Number tee, Number longitude) {
	tee -= equation_of_time(universal_from_local(tee, longitude));
	return tee;
}
Number universal_from_apparent(Number tee, Number longitude) {
	return universal_from_local(local_from_apparent(tee, longitude), longitude);
}
Number universal_from_standard(Number tee_rom_s, Number zone) {
	tee_rom_s -= zone;
	return tee_rom_s;
}
Number standard_from_universal(Number tee_rom_u, Number zone) {
	tee_rom_u += zone;
	return tee_rom_u;
}
Number standard_from_local(Number tee_ell, Number longitude, Number zone) {
	return standard_from_universal(universal_from_local(tee_ell, longitude), zone);
}
Number midday2(Number date, Number longitude, Number zone) {
	date += nr_half;
	return standard_from_local(local_from_apparent(date, longitude), longitude, zone);
}
Number midday(Number date, Number longitude) {
	date += nr_half;
	return universal_from_apparent(date, longitude);
}

#define TEHRAN_LONGITUDE Number("51.42")
#define TEHRAN_ZONE Number(7, 48)

Number midday_in_tehran(Number date) {
	return midday(date, TEHRAN_LONGITUDE);
}

Number chinese_zone(Number tee) {
	tee.floor();
	if(gregorian_year_from_fixed(tee) < 1929) return Number(1397, 4320);
	return Number(1, 3);
}

Number midnight_in_china(Number date) {
	return universal_from_standard(date, chinese_zone(date));
}

Number estimate_prior_solar_longitude(Number lam, Number tee) {
	Number rate = MEAN_TROPICAL_YEAR; rate /= 360;
	Number tau = solar_longitude(tee); tau -= lam; tau.mod(360); tau *= rate;
	tau.negate();
	tau += tee;
	Number cap_delta = solar_longitude(tau); cap_delta -= lam; cap_delta += 180; cap_delta.mod(360); cap_delta -= 180;
	cap_delta *= rate;
	tau -= cap_delta;
	if(tau < tee) return tau;
	return tee;
}

Number persian_new_year_on_or_before(Number date) {
	Number approx = estimate_prior_solar_longitude(nr_zero, midday_in_tehran(date));
	approx.floor(); approx -= 1;
	Number day(approx);
	while(solar_longitude(midday_in_tehran(day)).isGreaterThan(2)) {
		if(CALCULATOR->aborted()) break;
		day++;
	}
	return day;
}

Number universal_from_dynamical(Number tee) {
	tee -= ephemeris_correction(tee);
	return tee;
}

Number mean_lunar_longitude(Number c) {
	c = cal_poly(c, 5, 218.3164477L, 481267.88123421L, -0.0015786L, 1.0L/538841.0L, -1.0L/65194000.0L);
	c.mod(360);
	return c;
}
Number lunar_elongation(Number c) {
	c = cal_poly(c, 5, 297.8501921L, 445267.1114034L, -0.0018819L, 1.0L/545868.0L, -1.0L/113065000.0L);
	c.mod(360);
	return c;
}
Number solar_anomaly(Number c) {
	c = cal_poly(c, 4, 357.5291092L, 35999.0502909L, -0.0001536L, 1.0L/24490000.0L);
	c.mod(360);
	return c;
}
Number lunar_anomaly(Number c) {
	c = cal_poly(c, 5, 134.9633964L, 477198.8675055L, 0.0087414L, 1.0L/69699.0L, -1.0L/14712000.0L);
	c.mod(360);
	return c;
}
Number moon_node(Number c) {
	c = cal_poly(c, 5, 93.2720950L, 483202.0175233L, -0.0036539L, -1.0L/3526000.0L, 1.0L/863310000.0L);
	c.mod(360);
	return c;
}

Number lunar_longitude(Number tee) {
	Number c = julian_centuries(tee);
	Number cap_L_prime = mean_lunar_longitude(c);
	Number cap_D = lunar_elongation(c);
	Number cap_M = solar_anomaly(c);
	Number cap_M_prime = lunar_anomaly(c);
	Number cap_F = moon_node(c);
	Number cap_E = cal_poly(c, 3, 1.0L, -0.002516L, -0.0000074L);
	Number correction;
	int args_lunar_elongation[] = {
	0, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 1, 0, 2, 0, 0, 4, 0, 4, 2, 2, 1,
	1, 2, 2, 4, 2, 0, 2, 2, 1, 2, 0, 0, 2, 2, 2, 4, 0, 3, 2, 4, 0, 2,
	2, 2, 4, 0, 4, 1, 2, 0, 1, 3, 4, 2, 0, 1, 2, -1
	};
	int args_solar_anomaly[] = {
	0, 0, 0, 0, 1, 0, 0, -1, 0, -1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1,
	0, 1, -1, 0, 0, 0, 1, 0, -1, 0, -2, 1, 2, -2, 0, 0, -1, 0, 0, 1,
	-1, 2, 2, 1, -1, 0, 0, -1, 0, 1, 0, 1, 0, 0, -1, 2, 1, 0
	};
	int args_lunar_anomaly[] = {
	1, -1, 0, 2, 0, 0, -2, -1, 1, 0, -1, 0, 1, 0, 1, 1, -1, 3, -2,
	-1, 0, -1, 0, 1, 2, 0, -3, -2, -1, -2, 1, 0, 2, 0, -1, 1, 0,
	-1, 2, -1, 1, -2, -1, -1, -2, 0, 1, 4, 0, -2, 0, 2, 1, -2, -3,
	2, 1, -1, 3
	};
	int args_moon_node[] = {
	0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, -2, 2, -2, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, -2, 2, 0, 2, 0, 0, 0, 0,
	0, 0, -2, 0, 0, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0
	};
	long int sine_coeff[] = {
	6288774L, 1274027L, 658314L, 213618L, -185116L, -114332L,
	58793L, 57066L, 53322L, 45758L, -40923L, -34720L, -30383L,
	15327L, -12528L, 10980L, 10675L, 10034L, 8548L, -7888L,
	-6766L, -5163L, 4987L, 4036L, 3994L, 3861L, 3665L, -2689L,
	-2602L, 2390L, -2348L, 2236L, -2120L, -2069L, 2048L, -1773L,
	-1595L, 1215L, -1110L, -892L, -810L, 759L, -713L, -700L, 691L,
	596L, 549L, 537L, 520L, -487L, -399L, -381L, 351L, -340L, 330L,
	327L, -323L, 299L, 294L
	};
	Number v, w, x, x2, x3, y, z, nr_pi; nr_pi.pi();
	for(size_t i = 0; args_lunar_elongation[i] >= 0; i++) {
		v = sine_coeff[i];
		w = args_lunar_elongation[i];
		x = args_solar_anomaly[i];
		y = args_lunar_anomaly[i];
		z = args_moon_node[i];
		x2 = x;
		x2.abs();
		x3 = cap_E;
		x3 ^= x2;
		v *= x3;
		w *= cap_D;
		x *= cap_M;
		y *= cap_M_prime;
		z *= cap_F;
		w += x; w += y; w += z;
		w *= nr_pi;
		w /= 180;
		w.sin();
		v *= w;
		correction += v;
	}
	correction *= Number(1, 1, -6);
	Number venus; venus.setFloat(131.849L); venus *= c; v.setFloat(119.75L); venus += v; venus *= nr_pi; venus /= 180; venus.sin(); venus *= Number(3958, 1000000L);
	Number jupiter; jupiter.setFloat(479264.29L); jupiter *= c; v.setFloat(53.09L); jupiter += v; jupiter *= nr_pi; jupiter /= 180; jupiter.sin(); jupiter *= Number(318, 1000000L);
	Number flat_earth(cap_L_prime); flat_earth -= cap_F; flat_earth *= nr_pi; flat_earth /= 180; flat_earth.sin(); flat_earth *= Number(1962, 1000000L);
	Number ret(cap_L_prime); ret += correction; ret += venus; ret += jupiter; ret += flat_earth; ret += nutation(tee); ret.mod(360);
	return ret;
}

Number nth_new_moon(Number n) {
	Number n0(24724);
	Number k(n); k -= n0;
	Number c; c.setFloat(1236.85L); c.recip(); c *= k;
	Number approx = J2000; approx += cal_poly(c, 5, 5.09766L, 29.530588861L * 1236.85L, 0.00015437L, -0.000000150L, 0.00000000073L);
	Number cap_E = cal_poly(c, 3, 1.0L, -0.002516L, -0.0000074L);
	Number solar_anomaly_n = cal_poly(c, 4, 2.5534L, 1236.85L * 29.10535670L, -0.0000014L, -0.00000011L);
	Number lunar_anomaly_n = cal_poly(c, 5, 201.5643L, 385.81693528L * 1236.85L, 0.0107582L, 0.00001238L, -0.000000058L);
	Number moon_argument = cal_poly(c, 5, 160.7108L, 390.67050284L * 1236.85L, -0.0016118L, -0.00000227L, 0.000000011L);
	Number cap_omega = cal_poly(c, 4, 124.7746L, -1.56375588L * 1236.85L, 0.0020672L, 0.00000215L);
	int e_factor[] = {0, 1, 0, 0, 1, 1, 2, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1};
	int solar_coeff[] = {0, 1, 0, 0, -1, 1, 2, 0, 0, 1, 0, 1, 1, -1, 2, 0, 3, 1, 0, 1, -1, -1, 1, 0};
	int lunar_coeff[] = {1, 0, 2, 0, 1, 1, 0, 1, 1, 2, 3, 0, 0, 2, 1, 2, 0, 1, 2, 1, 1, 1, 3, 4};
	int moon_coeff[] = {0, 0, 0, 2, 0, 0, 0, -2, 2, 0, 0, 2, -2, 0, 0, -2, 0, -2, 2, 2, 2, -2, 0, 0};
	long double sine_coeff[] = { -0.40720L, 0.17241L, 0.01608L, 0.01039L, 0.00739L, -0.00514L, 0.00208L, -0.00111L, -0.00057L, 0.00056L, -0.00042L, 0.00042L, 0.00038L, -0.00024L, -0.00007L, 0.00004L, 0.00004L, 0.00003L, 0.00003L, -0.00003L, 0.00003L, -0.00002L, -0.00002L, 0.00002L};
	Number v, w, x, x2, y, z, nr_pi; nr_pi.pi();
	Number correction; correction.setFloat(-0.00017L); cap_omega *= nr_pi; cap_omega /= 180; cap_omega.sin(); correction *= cap_omega;
	for(size_t i = 0; e_factor[i] >= 0; i++) {
		v.setFloat(sine_coeff[i]);
		w = e_factor[i];
		x = solar_coeff[i];
		y = lunar_coeff[i];
		z = moon_coeff[i];
		x *= solar_anomaly_n;
		y *= lunar_anomaly_n;
		z *= moon_argument;
		x += y; x += z;
		x *= nr_pi;
		x /= 180;
		x.sin();
		x2 = cap_E; x2 ^= w;
		v *= x2; v *= x;
		correction += v;
	}
	long double add_const[] = {251.88L, 251.83L, 349.42L, 84.66L, 141.74L, 207.14L, 154.84L, 34.52L, 207.19L, 291.34L, 161.72L, 239.56L, 331.55L, -1.0L};
	long double add_coeff[] = {0.016321L, 26.651886L, 36.412478L, 18.206239L, 53.303771L, 2.453732L, 7.306860L, 27.261239L, 0.121824L, 1.844379L, 24.198154L, 25.513099L, 3.592518L};
	long double add_factor[] = {0.000165L, 0.000164L, 0.000126L, 0.000110L, 0.000062L, 0.000060L, 0.000056L, 0.000047L, 0.000042L, 0.000040L, 0.000037L, 0.000035L, 0.000023L};
	Number extra = cal_poly(c, 3, 299.77L, 132.8475848L, -0.009173L); extra *= nr_pi; extra /= 180; extra.sin(); v.setFloat(0.000325L); extra *= v;
	Number additional;
	for(size_t i = 0; add_const[i] >= 0; i++) {
		x.setFloat(add_const[i]);
		y.setFloat(add_coeff[i]);
		z.setFloat(add_factor[i]);
		y *= k; y += x; y *= nr_pi; y /= 180; y.sin(); y *= z;
		additional += y;
	}
	approx += correction; approx += extra; approx += additional;
	return universal_from_dynamical(approx);
}

Number lunar_phase(Number tee) {
	Number phi = lunar_longitude(tee); phi -= solar_longitude(tee); phi.mod(360);
	Number t0 = nth_new_moon(0);
	Number n(tee); n -= t0; n /= MEAN_SYNODIC_MONTH; n.round();
	Number phi_prime(tee); phi_prime -= nth_new_moon(n); phi_prime /= MEAN_SYNODIC_MONTH; phi_prime.mod(1); phi_prime *= 360;
	Number test(phi); test -= phi_prime; test.abs();
	if(test > 180) return phi_prime;
	return phi;
}

Number lunar_phase_at_or_after(Number phase, Number tee) {
	Number rate = MEAN_SYNODIC_MONTH; rate /= 360;
	Number tau(phase); tau -= lunar_phase(tee); tau.mod(360); tau *= rate; tau += tee;
	Number a(tau); a -= 5; if(tee > a) a = tee;
	Number b(tau); b += 5;
	Number precexp(1, 1, -5);
	Number phase_low(phase); phase_low -= precexp;
	Number phase_high(phase); phase_high += precexp;
	if(phase_low < 0) phase_low += 360;
	if(phase_high > 360) phase_high -= 360;
	Number newphase;
	Number test(a);
	while(true) {
		if(CALCULATOR->aborted()) return nr_zero;
		test = b; test -= a; test /= 2; test += a;
		newphase = lunar_phase(test);
		if(phase_high < phase_low) {
			if(newphase >= phase_low || newphase <= phase_high) return test;
		} else {
			if(newphase >= phase_low && newphase <= phase_high) return test;
		}
		newphase -= phase; newphase.mod(360);
		if(newphase < 180) b = test;
		else a = test;
	}
}

Number new_moon_at_or_after(Number tee) {
	Number t0 = nth_new_moon(0);
	Number phi = lunar_phase(tee); phi /= 360;
	Number n(tee); n -= t0; n /= MEAN_SYNODIC_MONTH; n -= phi; n.round();
	while(nth_new_moon(n) < tee) {
		if(CALCULATOR->aborted()) break;
		n++;
	}
	return nth_new_moon(n);
}
Number new_moon_before(Number tee) {
	Number t0 = nth_new_moon(0);
	Number phi = lunar_phase(tee); phi /= 360;
	Number n(tee); n -= t0; n /= MEAN_SYNODIC_MONTH; n -= phi; n.round();
	n--;
	while(nth_new_moon(n) < tee) {
		if(CALCULATOR->aborted()) break;
		n++;
	}
	n--;
	return nth_new_moon(n);
}

Number chinese_new_moon_on_or_after(Number date) {
	Number tee = new_moon_at_or_after(midnight_in_china(date));
	Number ret = standard_from_universal(tee, chinese_zone(tee));
	ret.floor();
	return ret;
}
Number chinese_new_moon_before(Number date) {
	Number tee = new_moon_before(midnight_in_china(date));
	Number ret = standard_from_universal(tee, chinese_zone(tee));
	ret.floor();
	return ret;
}
Number chinese_solar_longitude_on_or_after(Number lam, Number tee) {
	Number sun = solar_longitude_after(lam, universal_from_standard(tee, chinese_zone(tee)));
	return standard_from_universal(sun, chinese_zone(sun));
}
Number current_major_solar_term(Number date) {
	Number s = solar_longitude(universal_from_standard(date, chinese_zone(date)));
	cal_div(s, 30); s += 2; s.mod(-12); s += 12;
	return s;
}
Number major_solar_term_on_or_after(Number date) {
	Number s = solar_longitude(midnight_in_china(date));
	Number l(s); l /= 30; l.ceil(); l *= 30; l.mod(360);
	return chinese_solar_longitude_on_or_after(l, date);
}
Number current_minor_solar_term(Number date) {
	Number s = solar_longitude(universal_from_standard(date, chinese_zone(date)));
	s -= 15; cal_div(s, 30); s += 3; s.mod(-12); s += 12;
	return s;
}
Number minor_solar_term_on_or_after(Number date) {
	Number s = solar_longitude(midnight_in_china(date));
	Number l(s); l -= 15; l /= 30; l.ceil(); l *= 30; l += 15; l.mod(360);
	return chinese_solar_longitude_on_or_after(l, date);
}
bool chinese_no_major_solar_term(Number date) {
	return current_major_solar_term(date) == current_major_solar_term(chinese_new_moon_on_or_after(date + 1));
}
bool chinese_prior_leap_month(Number m_prime, Number m) {
	if(CALCULATOR->aborted()) return false;
	return m >= m_prime && (chinese_no_major_solar_term(m) || chinese_prior_leap_month(m_prime, chinese_new_moon_before(m)));
}
Number chinese_winter_solstice_on_or_before(Number date) {
	date++;
	Number approx = estimate_prior_solar_longitude(270, midnight_in_china(date));
	approx.floor(); approx--;
	while(solar_longitude(midnight_in_china(approx + 1)) <= 270) {
		if(CALCULATOR->aborted()) break;
		approx++;
	}
	return approx;
}
Number chinese_new_year_in_sui(Number date) {
	Number s1 = chinese_winter_solstice_on_or_before(date);
	Number s2 = chinese_winter_solstice_on_or_before(s1 + 370);
	Number m12 = chinese_new_moon_on_or_after(s1 + 1);
	Number m13 = chinese_new_moon_on_or_after(m12 + 1);
	Number next_m11 = chinese_new_moon_before(s2 + 1);
	next_m11 -= m12; next_m11 /= MEAN_SYNODIC_MONTH; next_m11.round();
	if(next_m11 == 12 && (chinese_no_major_solar_term(m12) || chinese_no_major_solar_term(m13))) {
		m13++;
		return chinese_new_moon_on_or_after(m13);
	}
	return m13;
}
Number chinese_new_year_on_or_before(Number date) {
	Number new_year = chinese_new_year_in_sui(date);
	if(date >= new_year) return new_year;
	date -= 180;
	return chinese_new_year_in_sui(date);
}

bool gregorian_leap_year(long int year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
bool julian_leap_year(long int year) {
	if(year < 0) return year % 4 == -1;
	return year % 4 == 0;
}
bool coptic_leap_year(long int year) {
	if(year < 0) return year % 4 == -1;
	return year % 4 == 2;
}
bool hebrew_leap_year(Number y) {
	y *= 7; y++; y.mod(19);
	return y.isLessThan(7);
}
long int last_month_of_hebrew_year(Number year) {
	if(hebrew_leap_year(year)) return 13;
	return 12;
}
bool hebrew_sabbatical_year(Number year) {
	year.mod(7);
	return year.isZero();
}
Number hebrew_calendar_elapsed_days(Number year) {
	Number months_elapsed(year); months_elapsed *= 235; months_elapsed -= 234; cal_div(months_elapsed, 19);
	Number parts_elapsed(months_elapsed); parts_elapsed *= 13753; parts_elapsed += 12084;
	months_elapsed *= 29;
	cal_div(parts_elapsed, 25920);
	Number days(months_elapsed); days += parts_elapsed;
	Number days2(days); days2++; days2 *= 3; days2.mod(7);
	if(days2.isLessThan(3)) days++;
	return days;
}
long int hebrew_year_length_correction(Number year) {
	Number ny0, ny1, ny2;
	year--;
	ny0 = hebrew_calendar_elapsed_days(year);
	year++;
	ny1 = hebrew_calendar_elapsed_days(year);
	year++;
	ny2 = hebrew_calendar_elapsed_days(year);
	ny2 -= ny1;
	if(ny2 == 356) return 2;
	ny1 -= ny0;
	if(ny1 == 382) return 1;
	return 0;
}
Number hebrew_new_year(Number year) {
	Number d(HEBREW_EPOCH); d += hebrew_calendar_elapsed_days(year); d += hebrew_year_length_correction(year); return d;
}

long int days_in_hebrew_year(Number year) {
	Number d1(hebrew_new_year(year));
	d1.negate();
	year++;
	d1 += hebrew_new_year(year);
	return d1.lintValue();
}
bool long_marheshvan(Number year) {
	long int d = days_in_hebrew_year(year);
	return d == 355 || d == 385;
}
bool short_kislev(Number year) {
	long int d = days_in_hebrew_year(year);
	return d == 353 || d == 383;
}

long int last_day_of_hebrew_month(Number year, Number month) {
	if(month == 2 || month == 4 || month == 6 || month == 10 || month == 13) return 29;
	if(month == 12 && !hebrew_leap_year(year)) return 29;
	if(month == 8 && !long_marheshvan(year)) return 29;
	if(month == 9 && short_kislev(year)) return 29;
	return 30;
}

Number date_to_fixed(long int y, long int m, long int d, CalendarSystem ct) {
	Number fixed;
	if(ct == CALENDAR_GREGORIAN) {
		Number year(y); year--;
		fixed = year; fixed *= 365; fixed += quotient(year, 4); fixed -= quotient(year, 100); fixed += quotient(year, 400);
		fixed += quotient((367 * m) - 362, 12);
		if(m > 2) fixed -= (gregorian_leap_year(y) ? 1 : 2);
		fixed += d;
	} else if(ct == CALENDAR_HEBREW) {
		fixed = hebrew_new_year(y);
		fixed += d - 1;
		if(m < 7) {
			long int l = last_month_of_hebrew_year(y);
			for(long int i = 7; i <= l; i++) fixed += last_day_of_hebrew_month(y, i);
			for(long int i = 1; i < m; i++) fixed += last_day_of_hebrew_month(y, i);
		} else {
			for(long int i = 7; i < m; i++) fixed += last_day_of_hebrew_month(y, i);
		}
	} else if(ct == CALENDAR_JULIAN) {
		Number y2(y); if(!y2.isNegative()) y2--;
		fixed = JULIAN_EPOCH; fixed--;
		fixed += y2 * 365;
		fixed += quotient(y2, 4);
		fixed += quotient((367 * m) - 362, 12);
		if(m > 2) fixed -= (julian_leap_year(y) ? 1 : 2);
		fixed += d;
	} else if(ct == CALENDAR_ISLAMIC) {
		Number year(y);
		fixed = ISLAMIC_EPOCH; fixed--;
		fixed += (year - 1) * 354;
		year *= 11; year += 3; cal_div(year, 30); fixed += year;
		fixed += (m - 1) * 29;
		fixed += quotient(m, 2);
		fixed += d;
	} else if(ct == CALENDAR_PERSIAN) {
		Number year(y);
		if(year.isPositive()) year--;
		year *= MEAN_TROPICAL_YEAR;
		year.floor();
		year += 180;
		year += PERSIAN_EPOCH;
		fixed = persian_new_year_on_or_before(year); fixed--;
		if(m <= 7) fixed += 31 * (m - 1);
		else fixed += (30 * (m - 1)) + 6;
		fixed += d;
	} else if(ct == CALENDAR_CHINESE) {
		bool leap = (m > 12);
		if(leap) m -= 12;
		Number mid_year(y); mid_year -= 61; mid_year += nr_half; mid_year *= MEAN_TROPICAL_YEAR; mid_year += CHINESE_EPOCH;
		Number new_year = chinese_new_year_on_or_before(mid_year);
		new_year += (m - 1) * 29;
		Number p = chinese_new_moon_on_or_after(new_year);
		long int dy, dm, dd;
		fixed_to_date(p, dy, dm, dd, ct);
		bool dleap = (dm > 12); if(dleap) dm -= 12;
		if(m != dm || leap != dleap) {p++; p = chinese_new_moon_on_or_after(p);}
		fixed = d; fixed--; fixed += p;
	} else if(ct == CALENDAR_EGYPTIAN) {
		Number year(y);
		fixed = EGYPTIAN_EPOCH;
		fixed += (year - 1) * 365;
		fixed += 30 * (m - 1);
		fixed += d - 1;
	} else if(ct == CALENDAR_COPTIC) {
		Number year(y);
		fixed = COPTIC_EPOCH; fixed--;
		fixed += (year - 1) * 365;
		fixed += quotient(year, 4);
		fixed += 30 * (m - 1);
		fixed += d;
	} else if(ct == CALENDAR_ETHIOPIAN) {
		Number year(y);
		fixed = ETHIOPIAN_EPOCH;
		fixed += date_to_fixed(y, m, d, CALENDAR_COPTIC);
		fixed -= COPTIC_EPOCH;
	} else {
		return date_to_cjdn(y, m, d, ct) - 1721425L;
	}
	return fixed;
}

bool fixed_to_date(Number date, long int &y, long int &m, long int &d, CalendarSystem ct) {
	if(ct == CALENDAR_GREGORIAN) {
		Number d0, n400, d1, n100, d2, n4, d3, n1, year;
		d0 = date; d0 -= 1;
		cal_div(d0, 146097, n400, d1);
		cal_div(d1, 36524, n100, d2);
		cal_div(d2, 1461, n4, d3);
		cal_div(d3, 365, n1);
		if(!n100.equals(4) && !n1.equals(4)) year = 1;
		else year = 0;
		n400 *= 400; n100 *= 100; n4 *= 4; year += n400; year += n100; year += n4; year += n1;
		bool overflow = false;
		y = year.lintValue(&overflow);
		if(overflow) return false;
		Number prior_days(date); prior_days -= date_to_fixed(y, 1, 1, ct);
		if(date.isGreaterThanOrEqualTo(date_to_fixed(y, 3, 1, ct))) prior_days += gregorian_leap_year(y) ? 1 : 2;
		prior_days *= 12; prior_days += 373;
		cal_div(prior_days, 367);
		m = prior_days.lintValue();
		date -= date_to_fixed(y, m, 1, ct); date++;
		d = date.lintValue();
		return true;
	} else if(ct == CALENDAR_HEBREW) {
		Number approx(date); approx -= HEBREW_EPOCH; approx /= 35975351L; approx *= 98496; approx.floor(); approx++;
		Number year(approx); year--;
		while(hebrew_new_year(year).isLessThanOrEqualTo(date)) {
			if(CALCULATOR->aborted()) return false;
			year++;
		}
		year--;
		bool overflow = false;
		y = year.lintValue(&overflow);
		if(overflow) return false;
		m = 1;
		if(date.isLessThan(date_to_fixed(y, 1, 1, ct))) m = 7;
		while(date.isGreaterThan(date_to_fixed(y, m, last_day_of_hebrew_month(y, m), ct))) {
			if(CALCULATOR->aborted()) return false;
			m++;
		}
		date -= date_to_fixed(y, m, 1, ct); date++;
		d = date.lintValue();
		return true;
	} else if(ct == CALENDAR_JULIAN) {
		Number approx(date); approx -= JULIAN_EPOCH; approx *= 4; approx += 1464; cal_div(approx, 1461);
		if(!approx.isPositive()) approx--;
		bool overflow = false;
		y = approx.lintValue(&overflow);
		if(overflow) return false;
		Number prior_days(date); prior_days -= date_to_fixed(y, 1, 1, ct);
		if(date.isGreaterThanOrEqualTo(date_to_fixed(y, 3, 1, ct))) prior_days += julian_leap_year(y) ? 1 : 2;
		prior_days *= 12; prior_days += 373;
		cal_div(prior_days, 367);
		m = prior_days.lintValue();
		date -= date_to_fixed(y, m, 1, ct); date++;
		d = date.lintValue();
		return true;
	} else if(ct == CALENDAR_ISLAMIC) {
		Number year(date); year -= ISLAMIC_EPOCH; year *= 30; year += 10646; cal_div(year, 10631);
		bool overflow = false;
		y = year.lintValue(&overflow);
		if(overflow) return false;
		Number prior_days(date); prior_days -= date_to_fixed(y, 1, 1, ct);
		prior_days *= 11; prior_days += 330; cal_div(prior_days, 325);
		m = prior_days.lintValue();
		date -= date_to_fixed(y, m, 1, ct); date++;
		d = date.lintValue();
		return true;
	} else if(ct == CALENDAR_PERSIAN) {
		Number new_year = persian_new_year_on_or_before(date);
		Number y2 = new_year; y2 -= PERSIAN_EPOCH; y2 /= MEAN_TROPICAL_YEAR; y2.round(); y2++;
		Number year = y2;
		if(y2.isNonPositive()) year--;
		bool overflow = false;
		y = year.lintValue(&overflow);
		if(overflow) return false;
		Number day_of_year(date); day_of_year -= date_to_fixed(y, 1, 1, CALENDAR_PERSIAN); day_of_year++;
		Number month = day_of_year;
		if(day_of_year <= 186) {month /= 31; month.ceil();}
		else {month -= 6; month /= 30; month.ceil();}
		m = month.lintValue();
		date++; date -= date_to_fixed(y, m, 1, CALENDAR_PERSIAN);
		d = date.lintValue();
		return true;
	} else if(ct == CALENDAR_CHINESE) {
		if(date > Number(1, 1, 8) || date < Number(-1, 1, 8)) return false;
		Number s1 = chinese_winter_solstice_on_or_before(date);
		Number s2 = chinese_winter_solstice_on_or_before(s1 + 370);
		Number m12 = chinese_new_moon_on_or_after(s1 + 1);
		Number next_m11 = chinese_new_moon_before(s2 + 1);
		Number m1 = chinese_new_moon_before(date + 1);
		Number test(next_m11); test -= m12; test /= MEAN_SYNODIC_MONTH; test.round();
		bool leap_year = (test == 12);
		Number month(m1); month -= m12; month /= MEAN_SYNODIC_MONTH; month.round();
		if(leap_year && chinese_prior_leap_month(m12, m1)) month--;
		month.mod(-12); month += 12;
		m = month.lintValue();
		bool leap_month = (leap_year && chinese_no_major_solar_term(m1) && !chinese_prior_leap_month(m12, chinese_new_moon_before(m1)));
		if(leap_month) m += 12;
		Number elapsed_years(date); elapsed_years -= CHINESE_EPOCH; elapsed_years /= MEAN_TROPICAL_YEAR; elapsed_years += Number(3, 2); month /= 12; elapsed_years -= month; elapsed_years.floor();
		elapsed_years += 60; //epoch of 2697 BCE
		Number day(date); day -= m1; day++;
		if(day <= 0) return false;
		d = day.lintValue();
		bool overflow = false;
		y = elapsed_years.lintValue(&overflow);
		if(overflow) return false;
		return true;
	} else if(ct == CALENDAR_EGYPTIAN) {
		date -= EGYPTIAN_EPOCH;
		Number year(date); cal_div(year, 365); year++;
		bool overflow = false;
		y = year.lintValue(&overflow);
		if(overflow) return false;
		Number month(date); month.mod(365); cal_div(month, 30); month++;
		m = month.lintValue();
		year--; year *= 365; month--; month *= 30; date -= year; date -= month; date++;
		d = date.lintValue();
		return true;
	} else if(ct == CALENDAR_COPTIC) {
		Number year(date); year -= COPTIC_EPOCH; year *= 4; year += 1463; cal_div(year, 1461);
		bool overflow = false;
		y = year.lintValue(&overflow);
		if(overflow) return false;
		Number month(date); month -= date_to_fixed(y, 1, 1, ct); cal_div(month, 30); month++;
		m = month.lintValue();
		Number day(date); day -= date_to_fixed(y, m, 1, ct); day++;
		d = day.lintValue();
		return true;
	} else if(ct == CALENDAR_ETHIOPIAN) {
		date -= ETHIOPIAN_EPOCH;
		date += COPTIC_EPOCH;
		return fixed_to_date(date, y, m, d, CALENDAR_COPTIC);
	} else if(ct == CALENDAR_MILANKOVIC || ct == CALENDAR_INDIAN) {
		date += 1721425L;
		return cjdn_to_date(date, y, m, d, ct);
	}
	return false;
}
Number date_to_cjdn(long int j, long int m, long int d, CalendarSystem ct) {
	Number J;
	if(ct == CALENDAR_GREGORIAN) {
		Number c0(m); c0 -= 3; c0 /= 12; c0.floor();
		Number x4(j); x4 += c0;
		Number x2, x3; cal_div(x4, 100, x3, x2);
		Number x1(m); c0 *= 12; x1 -= c0; x1 -= 3;
		x3 *= 146097; x3 /= 4; x3.floor();
		x2 *= 36525; x2 /= 100; x2.floor();
		x1 *= 153; x1 += 2; x1 /= 5; x1.floor();
		J = x3; J += x2; J += x1; J += d; J += 1721119;
	} else if(ct == CALENDAR_MILANKOVIC) {
		Number c0(m); c0 -= 3; c0 /= 12; c0.floor();
		Number x4(j); x4 += c0;
		Number x2, x3; cal_div(x4, 100, x3, x2);
		Number x1(m); c0 *= 12; x1 -= c0; x1 -= 3;
		x3 *= 328718; x3 += 6; x3 /= 9; x3.floor();
		x2 *= 36525; x2 /= 100; x2.floor();
		x1 *= 153; x1 += 2; x1 /= 5; x1.floor();
		J = x3; J += x2; J += x1; J += d; J += 1721119;
	} else if(ct == CALENDAR_JULIAN) {
		Number c0(m); c0 -= 3; c0 /= 12; c0.floor();
		Number j1(j); j1 += c0; j1 *= 1461; j1 /= 4; j1.floor();
		Number j2(m); j2 *= 153; c0 *= 1836; j2 -= c0; j2 -= 457; j2 /= 5; j2.floor();
		J = 1721117L; J += j1; J += j2; J += d;
	} else if(ct == CALENDAR_ISLAMIC) {
		Number x1(j); x1 *= 10631; x1 -= 10617; x1 /= 30; x1.floor();
		Number x2(m); x2 *= 325; x2 -= 320; x2 /= 11; x2.floor();
		J = x1; J += x2; J += d; J += 1948439L;
		return J;
	} else if(ct == CALENDAR_HEBREW) {
		Number c0, x1, x3, z4;
		c0 = 13; c0 -= m; c0 /= 7; c0.floor();
		x1 = j; x1--; x1 += c0;
		x3 = m; x3--;
		z4 = d; z4--;
		Number c1x1, qx1, rx1, v1x1, v2x1;
		c1x1 = x1; c1x1 *= 235; c1x1++; c1x1 /= 19; c1x1.floor();
		qx1 = c1x1; qx1 /= 1095; qx1.floor();
		rx1 = c1x1; rx1.mod(1095);
		v1x1 = qx1; v1x1 *= 15; rx1 *= 765433L; v1x1 += rx1; v1x1 += 12084; v1x1 /= 25920; v1x1.floor(); qx1 *= 32336; v1x1 += qx1;
		v2x1 = v1x1; v2x1.mod(7); v2x1 *= 6; v2x1 /= 7; v2x1.floor(); v2x1.mod(2); v2x1 += v1x1;
		Number x1p1, c1x1p1, qx1p1, rx1p1, v1x1p1, v2x1p1;
		x1p1 = x1; x1p1++;
		c1x1p1 = x1p1; c1x1p1 *= 235; c1x1p1++; c1x1p1 /= 19; c1x1p1.floor();
		qx1p1 = c1x1p1; qx1p1 /= 1095; qx1p1.floor();
		rx1p1 = c1x1p1; rx1p1.mod(1095);
		v1x1p1 = qx1p1; v1x1p1 *= 15; rx1p1 *= 765433L; v1x1p1 += rx1p1; v1x1p1 += 12084; v1x1p1 /= 25920; v1x1p1.floor(); qx1p1 *= 32336; v1x1p1 += qx1p1;
		v2x1p1 = v1x1p1; v2x1p1.mod(7); v2x1p1 *= 6; v2x1p1 /= 7; v2x1p1.floor(); v2x1p1.mod(2); v2x1p1 += v1x1p1;
		Number x1m1, c1x1m1, qx1m1, rx1m1, v1x1m1, v2x1m1;
		x1m1 = x1; x1m1--;
		c1x1m1 = x1m1; c1x1m1 *= 235; c1x1m1++; c1x1m1 /= 19; c1x1m1.floor();
		qx1m1 = c1x1m1; qx1m1 /= 1095; qx1m1.floor();
		rx1m1 = c1x1m1; rx1m1.mod(1095);
		v1x1m1 = qx1m1; v1x1m1 *= 15; rx1m1 *= 765433L; v1x1m1 += rx1m1; v1x1m1 += 12084; v1x1m1 /= 25920; v1x1m1.floor(); qx1m1 *= 32336; v1x1m1 += qx1m1;
		v2x1m1 = v1x1m1; v2x1m1.mod(7); v2x1m1 *= 6; v2x1m1 /= 7; v2x1m1.floor(); v2x1m1.mod(2); v2x1m1 += v1x1m1;
		Number x1p2, c1x1p2, qx1p2, rx1p2, v1x1p2, v2x1p2;
		x1p2 = x1; x1p2 += 2;
		c1x1p2 = x1p2; c1x1p2 *= 235; c1x1p2++; c1x1p2 /= 19; c1x1p2.floor();
		qx1p2 = c1x1p2; qx1p2 /= 1095; qx1p2.floor();
		rx1p2 = c1x1p2; rx1p2.mod(1095);
		v1x1p2 = qx1p2; v1x1p2 *= 15; rx1p2 *= 765433L; v1x1p2 += rx1p2; v1x1p2 += 12084; v1x1p2 /= 25920; v1x1p2.floor(); qx1p2 *= 32336; v1x1p2 += qx1p2;
		v2x1p2 = v1x1p2; v2x1p2.mod(7); v2x1p2 *= 6; v2x1p2 /= 7; v2x1p2.floor(); v2x1p2.mod(2); v2x1p2 += v1x1p2;
		Number L2x1, L2x1m1, v3x1, v4x1, c2x1;
		L2x1 = v2x1p1; L2x1 -= v2x1;
		L2x1m1 = v2x1; L2x1m1 -= v2x1m1;
		v3x1 = L2x1; v3x1 += 19; v3x1 /= 15; v3x1.floor(); v3x1.mod(2); v3x1 *= 2;
		v4x1 = L2x1m1; v4x1 += 7; v4x1 /= 15; v4x1.floor(); v4x1.mod(2);
		c2x1 = v2x1; c2x1 += v3x1; c2x1 += v4x1;
		Number L2x1p1, v3x1p1, v4x1p1, c2x1p1;
		L2x1p1 = v2x1p2; L2x1p1 -= v2x1p1;
		v3x1p1 = L2x1p1; v3x1p1 += 19; v3x1p1 /= 15; v3x1p1.floor(); v3x1p1.mod(2); v3x1p1 *= 2;
		v4x1p1 = L2x1; v4x1p1 += 7; v4x1p1 /= 15; v4x1p1.floor(); v4x1p1.mod(2);
		c2x1p1 = v2x1p1; c2x1p1 += v3x1p1; c2x1p1 += v4x1p1;
		Number L, c8, c9, c3, c4;
		L = c2x1p1; L -= c2x1;
		c8 = L; c8 += 7; c8 /= 2; c8.floor(); c8.mod(15);
		c9 = 385; c9 -= L; c9 /= 2; c9.floor(); c9.mod(15); c9.negate();
		Number x3a(x3), x3b(x3), x3c(x3);
		x3a *= 384; x3a += 7; x3a /= 13; x3a.floor(); x3b += 4; x3b /= 12; x3b.floor(); x3c += 3; x3c /= 12; x3c.floor();
		c3 = x3a; c8 *= x3b; c3 += c8; c9 *= x3c; c3 += c9;
		J = 347821L; J += c2x1; J += c3; J += z4;
	} else if(ct == CALENDAR_EGYPTIAN) {
		j *= 365; m *= 30;
		J = j; J += m; J += d; J+= 1448242L;
	} else if(ct == CALENDAR_INDIAN) {
		j += 78;
		bool leap = gregorian_leap_year(j);
		J = date_to_cjdn(j, 3, leap ? 21 : 22, CALENDAR_GREGORIAN);
		if(m != 1) {
			J += leap ? 31 : 30;
			long int m2 = m - 2;
			if(m2 > 5) m2 = 5;
			J += m2 * 31;
			if(m >= 8) J += (m - 7) * 30;
		}
		J += d;
		J--;
	} else {
		return date_to_fixed(j, m, d, ct) + 1721425L;
	}
	return J;
}
bool cjdn_to_date(Number J, long int &y, long int &m, long int &d, CalendarSystem ct) {
	if(ct == CALENDAR_GREGORIAN) {
		Number x3, r3, x2, r2, x1, r1, j;
		J *= 4; J -= 6884477L;
		cal_div(J, 146097, x3, r3);
		r3 /= 4; r3.floor(); r3 *= 100; r3 += 99;
		cal_div(r3, 36525, x2, r2);
		r2 /= 100; r2.floor(); r2 *= 5; r2 += 2;
		cal_div(r2, 153, x1, r1);
		r1 /= 5; r1.floor(); r1++; d = r1.lintValue();
		Number c0(x1); c0 += 2; c0 /= 12; c0.floor();
		j = x3; j *= 100; j += x2; j += c0;
		bool overflow = false; y = j.lintValue(&overflow); if(overflow) return false;
		c0 *= -12; c0 += x1; c0 += 3; m = c0.lintValue();
		return true;
	} else if(ct == CALENDAR_MILANKOVIC) {
		Number x3, r3, x2, r2, x1, r1, c0, j;
		J -= 1721120L; J *= 9; J += 2;
		cal_div(J, 328718L, x3, r3);
		r3 /= 9; r3.floor(); r3 *= 100; r3 += 99;
		cal_div(r3, 36525, x2, r2);
		r2 /= 100; r2.floor(); r2 *= 5; r2 += 2;
		cal_div(r2, 153, x1, r1);
		c0 = x1; c0 += 2; c0 /= 12; c0.floor();
		j = x3; j *= 100; j += x2; j += c0;
		bool overflow = false; y = j.lintValue(&overflow); if(overflow) return false;
		c0 *= 12; x1 -= c0; x1 += 3; m = x1.lintValue();
		r1.mod(153); r1 /= 5; r1.floor(); r1++; d = r1.lintValue();
		return true;
	} else if(ct == CALENDAR_JULIAN) {
		Number y2, k2, k1, x2, x1, c0, j;
		y2 = J; y2 -= 1721118L;
		k2 = y2; k2 *= 4; k2 += 3;
		k1 = k2; k1.mod(1461); k1 /= 4; k1.floor(); k1 *= 5; k1 += 2;
		x1 = k1; x1 /= 153; x1.floor();
		c0 = x1; c0 += 2; c0 /= 12; c0.floor();
		j = k2; j /= 1461; j.floor(); j += c0;
		bool overflow = false; y = j.lintValue(&overflow); if(overflow) return false;
		c0 *= 12; x1 -= c0; x1 += 3; m = x1.lintValue();
		k1.mod(153); k1 /= 5; k1.floor(); k1++; d = k1.lintValue();
		return true;
	} else if(ct == CALENDAR_ISLAMIC) {
		Number k2, k1, j;
		k2 = J; k2 -= 1948440L; k2 *= 30; k2 += 15;
		k1 = k2; k1.mod(10631); k1 /= 30; k1.floor(); k1 *= 11; k1 += 5;
		j = k2; j /= 10631; j.floor(); j++;
		bool overflow = false; y = j.lintValue(&overflow); if(overflow) return false;
		k2 = k1; k2 /= 325; k2.floor(); k2++; m = k2.lintValue();
		k1.mod(153); k1 /= 11; k1.floor(); k1++; d = k1.lintValue();
		return true;
	} else if(ct == CALENDAR_EGYPTIAN) {
		Number y2, x2, y1, j;
		y2 = J; x2 = y2; x2 /= 365; x2.floor(); y1 = y2; y1.mod(365);
		j = x2; j++;
		bool overflow = false; y = j.lintValue(&overflow); if(overflow) return false;
		y2 = y1; y2 /= 30; y2.floor(); y2++; m = y2.lintValue();
		y1.mod(30); y1++; d = y1.lintValue();
		return true;
	} else if(ct == CALENDAR_INDIAN) {
		if(!cjdn_to_date(J, y, m, d, CALENDAR_GREGORIAN)) return false;
		bool leap = gregorian_leap_year(y);
		Number j(y); j -= 78;
		Number J0;
		J0 = date_to_cjdn(y, 1, 1, CALENDAR_GREGORIAN);
		Number yday(J); yday -= J0;
		if(yday.isLessThan(80)) {j--; yday += (leap ? 31 : 30) + (31 * 5) + (30 * 3) + 10 + 80;}
		yday -= 80;
		if(yday.isLessThan(leap ? 31 : 30)) {
			m = 1;
			yday++;
			d = yday.lintValue();
		} else {
			Number mday(yday); mday -= (leap ? 31 : 30);
			if(mday.isLessThan(31 * 5)) {
				m = quotient(mday, 31).lintValue() + 2;
				mday.rem(31);
				mday++;
				d = mday.lintValue();
			} else {
				mday -= 31 * 5;
				m = quotient(mday, 30).lintValue() + 7;
				mday.rem(30);
				mday++;
				d = mday.lintValue();
			}
		}
		bool overflow = false; y = j.lintValue(&overflow); if(overflow) return false;
		return true;
	} else {
		J -= 1721425L;
		return fixed_to_date(J, y, m, d, ct);
	}
	return false;
}

bool calendarToDate(QalculateDateTime &date, long int y, long int m, long int d, CalendarSystem ct) {
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	long int new_y, new_m, new_d;
	if(!fixed_to_date(date_to_fixed(y, m, d, ct), new_y, new_m, new_d, CALENDAR_GREGORIAN)) {
		CALCULATOR->endTemporaryStopIntervalArithmetic();
		return false;
	}
	date.set(new_y, new_m, new_d);
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	return true;
}
bool dateToCalendar(const QalculateDateTime &date, long int &y, long int &m, long int &d, CalendarSystem ct) {
	if(ct == CALENDAR_GREGORIAN) {
		y = date.year(); m = date.month(); d = date.day();
		return true;
	}
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	if(!fixed_to_date(date_to_fixed(date.year(), date.month(), date.day(), CALENDAR_GREGORIAN), y, m, d, ct)) {
		CALCULATOR->endTemporaryStopIntervalArithmetic();
		return false;
	}
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	return true;
}

int numberOfMonths(CalendarSystem ct) {
	if(ct == CALENDAR_CHINESE) return 24;
	if(ct == CALENDAR_HEBREW || ct == CALENDAR_COPTIC || ct == CALENDAR_EGYPTIAN || ct == CALENDAR_ETHIOPIAN) return 13;
	return 12;
}
string monthName(long int month, CalendarSystem ct, bool append_number, bool append_leap) {
	if(month < 1) return i2s(month);
	if(ct == CALENDAR_CHINESE) {
		if(month > 24) return i2s(month);
		bool leap = month > 12;
		if(leap) month -= 12;
		string str = i2s(month);
		if(leap && append_leap) {str += " ("; str += _("leap month"); str += ")";}
		return str;
	}
	if(month > 13) return i2s(month);
	string str;
	if(ct == CALENDAR_HEBREW) {
		str = HEBREW_MONTHS[month - 1];
	} else if(ct == CALENDAR_JULIAN || ct == CALENDAR_MILANKOVIC || ct == CALENDAR_GREGORIAN) {
		if(month == 13) return i2s(month);
		str = _(STANDARD_MONTHS[month - 1]);
	} else if(ct == CALENDAR_COPTIC) {
		str = _(COPTIC_MONTHS[month - 1]);
	} else if(ct == CALENDAR_ETHIOPIAN) {
		str = _(ETHIOPIAN_MONTHS[month - 1]);
	} else if(ct == CALENDAR_ISLAMIC) {
		if(month == 13) return i2s(month);
		str = _(ISLAMIC_MONTHS[month - 1]);
	} else if(ct == CALENDAR_PERSIAN) {
		if(month == 13) return i2s(month);
		str = _(PERSIAN_MONTHS[month - 1]);
	} else if(ct == CALENDAR_INDIAN) {
		if(month == 13) return i2s(month);
		str = _(INDIAN_MONTHS[month - 1]);
	} else {
		return i2s(month);
	}
	if(append_number) {str += " ("; str += i2s(month); str += ")";}
	return str;
}

void chineseYearInfo(long int year, long int &cycle, long int &year_in_cycle, long int &stem, long int &branch) {
	cycle = (year - 1) / 60 + 1;
	if(year <= 0) year += ((-year / 60) + 1) * 60;
	year_in_cycle = ((year - 1) % 60) + 1;
	stem = ((year_in_cycle - 1) % 10) + 1;
	branch = ((year_in_cycle - 1) % 12) + 1;
}
long int chineseCycleYearToYear(long int cycle, long int year_in_cycle) {
	return ((cycle - 1) * 60) + year_in_cycle;
}
int chineseStemBranchToCycleYear(long int stem, long int branch) {
	stem = (stem + 1) / 2;
	stem -= (branch - 1) / 2;
	if(stem < 1) stem += 5;
	int cy = (stem - 1) * 12 + branch;
	if(cy < 0 || cy > 60) return 0;
	return cy;
}
string chineseStemName(long int stem) {
	stem = (stem + 1) / 2;
	if(stem < 1 || stem > 5) return empty_string;
	return _(CHINESE_ELEMENTS[stem - 1]);
}
string chineseBranchName(long int branch) {
	if(branch < 1 || branch > 12) return empty_string;
	return _(CHINESE_ANIMALS[branch - 1]);
}

Number solarLongitude(const QalculateDateTime &date) {
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	Number fixed = date_to_fixed(date.year(), date.month(), date.day(), CALENDAR_GREGORIAN);
	Number time = date.second(); time /= 60; time += date.minute(); time -= dateTimeZone(date, false); time /= 60; time += date.hour(); time /= 24;
	fixed += time;
	Number longitude = solar_longitude(fixed);
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	longitude.setPrecision(8);
	return longitude;
}
QalculateDateTime findNextSolarLongitude(const QalculateDateTime &date, Number longitude) {
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	Number fixed = date_to_fixed(date.year(), date.month(), date.day(), CALENDAR_GREGORIAN);
	Number time = date.second(); time /= 60; time += date.minute(); time -= dateTimeZone(date, false); time /= 60; time += date.hour(); time /= 24;
	fixed += time;
	fixed = solar_longitude_after(longitude, fixed);
	long int y, m, d;
	fixed_to_date(fixed, y, m, d, CALENDAR_GREGORIAN);
	QalculateDateTime dt(y, m, d);
	Number fixed2 = date_to_fixed(y, m, d, CALENDAR_GREGORIAN);
	dt.addMinutes(dateTimeZone(dt, true));
	dt.addDays(fixed - fixed2);
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	return dt;
}
Number lunarPhase(const QalculateDateTime &date) {
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	Number fixed = date_to_fixed(date.year(), date.month(), date.day(), CALENDAR_GREGORIAN);
	Number time = date.second(); time /= 60; time += date.minute(); time -= dateTimeZone(date, false); time /= 60; time += date.hour(); time /= 24;
	fixed += time;
	Number phase = lunar_phase(fixed); phase /= 360;
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	phase.setPrecision(8);
	return phase;
}
QalculateDateTime findNextLunarPhase(const QalculateDateTime &date, Number phase) {
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	Number fixed = date_to_fixed(date.year(), date.month(), date.day(), CALENDAR_GREGORIAN);
	Number time = date.second(); time /= 60; time += date.minute(); time -= dateTimeZone(date, false); time /= 60; time += date.hour(); time /= 24;
	fixed += time;
	phase *= 360;
	fixed = lunar_phase_at_or_after(phase, fixed);
	long int y, m, d;
	fixed_to_date(fixed, y, m, d, CALENDAR_GREGORIAN);
	QalculateDateTime dt(y, m, d);
	Number fixed2 = date_to_fixed(y, m, d, CALENDAR_GREGORIAN);
	dt.addMinutes(dateTimeZone(dt, true));
	dt.addDays(fixed - fixed2);
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	return dt;
}

#ifdef _MSC_VER
#	define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64

struct timezone {
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval* tv, struct timezone* tz) {
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag = 0;
	if(nullptr != tv) {
		GetSystemTimeAsFileTime(&ft);
		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;
		tmpres /= 10;  /*convert into microseconds*/
		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}
	if(nullptr != tz) {
		if(!tzflag) {
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}
	return 0;
}
#endif
