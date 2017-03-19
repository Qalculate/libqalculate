/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna_k@fmgirl.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "util.h"
#include <stdarg.h>
#include "Number.h"

#include <glib.h>
#include <time.h>
#include <fstream>
#ifdef _WIN32
#	include <windows.h>
#	include <shlobj.h>
#	include <glib/gstdio.h>
#else
#	include <utime.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <pwd.h>
#endif


bool eqstr::operator()(const char *s1, const char *s2) const {
	return strcmp(s1, s2) == 0;
}

char buffer[20000];

string date2s(int year, int month, int day) {
	string str = i2s(year);
	str += "-";
	if(month < 10) {
		str += "0";
	}
	str += i2s(month);
	str += "-";
	if(day < 10) {
		str += "0";
	}
	str += i2s(day);
	return str;
}
bool s2date(string str, void *gtime) {
/*	if(strptime(str.c_str(), "%x", time) || strptime(str.c_str(), "%Ex", time) || strptime(str.c_str(), "%Y-%m-%d", time) || strptime(str.c_str(), "%m/%d/%Y", time) || strptime(str.c_str(), "%m/%d/%y", time)) {
		return true;
	}*/
	//char *date_format = nl_langinfo(D_FMT);
	if(equalsIgnoreCase(str, _("today")) || equalsIgnoreCase(str, "today") || equalsIgnoreCase(str, _("now")) || equalsIgnoreCase(str, "now")) {
		g_date_set_time_t((GDate*) gtime, time(NULL));
		return true;
	} else if(equalsIgnoreCase(str, _("tomorrow")) || equalsIgnoreCase(str, "tomorrow")) {
		g_date_set_time_t((GDate*) gtime, time(NULL));
		g_date_add_days((GDate*) gtime, 1);
		return true;
	} else if(equalsIgnoreCase(str, _("yesterday")) || equalsIgnoreCase(str, "yesterday")) {
		g_date_set_time_t((GDate*) gtime, time(NULL));
		g_date_subtract_days((GDate*) gtime, 1);
		return true;
	}
	g_date_set_parse((GDate*) gtime, str.c_str());
	return g_date_valid((GDate*) gtime);
}

void now(int &hour, int &min, int &sec) {
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	hour = lt->tm_hour;
	min = lt->tm_min;
	sec = lt->tm_sec;
}
void today(int &year, int &month, int &day) {
	GDate *gtime = g_date_new();
	g_date_set_time_t(gtime, time(NULL));
	year = g_date_get_year(gtime);
	month = g_date_get_month(gtime);
	day = g_date_get_day(gtime);
	g_date_free(gtime);
}
bool addDays(GDate *gtime, int days) {
	if(days < 0) g_date_subtract_days(gtime, (guint) -days);
	else if(days > 0) g_date_add_days(gtime, (guint) days);
	if(!g_date_valid(gtime)) return false;
	return true;
}
bool addDays(int &year, int &month, int &day, int days) {
	GDate *gtime = g_date_new_dmy((GDateDay) day, (GDateMonth) month, (GDateYear) year);
	if(!addDays(gtime, days)) {
		g_date_free(gtime); 
		return false;
	}
	year = (int) g_date_year(gtime);
	month = (int) g_date_month(gtime);
	day = (int) g_date_day(gtime);
	g_date_free(gtime);
	return true;
}
string addDays(string str, int days) {
	GDate *gtime = g_date_new();
	if(!s2date(str, gtime) || !addDays(gtime, days)) {
		g_date_free(gtime); 
		return empty_string;
	}
	int y = (int) g_date_year(gtime);
	int m = (int) g_date_month(gtime);
	int d = (int) g_date_day(gtime);
	g_date_free(gtime);
	return date2s(y, m, d);
}
bool addMonths(GDate *gtime, int months) {
	if(months < 0) g_date_subtract_months(gtime, (guint) -months);
	else if(months > 0) g_date_add_months(gtime, (guint) months);
	if(!g_date_valid(gtime)) return false;
	return true;
}
bool addMonths(int &year, int &month, int &day, int months) {
	GDate *gtime = g_date_new_dmy((GDateDay) day, (GDateMonth) month, (GDateYear) year);
	if(!addMonths(gtime, months)) {
		g_date_free(gtime); 
		return false;
	}
	year = (int) g_date_year(gtime);
	month = (int) g_date_month(gtime);
	day = (int) g_date_day(gtime);
	g_date_free(gtime);
	return true;
}
string addMonths(string str, int months) {
	GDate *gtime = g_date_new();
	if(!s2date(str, gtime) || !addMonths(gtime, months)) {
		g_date_free(gtime); 
		return empty_string;
	}
	int y = (int) g_date_year(gtime);
	int m = (int) g_date_month(gtime);
	int d = (int) g_date_day(gtime);
	g_date_free(gtime);
	return date2s(y, m, d);
}
bool addYears(GDate *gtime, int years) {
	if(years < 0) g_date_subtract_years(gtime, (guint) -years);
	else if(years > 0) g_date_add_years(gtime, (guint) years);
	if(!g_date_valid(gtime)) return false;
	return true;
}
bool addYears(int &year, int &month, int &day, int years) {
	GDate *gtime = g_date_new_dmy((GDateDay) day, (GDateMonth) month, (GDateYear) year);
	if(!addYears(gtime, years)) {
		g_date_free(gtime); 
		return false;
	}
	year = (int) g_date_year(gtime);
	month = (int) g_date_month(gtime);
	day = (int) g_date_day(gtime);
	g_date_free(gtime);
	return true;
}
string addYears(string str, int years) {
	GDate *gtime = g_date_new();
	if(!s2date(str, gtime) || !addYears(gtime, years)) {
		g_date_free(gtime); 
		return empty_string;
	}
	int y = (int) g_date_year(gtime);
	int m = (int) g_date_month(gtime);
	int d = (int) g_date_day(gtime);
	g_date_free(gtime);
	return date2s(y, m, d);
}

int week(string str, bool start_sunday) {
	remove_blank_ends(str);
	GDate *gtime = g_date_new();
	int week = -1;
	if(s2date(str, gtime)) {
		if(start_sunday) {
			week = g_date_get_sunday_week_of_year(gtime);
		} else {
			if(g_date_get_month(gtime) == G_DATE_DECEMBER && g_date_get_day(gtime) >= 29 && g_date_get_weekday(gtime) <= g_date_get_day(gtime) - 28) {
				week = 1;
			} else {
				calc_week_1:
				int day = g_date_get_day_of_year(gtime);
				g_date_set_day(gtime, 1);
				g_date_set_month(gtime, G_DATE_JANUARY);
				int wday = g_date_get_weekday(gtime);
				day -= (8 - wday);
				if(wday <= 4) {
					week = 1;
				} else {
					week = 0;
				}
				if(day > 0) {
					day--;
					week += day / 7 + 1;
				}
				if(week == 0) {
					int year = g_date_get_year(gtime);
					g_date_set_dmy(gtime, 31, G_DATE_DECEMBER, year - 1);
					goto calc_week_1;
				}
			}
		}
	}
	g_date_free(gtime);
	return week;
}
int weekday(string str) {
	remove_blank_ends(str);
	GDate *gtime = g_date_new();
	int day = -1;
	if(s2date(str, gtime)) {
		day = g_date_get_weekday(gtime);
	}
	g_date_free(gtime);
	return day;
}
int yearday(string str) {
	remove_blank_ends(str);
	GDate *gtime = g_date_new();
	int day = -1;
	if(s2date(str, gtime)) {
		day = g_date_get_day_of_year(gtime);
	}
	g_date_free(gtime);
	return day;
}

bool s2date(string str, int &year, int &month, int &day) {
	//struct tm time;
	remove_blank_ends(str);
	GDate *gtime = g_date_new();
	if(s2date(str, gtime)) {
/*		year = time.tm_year + 1900;
		month = time.tm_mon + 1;
		day = time.tm_mday;	*/
		year = g_date_get_year(gtime);
		month = g_date_get_month(gtime);
		day = g_date_get_day(gtime);
		g_date_free(gtime);
		return true;
	}
	g_date_free(gtime);
	return false;
}
bool isLeapYear(int year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
int daysPerYear(int year, int basis) {
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

int daysPerMonth(int month, int year) {
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

Number yearsBetweenDates(string date1, string date2, int basis, bool date_func) {
	if(basis < 0 || basis > 4) return -1;
	if(basis == 1) {
		int day1, day2, month1, month2, year1, year2;
		if(!s2date(date1, year1, month1, day1)) {
  			return -1;
		}
		if(!s2date(date2, year2, month2, day2)) {
  			return -1;
		}		
		if(year1 > year2 || (year1 == year2 && month1 > month2) || (year1 == year2 && month1 == month2 && day1 > day2)) {
			int year3 = year1, month3 = month1, day3 = day1;
			year1 = year2; month1 = month2; day1 = day2;
			year2 = year3; month2 = month3; day2 = day3;		
		}		
		int days = 0;
		if(year1 == year2) {
			days = daysBetweenDates(year1, month1, day1, year2, month2, day2, basis, date_func);
			if(days < 0) return -1;
			return Number(days, daysPerYear(year1, basis));
		}
		for(int month = 12; month > month1; month--) {
			days += daysPerMonth(month, year1);
		}
		days += daysPerMonth(month1, year1) - day1 + 1;
/*		Number *nr = new Number(days, daysPerYear(year1, basis));
		year1++;
		if(year1 != year2) {
			Number yfr(year2 - year1);
			nr->add(&yfr);
		}
		days = 0;*/
		for(int month = 1; month < month2; month++) {
			days += daysPerMonth(month, year2);
		}
		days += day2 - 1;
		int days_of_years = 0;
		for(int year = year1; year <= year2; year++) {
			days_of_years += daysPerYear(year, basis);
			if(year != year1 && year != year2) {
				days += daysPerYear(year, basis);
			}
		}
		Number year_frac(days_of_years, year2 + 1 - year1);
/*		if(days > 0) {
			Number nr2(days, daysPerYear(year2, basis));
			nr->add(&nr2);
		}*/
		Number nr(days);
		nr /= year_frac;
		return nr;
	} else {
		int days = daysBetweenDates(date1, date2, basis, date_func);
		if(days < 0) return -1;
		return Number(days, daysPerYear(0, basis));	
	}
	return -1;
}
int daysBetweenDates(string date1, string date2, int basis, bool date_func) {
	int day1, day2, month1, month2, year1, year2;
	if(!s2date(date1, year1, month1, day1)) {
  		return -1;
	}
	if(!s2date(date2, year2, month2, day2)) {
  		return -1;
	}
	return daysBetweenDates(year1, month1, day1, year2, month2, day2, basis, date_func);	
}
int daysBetweenDates(int year1, int month1, int day1, int year2, int month2, int day2, int basis, bool date_func) {
	if(basis < 0 || basis > 4) return -1;
	bool isleap = false;
	int days, months, years;

	if(year1 > year2 || (year1 == year2 && month1 > month2) || (year1 == year2 && month1 == month2 && day1 > day2)) {
		int year3 = year1, month3 = month1, day3 = day1;
		year1 = year2; month1 = month2; day1 = day2;
		year2 = year3; month2 = month3; day2 = day3;		
	}

	years = year2  - year1;
	months = month2 - month1 + years * 12;
	days = day2 - day1;

	isleap = isLeapYear(year1);

	switch(basis) {
		case 0: {
			if(date_func) {
				if(month1 == 2 && ((day1 == 28 && !isleap) || (day1 == 29 && isleap)) && !(month2 == month1 && day1 == day2 && year1 == year2)) {
					if(isleap) return months * 30 + days - 1;
					else return months * 30 + days - 2;
				}
				if(day1 == 31 && day2 < 31) days++;
			} else {
				if(month1 == 2 && month2 != 2 && year1 == year2) {
					if(isleap) return months * 30 + days - 1;
					else return months * 30 + days - 2;
				}			
			}
			return months * 30 + days;
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
			for(; month1 < month4 || b; month1++) {
				if(month1 > month4 && b) {
					b = false;
					month1 = 1;
					month4 = month2;
					if(month1 == month2) break;
				}
				if(!b) {
					days += daysPerMonth(month1, year2);
				} else {
					days += daysPerMonth(month1, year1);
				}
			}
			if(years == 0) return days;
			//if(basis == 1) {
				for(year1 += 1; year1 < year2; year1++) {
					if(isLeapYear(year1)) days += 366;
					else days += 365;
				} 
				return days;
			//}
			//if(basis == 2) return (years - 1) * 360 + days;		
			//if(basis == 3) return (years - 1) * 365 + days;
		} 
		case 4: {
			if(date_func) {
				if(day2 == 31 && day1 < 31) days--;
				if(day1 == 31 && day2 < 31) days++;
			}
			return months * 30 + days;
		}
	}
	return -1;
	
}

string& gsub(const string &pattern, const string &sub, string &str) {
	size_t i = str.find(pattern);
	while(i != string::npos) {
		str.replace(i, pattern.length(), sub);
		i = str.find(pattern, i + sub.length());
	}
	return str;
}
string& gsub(const char *pattern, const char *sub, string &str) {
	size_t i = str.find(pattern);
	while(i != string::npos) {
		str.replace(i, strlen(pattern), string(sub));
		i = str.find(pattern, i + strlen(sub));
	}
	return str;
}

string& remove_blanks(string &str) {
	size_t i = str.find_first_of(SPACES, 0);
	while(i != string::npos) {
		str.erase(i, 1);
		i = str.find_first_of(SPACES, i);
	}
	return str;
}

string& remove_duplicate_blanks(string &str) {
	size_t i = str.find_first_of(SPACES, 0);
	while(i != string::npos) {
		if(i == 0 || is_in(SPACES, str[i - 1])) {
			str.erase(i, 1);
		} else {
			i++;
		}
		i = str.find_first_of(SPACES, i);
	}
	return str;
}

string& remove_blank_ends(string &str) {
	size_t i = str.find_first_not_of(SPACES);
	size_t i2 = str.find_last_not_of(SPACES);
	if(i != string::npos && i2 != string::npos) {
		if(i > 0 || i2 < str.length() - 1) {
			str = str.substr(i, i2 - i + 1);
		}
	} else {
		str.resize(0);
	}
	return str;
}
string& remove_parenthesis(string &str) {
	if(str[0] == LEFT_PARENTHESIS_CH && str[str.length() - 1] == RIGHT_PARENTHESIS_CH) {
		str = str.substr(1, str.length() - 2);
		return remove_parenthesis(str);
	}
	return str;
}

string d2s(double value, int precision) {
	//	  qgcvt(value, precision, buffer);
	sprintf(buffer, "%.*G", precision, value);
	string stmp = buffer;
	//	  gsub("e", "E", stmp);
	return stmp;
}

string p2s(void *o) {
	sprintf(buffer, "%p", o);
	string stmp = buffer;
	return stmp;
}
string i2s(int value) {
	//	  char buffer[10];
	sprintf(buffer, "%i", value);
	string stmp = buffer;
	return stmp;
}
string i2s(long int value) {
	sprintf(buffer, "%li", value);
	string stmp = buffer;
	return stmp;
}
string i2s(unsigned int value) {
	sprintf(buffer, "%u", value);
	string stmp = buffer;
	return stmp;
}
string i2s(unsigned long int value) {
	sprintf(buffer, "%lu", value);
	string stmp = buffer;
	return stmp;
}
const char *b2yn(bool b, bool capital) {
	if(capital) {
		if(b) return _("Yes");
		return _("No");
	}
	if(b) return _("yes");
	return _("no");
}
const char *b2tf(bool b, bool capital) {
	if(capital) {
		if(b) return _("True");
		return _("False");
	}
	if(b) return _("true");
	return _("false");
}
const char *b2oo(bool b, bool capital) {
	if(capital) {
		if(b) return _("On");
		return _("Off");
	}
	if(b) return _("on");
	return _("off");
}
int s2i(const string& str) {
	return strtol(str.c_str(), NULL, 10);
}
int s2i(const char *str) {
	return strtol(str, NULL, 10);
}
void *s2p(const string& str) {
	void *p;
	sscanf(str.c_str(), "%p", &p);
	return p;
}
void *s2p(const char *str) {
	void *p;
	sscanf(str, "%p", &p);
	return p;
}

size_t find_ending_bracket(const string &str, size_t start, int *missing) {
	int i_l = 1;
	while(true) {
		start = str.find_first_of(LEFT_PARENTHESIS RIGHT_PARENTHESIS, start);
		if(start == string::npos) {
			if(missing) *missing = i_l;
			return string::npos;
		}
		if(str[start] == LEFT_PARENTHESIS_CH) {
			i_l++;
		} else {
			i_l--;
			if(!i_l) {
				if(missing) *missing = i_l;
				return start;
			}
		}
		start++;
	}
}

char op2ch(MathOperation op) {
	switch(op) {
		case OPERATION_ADD: return PLUS_CH;
		case OPERATION_SUBTRACT: return MINUS_CH;		
		case OPERATION_MULTIPLY: return MULTIPLICATION_CH;		
		case OPERATION_DIVIDE: return DIVISION_CH;		
		case OPERATION_RAISE: return POWER_CH;		
		case OPERATION_EXP10: return EXP_CH;
		default: return ' ';		
	}
}

string& wrap_p(string &str) {
	str.insert(str.begin(), 1, LEFT_PARENTHESIS_CH);
	str += RIGHT_PARENTHESIS_CH;
	return str;
}

bool is_in(const char *str, char c) {
	for(size_t i = 0; i < strlen(str); i++) {
		if(str[i] == c)
			return true;
	}
	return false;
}
bool is_not_in(const char *str, char c) {
	for(size_t i = 0; i < strlen(str); i++) {
		if(str[i] == c)
			return false;
	}
	return true;
}
bool is_in(const string &str, char c) {
	for(size_t i = 0; i < str.length(); i++) {
		if(str[i] == c)
			return true;
	}
	return false;
}
bool is_not_in(const string &str, char c) {
	for(size_t i = 0; i < str.length(); i++) {
		if(str[i] == c)
			return false;
	}
	return true;
}

int sign_place(string *str, size_t start) {
	size_t i = str->find_first_of(OPERATORS, start);
	if(i != string::npos)
		return i;
	else
		return -1;
}

int gcd(int i1, int i2) {
	if(i1 < 0) i1 = -i1;
	if(i2 < 0) i2 = -i2;
	if(i1 == i2) return i2;
	int i3;
	if(i2 > i1) {
		i3 = i2;
		i2 = i1;
		i1 = i3;
	}
	while((i3 = i1 % i2) != 0) {
		i1 = i2;
		i2 = i3;
	}
	return i2;
}

size_t unicode_length(const string &str) {
	size_t l = str.length(), l2 = 0;
	for(size_t i = 0; i < l; i++) {
		if(str[i] > 0 || (unsigned char) str[i] >= 0xC2) {
			l2++;
		}
	}
	return l2;
}
size_t unicode_length(const char *str) {
	size_t l = strlen(str), l2 = 0;
	for(size_t i = 0; i < l; i++) {
		if(str[i] > 0 || (unsigned char) str[i] >= 0xC2) {
			l2++;
		}
	}
	return l2;
}

bool text_length_is_one(const string &str) {
	if(str.empty()) return false;
	if(str.length() == 1) return true;
	if(str[0] >= 0) return false;
	for(size_t i = 1; i < str.length(); i++) {
		if(str[i] > 0 || (unsigned char) str[i] >= 0xC2) {
			return false;
		}
	}
	return true;
}

bool equalsIgnoreCase(const string &str1, const string &str2) {
	if(str1.length() != str2.length()) return false;
	for(size_t i = 0; i < str1.length(); i++) {
		if(str1[i] < 0 && i + 1 < str1.length()) {
			if(str2[i] >= 0) return false;
			int i2 = 1;
			while(i2 + i < str1.length() && str1[i2 + i] < 0) {
				if(str2[i2 + i] >= 0) return false;
				i2++;
			}
			gchar *gstr1 = g_utf8_strdown(str1.c_str() + (sizeof(char) * i), i2);
			gchar *gstr2 = g_utf8_strdown(str2.c_str() + (sizeof(char) * i), i2);
			if(strcmp(gstr1, gstr2) != 0) return false;
			g_free(gstr1);
			g_free(gstr2);
			i += i2 - 1;
		} else if(str1[i] != str2[i] && !((str1[i] >= 'a' && str1[i] <= 'z') && str1[i] - 32 == str2[i]) && !((str1[i] <= 'Z' && str1[i] >= 'A') && str1[i] + 32 == str2[i])) {
			return false;
		}
	}
	return true;
}

bool equalsIgnoreCase(const string &str1, const char *str2) {
	if(str1.length() != strlen(str2)) return false;
	for(size_t i = 0; i < str1.length(); i++) {
		if(str1[i] < 0 && i + 1 < str1.length()) {
			if(str2[i] >= 0) return false;
			int i2 = 1;
			while(i2 + i < str1.length() && str1[i2 + i] < 0) {
				if(str2[i2 + i] >= 0) return false;
				i2++;
			}
			gchar *gstr1 = g_utf8_strdown(str1.c_str() + (sizeof(char) * i), i2);
			gchar *gstr2 = g_utf8_strdown(str2 + (sizeof(char) * i), i2);
			if(strcmp(gstr1, gstr2) != 0) return false;
			g_free(gstr1);
			g_free(gstr2);
			i += i2 - 1;
		} else if(str1[i] != str2[i] && !((str1[i] >= 'a' && str1[i] <= 'z') && str1[i] - 32 == str2[i]) && !((str1[i] <= 'Z' && str1[i] >= 'A') && str1[i] + 32 == str2[i])) {
			return false;
		}
	}
	return true;
}

void parse_qalculate_version(string qalculate_version, int *qalculate_version_numbers) {
	for(size_t i = 0; i < 3; i++) {
		size_t dot_i = qalculate_version.find(".");
		if(dot_i == string::npos) {
			qalculate_version_numbers[i] = s2i(qalculate_version);
			break;
		}
		qalculate_version_numbers[i] = s2i(qalculate_version.substr(0, dot_i));
		qalculate_version = qalculate_version.substr(dot_i + 1, qalculate_version.length() - (dot_i + 1));
	}
}

string getOldLocalDir() {
	gchar *gstr = g_build_filename(g_get_home_dir(), ".qalculate", NULL);
	string homedir = gstr;
	g_free(gstr);
	return homedir;
}
string getLocalDir() {
	gchar *gstr = g_build_filename(g_get_user_config_dir(), "qalculate", NULL);
	string confdir = gstr;
	g_free(gstr);
	return confdir;
}
string getLocalDataDir() {
	gchar *gstr = g_build_filename(g_get_user_data_dir(), "qalculate", NULL);
	string datadir = gstr;
	g_free(gstr);
	return datadir;
}
string getLocalTmpDir() {
	gchar *gstr = g_build_filename(g_get_tmp_dir(), "qalculate", NULL);
	string tmpdir = gstr;
	g_free(gstr);
	return tmpdir;
}

bool move_file(const char *from_file, const char *to_file) {
#ifdef _WIN32
	return g_rename(from_file, to_file) == 0;
#else
	ifstream source(from_file);
	if(source.fail()) {
		source.close();
		return false;
	}

	ofstream dest(to_file);
	if(dest.fail()) {
		source.close();
		dest.close();
		return false;
	}

	dest << source.rdbuf();

	source.close();
	dest.close();
	
	struct stat stats_from;
	if(stat(from_file, &stats_from) == 0) {
		struct utimbuf to_times;
		to_times.actime = stats_from.st_atime;
		to_times.modtime = stats_from.st_mtime;
		utime(to_file, &to_times);
	}
	
	remove(from_file);
	
	return true;
#endif
}

string getPackageDataDir() {
#ifdef USE_ABSOLUTE_PACKAGE_PATHS
	string datadir(PACKAGE_DATA_DIR);
	return datadir;
#else
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string datadir(exepath);
	datadir.resize(datadir.find_last_of('\\'));
	if (datadir.substr(datadir.length() - 3) == "bin") {
		datadir.resize(datadir.find_last_of('\\'));
	}
	datadir += "\\share";
	return datadir;
#endif
}

string getPackageLocaleDir() {
#ifdef USE_ABSOLUTE_PACKAGE_PATHS
	string localedir(PACKAGE_LOCALE_DIR);
	return localedir;
#else
	gchar *dir = g_build_filename(getPackageDataDir().c_str(), "locale", NULL);
	string localeDir(dir);
	g_free(dir);
	return localeDir;
#endif
}


#ifdef _WIN32

Thread::Thread() :
	running(false),
	m_thread(NULL),
	m_threadReadyEvent(NULL),
	m_threadID(0)
{
	m_threadReadyEvent = CreateEvent(NULL, false, false, NULL);
}

Thread::~Thread() {
	CloseHandle(m_threadReadyEvent);
}

DWORD WINAPI Thread::doRun(void *data) {
	// create thread message queue
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	Thread *thread = (Thread *) data;
	SetEvent(thread->m_threadReadyEvent);
	thread->run();
	return 0;
}

bool Thread::start() {
	m_thread = CreateThread(NULL, 0, Thread::doRun, this, 0, &m_threadID);
	if (m_thread == NULL) return false;
	WaitForSingleObject(m_threadReadyEvent, INFINITE);
	running = (m_thread != NULL);
	return running;
}

bool Thread::cancel() {
	// FIXME: this is dangerous
	int ret = TerminateThread(m_thread, 0);
	if (ret == 0) return false;
	CloseHandle(m_thread);
	m_thread = NULL;
	m_threadID = 0;
	running = false;
	return true;
}

#else

Thread::Thread() :
	running(false),
	m_pipe_r(NULL),
	m_pipe_w(NULL)
{
	pthread_attr_init(&m_thread_attr);
	int pipe_wr[] = {0, 0};
	pipe(pipe_wr);
	m_pipe_r = fdopen(pipe_wr[0], "r");
	m_pipe_w = fdopen(pipe_wr[1], "w");
}

Thread::~Thread() {
	fclose(m_pipe_r);
	fclose(m_pipe_w);
	pthread_attr_destroy(&m_thread_attr);
}

void Thread::doCleanup(void *data) {
	Thread *thread = (Thread *) data;
	thread->running = false;
}

void *Thread::doRun(void *data) {
	pthread_cleanup_push(&Thread::doCleanup, data);

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	Thread *thread = (Thread *) data;
	thread->run();

	pthread_cleanup_pop(1);
	return NULL;
}

bool Thread::start() {
	int ret = pthread_create(&m_thread, &m_thread_attr, &Thread::doRun, this);
	running = (ret == 0);
	return running;
}

bool Thread::cancel() {
	int ret = pthread_cancel(m_thread);
	running = (ret != 0);
	return !running;
}


#endif
