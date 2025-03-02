/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "util.h"
#include <stdarg.h>
#include "Number.h"

#include <string.h>
#include <time.h>
#ifdef HAVE_ICONV
#	include <iconv.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#ifdef HAVE_LIBCURL
#	include <curl/curl.h>
#endif
#ifdef HAVE_ICU
#	include <unicode/ucasemap.h>
#endif
#include <fstream>
#ifdef _WIN32
#	include <winsock2.h>
#	include <windows.h>
#	include <shlobj.h>
#	include <direct.h>
#	include <knownfolders.h>
#	include <initguid.h>
#	include <shlobj.h>
#else
#	ifdef HAVE_PIPE2
#		include <fcntl.h>
#	endif
#	include <utime.h>
#	include <unistd.h>
#	include <pwd.h>
#endif

using std::string;
using std::endl;
using std::cout;
using std::vector;
using std::ifstream;
using std::ofstream;

bool eqstr::operator()(const char *s1, const char *s2) const {
	return strcmp(s1, s2) == 0;
}

#ifdef HAVE_ICU
	UCaseMap *ucm = NULL;
#endif

void sleep_ms(int milliseconds) {
#ifdef _WIN32
	Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	usleep(milliseconds * 1000);
#endif
}

void now(int &hour, int &min, int &sec) {
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	hour = lt->tm_hour;
	min = lt->tm_min;
	sec = lt->tm_sec;
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
		if(i != 0 && is_in(SPACES, str[i - 1])) {
			str.erase(i, 1);
		} else {
			str[i] = ' ';
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
	// qgcvt(value, precision, buffer);
	char *buffer = (char*) malloc((precision + 21) * sizeof(char));
	snprintf(buffer, precision + 21, "%.*G", precision, value);
	string stmp = buffer;
	free(buffer);
	// gsub("e", "E", stmp);
	return stmp;
}

string p2s(void *o) {
	char buffer[21];
	snprintf(buffer, 21, "%p", o);
	string stmp = buffer;
	return stmp;
}
string i2s(long int value) {
	char buffer[21];
	snprintf(buffer, 21, "%li", value);
	string stmp = buffer;
	return stmp;
}
string u2s(unsigned long int value) {
	char buffer[21];
	snprintf(buffer, 21, "%lu", value);
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
long int s2i(const string& str) {
	if(str.find(' ') != string::npos) {
		string str2 = str;
		remove_blanks(str2);
		return strtol(str2.c_str(), NULL, 10);
	}
	return strtol(str.c_str(), NULL, 10);
}
long int s2i(const char *str) {
	for(size_t i = 0; i < strlen(str); i++) {
		if(str[i] == ' ') {
			string str2 = str;
			remove_blanks(str2);
			return strtol(str2.c_str(), NULL, 10);
		}
	}
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
		if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			l2++;
		}
	}
	return l2;
}
size_t unicode_length(const string &str, size_t l) {
	size_t l2 = 0;
	for(size_t i = 0; i < l; i++) {
		if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			l2++;
		}
	}
	return l2;
}
size_t unicode_length(const char *str) {
	size_t l = strlen(str), l2 = 0;
	for(size_t i = 0; i < l; i++) {
		if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			l2++;
		}
	}
	return l2;
}
size_t unicode_length(const char *str, size_t l) {
	size_t l2 = 0;
	for(size_t i = 0; i < l; i++) {
		if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			l2++;
		}
	}
	return l2;
}
bool text_length_is_one(const string &str) {
	if(str.empty()) return false;
	if(str.length() == 1) return true;
	if((signed char) str[0] >= 0) return false;
	for(size_t i = 1; i < str.length(); i++) {
		if((signed char) str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			return false;
		}
	}
	return true;
}

bool equalsIgnoreCase(const string &str1, const string &str2) {
	if(str1.empty() || str2.empty()) return str1.empty() && str2.empty();
	for(size_t i1 = 0, i2 = 0; i1 < str1.length() || i2 < str2.length(); i1++, i2++) {
		if(i1 >= str1.length() || i2 >= str2.length()) return false;
		if(((signed char) str1[i1] < 0 && i1 + 1 < str1.length()) || ((signed char) str2[i2] < 0 && i2 + 1 < str2.length())) {
			size_t iu1 = 1, iu2 = 1;
			if((signed char) str1[i1] < 0) {
				while(iu1 + i1 < str1.length() && (signed char) str1[i1 + iu1] < 0) {
					iu1++;
				}
			}
			if((signed char) str2[i2] < 0) {
				while(iu2 + i2 < str2.length() && (signed char) str2[i2 + iu2] < 0) {
					iu2++;
				}
			}
			bool isequal = (iu1 == iu2);
			if(isequal) {
				for(size_t i = 0; i < iu1; i++) {
					if(str1[i1 + i] != str2[i2 + i]) {
						isequal = false;
						break;
					}
				}
			}
			if(!isequal) {
				char *gstr1 = utf8_strdown(str1.c_str() + (sizeof(char) * i1));
				if(!gstr1) return false;
				char *gstr2 = utf8_strdown(str2.c_str() + (sizeof(char) * i2));
				if(!gstr2) {
					free(gstr1);
					return false;
				}
				bool b = strcmp(gstr1, gstr2) == 0;
				free(gstr1);
				free(gstr2);
				return b;
			}
			i1 += iu1 - 1;
			i2 += iu2 - 1;
		} else if(str1[i1] != str2[i2] && !((str1[i1] >= 'a' && str1[i1] <= 'z') && str1[i1] - 32 == str2[i2]) && !((str1[i1] <= 'Z' && str1[i1] >= 'A') && str1[i1] + 32 == str2[i2])) {
			return false;
		}
	}
	return true;
}
bool equalsIgnoreCase(const string &str1, const char *str2) {
	if(str1.empty() || strlen(str2) == 0) return str1.empty() && strlen(str2) == 0;
	for(size_t i1 = 0, i2 = 0; i1 < str1.length() || i2 < strlen(str2); i1++, i2++) {
		if(i1 >= str1.length() || i2 >= strlen(str2)) return false;
		if(((signed char) str1[i1] < 0 && i1 + 1 < str1.length()) || ((signed char) str2[i2] < 0 && i2 + 1 < strlen(str2))) {
			size_t iu1 = 1, iu2 = 1;
			if((signed char) str1[i1] < 0) {
				while(iu1 + i1 < str1.length() && (signed char) str1[i1 + iu1] < 0) {
					iu1++;
				}
			}
			if((signed char) str2[i2] < 0) {
				while(iu2 + i2 < strlen(str2) && (signed char) str2[i2 + iu2] < 0) {
					iu2++;
				}
			}
			bool isequal = (iu1 == iu2);
			if(isequal) {
				for(size_t i = 0; i < iu1; i++) {
					if(str1[i1 + i] != str2[i2 + i]) {
						isequal = false;
						break;
					}
				}
			}
			if(!isequal) {
				char *gstr1 = utf8_strdown(str1.c_str() + (sizeof(char) * i1));
				if(!gstr1) return false;
				char *gstr2 = utf8_strdown(str2 + (sizeof(char) * i2));
				if(!gstr2) {
					free(gstr1);
					return false;
				}
				bool b = strcmp(gstr1, gstr2) == 0;
				free(gstr1);
				free(gstr2);
				return b;
			}
			i1 += iu1 - 1;
			i2 += iu2 - 1;
		} else if(str1[i1] != str2[i2] && !((str1[i1] >= 'a' && str1[i1] <= 'z') && str1[i1] - 32 == str2[i2]) && !((str1[i1] <= 'Z' && str1[i1] >= 'A') && str1[i1] + 32 == str2[i2])) {
			return false;
		}
	}
	return true;
}

string sub_suffix_html(const string &name) {
	size_t i = name.rfind('_');
	bool b = (i == string::npos || i == name.length() - 1 || i == 0);
	size_t i2 = 1;
	string str;
	if(b) {
		if(is_in(NUMBERS, name[name.length() - 1])) {
			while(name.length() > i2 + 1 && is_in(NUMBERS, name[name.length() - 1 - i2])) {
				i2++;
			}
		} else {
			while((signed char) name[name.length() - i2] < 0 && (unsigned char) name[name.length() - i2] < 0xC0 && i2 < name.length()) {
				i2++;
			}
		}
		str += name.substr(0, name.length() - i2);
	} else {
		str += name.substr(0, i);
	}
	str += "<sub>";
	if(b) {
		str += name.substr(name.length() - i2, i2);
	} else {
		str += name.substr(i + 1, name.length() - (i + 1));
	}
	str += "</sub>";
	return str;
}
string sub_suffix(const string &name, const string &tag_begin, const string &tag_end) {
	size_t i = name.rfind('_');
	bool b = (i == string::npos || i == name.length() - 1 || i == 0);
	size_t i2 = 1;
	string str;
	if(b) {
		if(is_in(NUMBERS, name[name.length() - 1])) {
			while(name.length() > i2 + 1 && is_in(NUMBERS, name[name.length() - 1 - i2])) {
				i2++;
			}
		} else {
			while((signed char) name[name.length() - i2] < 0 && (unsigned char) name[name.length() - i2] < 0xC0 && i2 < name.length()) {
				i2++;
			}
		}
		str += name.substr(0, name.length() - i2);
	} else {
		str += name.substr(0, i);
	}
	str += tag_begin;
	if(b) str += name.substr(name.length() - i2, i2);
	else str += name.substr(i + 1, name.length() - (i + 1));
	str += tag_end;
	return str;
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

#ifdef _WIN32
string utf8_encode(const std::wstring &wstr) {
	if(wstr.empty()) return string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int) wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int) wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}
#endif

#ifndef _WIN32
string getHomeDir() {
	const char *homedir;
	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}
	return homedir;
}
#endif

string getOldLocalDir() {
#ifdef _WIN32
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
	string str = path;
	return str + "\\Qalculate";
#else
	return getHomeDir() + "/.qalculate";
#endif
}
string getLocalDir() {
	const char *homedir;
	if((homedir = getenv("QALCULATE_USER_DIR")) != NULL) {
		return homedir;
	}
#ifdef _WIN32
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string str(exepath);
	str.resize(str.find_last_of('\\'));
	str += "\\user";
#	ifdef WIN_PORTABLE
	_mkdir(str.c_str());
	return str;
#	else
	if(dirExists(str)) return str;
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path);
	str = path;
	return str + "\\Qalculate";
#	endif
#else
	if((homedir = getenv("XDG_CONFIG_HOME")) == NULL) {
		return getHomeDir() + "/.config/qalculate";
	}
	return string(homedir) + "/qalculate";
#endif
}
string getLocalDataDir() {
	const char *homedir;
	if((homedir = getenv("QALCULATE_USER_DIR")) != NULL) {
		return homedir;
	}
#ifdef _WIN32
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string str(exepath);
	str.resize(str.find_last_of('\\'));
	str += "\\user";
#	ifdef WIN_PORTABLE
	_mkdir(str.c_str());
	return str;
#	else
	if(dirExists(str)) return str;
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path);
	str = path;
	return str + "\\Qalculate";
#	endif
#else
	if((homedir = getenv("XDG_DATA_HOME")) == NULL) {
		return getHomeDir() + "/.local/share/qalculate";
	}
	return string(homedir) + "/qalculate";
#endif
}
string getLocalTmpDir() {
	const char *homedir;
	if((homedir = getenv("QALCULATE_USER_DIR")) != NULL) {
		return homedir;
	}
#ifdef _WIN32
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string str(exepath);
	str.resize(str.find_last_of('\\'));
#	ifdef WIN_PORTABLE
	str += "\\tmp";
	_mkdir(str.c_str());
	return str;
#	else
	str += "\\user";
	if(dirExists(str)) return str;
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path);
	str = path;
	str += "\\cache";
	_mkdir(str.c_str());
	return str + "\\Qalculate";
#	endif
#else
	if((homedir = getenv("XDG_CACHE_HOME")) == NULL) {
		return getHomeDir() + "/.cache/qalculate";
	}
	return string(homedir) + "/qalculate";
#endif
}

bool move_file(const char *from_file, const char *to_file) {
#ifdef _WIN32
	return MoveFile(from_file, to_file) != 0;
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
#ifndef WIN32
	return PACKAGE_DATA_DIR;
#else
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string datadir(exepath);
	datadir.resize(datadir.find_last_of('\\'));
	if (datadir.substr(datadir.length() - 4) == "\\bin") {
		datadir.resize(datadir.find_last_of('\\'));
		datadir += "\\share";
	} else if(datadir.substr(datadir.length() - 6) == "\\.libs") {
		datadir.resize(datadir.find_last_of('\\'));
		datadir.resize(datadir.find_last_of('\\'));
	}
	return datadir;
#endif
}

string getGlobalDefinitionsDir() {
#ifdef COMPILED_DEFINITIONS
	return "";
#else
#	ifndef WIN32
	char buffer[500];
	if(getcwd(buffer, 500)) {
		string dir = buffer;
		if(dirExists(buildPath(dir, "libqalculate")) && fileExists(buildPath(dir, "data", "functions.xml"))) return buildPath(dir, "data");
		size_t i = dir.rfind("/");
		if(i != string::npos && i > 0 && i < dir.length() - 1) dir = dir.substr(0, i);
		if(dirExists(buildPath(dir, "libqalculate")) && fileExists(buildPath(dir, "data", "functions.xml"))) return buildPath(dir, "data");
	}
	return string(PACKAGE_DATA_DIR) + "/qalculate";
#	else
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string datadir(exepath);
	bool is_qalc = datadir.substr(datadir.length() - 8) == "qalc.exe";
	datadir.resize(datadir.find_last_of('\\'));
	if(datadir.substr(datadir.length() - 4) == "\\bin") {
		datadir.resize(datadir.find_last_of('\\'));
		datadir += "\\share\\qalculate";
		return datadir;
	} else if(datadir.substr(datadir.length() - 6) == "\\.libs") {
		datadir.resize(datadir.find_last_of('\\'));
		datadir.resize(datadir.find_last_of('\\'));
		if(!is_qalc) {
			datadir.resize(datadir.find_last_of('\\'));
			datadir += "\\libqalculate";
			if(!dirExists(datadir)) {
				datadir += "-";
				datadir += VERSION;
			}
		}
		return datadir + "\\data";
	}
	return datadir + "\\definitions";
#	endif
#endif
}

string getPackageLocaleDir() {
#ifndef WIN32
	return PACKAGE_LOCALE_DIR;
#else
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string datadir(exepath);
	datadir.resize(datadir.find_last_of('\\'));
	if(datadir.substr(datadir.length() - 4) == "\\bin" || datadir.substr(datadir.length() - 6) == "\\.libs") {
		datadir.resize(datadir.find_last_of('\\'));
		return datadir + "\\share\\locale";
	}
	return datadir + "\\locale";
#endif
}

string buildPath(string dir, string filename) {
#ifdef WIN32
	return dir + '\\' + filename;
#else
	return dir + '/' + filename;
#endif
}
string buildPath(string dir1, string dir2, string filename) {
#ifdef WIN32
	return dir1 + '\\' + dir2 + '\\' + filename;
#else
	return dir1 + '/' + dir2 + '/' + filename;
#endif
}
string buildPath(string dir1, string dir2, string dir3, string filename) {
#ifdef WIN32
	return dir1 + '\\' + dir2 + '\\' + dir3 + '\\' + filename;
#else
	return dir1 + '/' + dir2 + '/' + dir3 + '/' + filename;
#endif
}

bool dirExists(string dirpath) {
	return fileExists(dirpath);
}
bool fileExists(string filepath) {
#ifdef WIN32
	struct _stat info;
	return _stat(filepath.c_str(), &info) == 0;
#else
	struct stat info;
	return stat(filepath.c_str(), &info) == 0;
#endif
}
bool makeDir(string dirpath) {
#ifdef WIN32
	return _mkdir(dirpath.c_str()) == 0;
#else
	return mkdir(dirpath.c_str(), S_IRWXU) == 0;
#endif
}

bool recursiveMakeDir(string dirpath) {
#ifdef WIN32
	return _mkdir(dirpath.c_str()) == 0;
#else
	char tmp[256];
	char *p = NULL;
	size_t len;
	snprintf(tmp, sizeof(tmp), "%s", dirpath.c_str());
	len = strlen(tmp);
	if(tmp[len - 1] == '/') tmp[len - 1] = 0;
	for(p = tmp + 1; *p; p++) {
		if(*p == '/') {
			*p = 0;
			if(!dirExists(tmp)) mkdir(tmp, S_IRWXU);
			*p = '/';
		}
	}
	return mkdir(tmp, S_IRWXU) == 0;
#endif
}

bool removeDir(string dirpath) {
#ifdef WIN32
	return _rmdir(dirpath.c_str()) == 0;
#else
	return rmdir(dirpath.c_str()) == 0;
#endif
}

#ifdef _MSC_VER
#	define ICONV_CONST const
#endif
char *locale_from_utf8(const char *str) {
#ifdef HAVE_ICONV
	iconv_t conv = iconv_open("", "UTF-8");
	if(conv == (iconv_t) -1) return NULL;
	size_t inlength = strlen(str);
	size_t outlength = inlength * 4;
	char *dest, *buffer;
	buffer = dest = (char*) malloc((outlength + 4) * sizeof(char));
	if(!buffer) return NULL;
	size_t err = iconv(conv, (ICONV_CONST char **) &str, &inlength, &buffer, &outlength);
	if(err != (size_t) -1) err = iconv(conv, NULL, &inlength, &buffer, &outlength);
	iconv_close(conv);
	memset(buffer, 0, 4);
	if(err == (size_t) -1) {free(dest); return NULL;}
	return dest;
#else
	return NULL;
#endif
}
char *locale_to_utf8(const char *str) {
#ifdef HAVE_ICONV
	iconv_t conv = iconv_open("UTF-8", "");
	if(conv == (iconv_t) -1) return NULL;
	size_t inlength = strlen(str);
	size_t outlength = inlength * 4;
	char *dest, *buffer;
	buffer = dest = (char*) malloc((outlength + 4) * sizeof(char));
	if(!buffer) return NULL;
	size_t err = iconv(conv, (ICONV_CONST char**) &str, &inlength, &buffer, &outlength);
	if(err != (size_t) -1) err = iconv(conv, NULL, &inlength, &buffer, &outlength);
	iconv_close(conv);
	memset(buffer, 0, 4 * sizeof(char));
	if(err == (size_t) -1) {free(dest); return NULL;}
	return dest;
#else
	return NULL;
#endif
}
char *utf8_strdown(const char *str, int l) {
#ifdef HAVE_ICU
	if(!ucm) return NULL;
	UErrorCode err = U_ZERO_ERROR;
	size_t inlength = l <= 0 ? strlen(str) : (size_t) l;
	size_t outlength = inlength + 4;
	char *buffer = (char*) malloc(outlength * sizeof(char));
	if(!buffer) return NULL;
	int32_t length = ucasemap_utf8ToLower(ucm, buffer, outlength, str, inlength, &err);
	if(U_SUCCESS(err)) {
		//basic accent removal
		if(strcmp(buffer, "á") == 0 || strcmp(buffer, "à") == 0) {buffer[0] = 'a'; buffer[1] = '\0';}
		else if(strcmp(buffer, "é") == 0 || strcmp(buffer, "è") == 0) {buffer[0] = 'e'; buffer[1] = '\0';}
		else if(strcmp(buffer, "í") == 0 || strcmp(buffer, "ì") == 0) {buffer[0] = 'i'; buffer[1] = '\0';}
		else if(strcmp(buffer, "ú") == 0 || strcmp(buffer, "ù") == 0) {buffer[0] = 'u'; buffer[1] = '\0';}
		else if(strcmp(buffer, "ó") == 0 || strcmp(buffer, "ò") == 0 || strcmp(buffer, "õ") == 0) {buffer[0] = 'o'; buffer[1] = '\0';}
		else if(strcmp(buffer, "ñ") == 0) {buffer[0] = 'n'; buffer[1] = '\0';}
		return buffer;
	} else if(err == U_BUFFER_OVERFLOW_ERROR) {
		outlength = length + 4;
		char *buffer_realloc = (char*) realloc(buffer, outlength * sizeof(char));
		if(buffer_realloc) buffer = buffer_realloc;
		else {free(buffer); return NULL;}
		err = U_ZERO_ERROR;
		ucasemap_utf8ToLower(ucm, buffer, outlength, str, inlength, &err);
		if(U_SUCCESS(err)) {
			return buffer;
		}
	}
	free(buffer);
	return NULL;
#else
	return NULL;
#endif
}

char *utf8_strup(const char *str, int l) {
#ifdef HAVE_ICU
	if(!ucm) return NULL;
	UErrorCode err = U_ZERO_ERROR;
	size_t inlength = l <= 0 ? strlen(str) : (size_t) l;
	size_t outlength = inlength + 4;
	char *buffer = (char*) malloc(outlength * sizeof(char));
	if(!buffer) return NULL;
	int32_t length = ucasemap_utf8ToUpper(ucm, buffer, outlength, str, inlength, &err);
	if(U_SUCCESS(err)) {
		if(inlength > 1 && ((size_t) length != inlength || (buffer[0] != str[0] && buffer[1] != str[1]))) {
			err = U_ZERO_ERROR;
			char *buffer2 = (char*) malloc((inlength + 1) * sizeof(char));
			if(buffer2) {
				length = ucasemap_utf8ToLower(ucm, buffer2, inlength + 1, buffer, inlength, &err);
				if(!U_SUCCESS(err) || (size_t) length != inlength || strncmp(str, buffer2, inlength) != 0) {
					free(buffer2);
					buffer2 = NULL;
				}
			}
			if(!buffer2) {
				free(buffer);
				return NULL;
			}
		}
		return buffer;
	} else if(err == U_BUFFER_OVERFLOW_ERROR) {
		outlength = length + 4;
		char *buffer_realloc = (char*) realloc(buffer, outlength * sizeof(char));
		if(buffer_realloc) buffer = buffer_realloc;
		else {free(buffer); return NULL;}
		err = U_ZERO_ERROR;
		ucasemap_utf8ToUpper(ucm, buffer, outlength, str, inlength, &err);
		if(U_SUCCESS(err)) {
			err = U_ZERO_ERROR;
			char *buffer2 = (char*) malloc((inlength + 1) * sizeof(char));
			if(buffer2) {
				length = ucasemap_utf8ToLower(ucm, buffer2, inlength + 1, buffer, inlength, &err);
				if(!U_SUCCESS(err) || (size_t) length != inlength || strncmp(str, buffer2, inlength) != 0) {
					free(buffer2);
					buffer2 = NULL;
				}
			}
			if(!buffer2) {
				free(buffer);
				return NULL;
			}
			return buffer;
		}
	}
	free(buffer);
	return NULL;
#else
	return NULL;
#endif
}

extern size_t write_data(void *ptr, size_t size, size_t nmemb, string *sbuffer);
int checkAvailableVersion(const char *version_id, const char *current_version, string *available_version, int timeout) {
	return checkAvailableVersion(version_id, current_version, available_version, NULL, timeout);
}
int checkAvailableVersion(const char *version_id, const char *current_version, string *available_version, string *url, int timeout) {
#ifdef HAVE_LIBCURL
	string sbuffer;
	char error_buffer[CURL_ERROR_SIZE];
	CURL *curl;
	CURLcode res;
	long int file_time;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(!curl) {return -1;}
	curl_easy_setopt(curl, CURLOPT_URL, "https://qalculate.github.io/CURRENT_VERSIONS");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sbuffer);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
	curl_easy_setopt(curl, CURLOPT_FILETIME, &file_time);
#ifdef _WIN32
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string datadir(exepath);
	datadir.resize(datadir.find_last_of('\\'));
	if(datadir.substr(datadir.length() - 4) != "\\bin" && datadir.substr(datadir.length() - 6) != "\\.libs") {
		string cainfo = buildPath(datadir, "ssl", "certs", "ca-bundle.crt");
		gsub("\\", "/", cainfo);
		curl_easy_setopt(curl, CURLOPT_CAINFO, cainfo.c_str());
	}
#endif
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	if(res != CURLE_OK || sbuffer.empty()) {return -1;}
	size_t i = sbuffer.find(version_id);
	if(i == string::npos) return -1;
	size_t i2 = sbuffer.find('\n', i + strlen(version_id) + 1);
	string s_version;
	if(i2 == string::npos) s_version = sbuffer.substr(i + strlen(version_id) + 1);
	else s_version = sbuffer.substr(i + strlen(version_id) + 1, i2 - (i + strlen(version_id) + 1));
	remove_blank_ends(s_version);
	if(s_version.empty()) return -1;
	if(available_version) *available_version = s_version;
	if(url) {
#ifdef _WIN32
#	ifdef _WIN64
#		ifdef WIN_PORTABLE
		i = sbuffer.find("windows-x64-portable");
#		else
		i = sbuffer.find("windows-x64-installer");
#		endif
#	else
#		ifdef WIN_PORTABLE
		i = sbuffer.find("windows-i386-portable");
#		else
		i = sbuffer.find("windows-i386-installer");
#		endif
#	endif
#else
		i = sbuffer.find("linux-x86_64-selfcontained");
#endif
		if(i != string::npos) i = sbuffer.find(":", i);
		if(i != string::npos) {
			i2 = sbuffer.find('\n', i);
			if(i2 == string::npos) *url = sbuffer.substr(i + 1, sbuffer.length() - (i + 1));
			else *url = sbuffer.substr(i + 1, i2 - (i + 1));
		}
	}
	if(s_version != current_version) {
		std::vector<int> version_parts_old, version_parts_new;

		string s_old_version = current_version;
		if(s_old_version == "5.5.1" && s_version == "5.5.2") return 0;
		while((i = s_old_version.find('.', 0)) != string::npos) {
			version_parts_old.push_back(s2i(s_old_version.substr(0, i)));
			s_old_version = s_old_version.substr(i + 1);
		}
		i = s_old_version.find_first_not_of("0123456789", 1);
		if(i != string::npos) {
			version_parts_old.push_back(s2i(s_old_version.substr(0, i)));
			s_old_version = s_old_version.substr(i + 1);
		}
		version_parts_old.push_back(s2i(s_old_version));

		while((i = s_version.find('.', 0)) != string::npos) {
			version_parts_new.push_back(s2i(s_version.substr(0, i)));
			s_version = s_version.substr(i + 1);
		}
		i = s_version.find_first_not_of("0123456789", 1);
		if(i != string::npos) {
			version_parts_new.push_back(s2i(s_version.substr(0, i)));
			s_version = s_version.substr(i + 1);
		}
		version_parts_new.push_back(s2i(s_version));

		for(i = 0; i < version_parts_new.size(); i++) {
			if(i == version_parts_old.size() || version_parts_new[i] > version_parts_old[i]) return true;
			else if(version_parts_new[i] < version_parts_old[i]) return false;
		}

	}

	return 0;
#else
	return -1;
#endif
}

void free_thread_caches() {
	mpfr_free_cache2(MPFR_FREE_LOCAL_CACHE);
}

#ifdef _WIN32

Thread::Thread() : running(false), m_thread(NULL), m_threadReadyEvent(NULL), m_threadID(0) {
	m_threadReadyEvent = CreateEvent(NULL, false, false, NULL);
}

Thread::~Thread() {
	cancel();
	CloseHandle(m_threadReadyEvent);
}

void Thread::enableAsynchronousCancel() {}

DWORD WINAPI Thread::doRun(void *data) {
	// create thread message queue
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	Thread *thread = (Thread *) data;
	SetEvent(thread->m_threadReadyEvent);
	thread->run();
	mpfr_free_cache2(MPFR_FREE_LOCAL_CACHE);
	thread->running = false;
	return 0;
}

bool Thread::start() {
	m_thread = CreateThread(NULL, 0, Thread::doRun, this, 0, &m_threadID);
	running = (m_thread != NULL);
	if(!running) return false;
	WaitForSingleObject(m_threadReadyEvent, INFINITE);
	return running;
}

bool Thread::cancel() {
	if(!running) return true;
	// FIXME: this is dangerous
	int ret = TerminateThread(m_thread, 0);
	if(ret == 0) return false;
	CloseHandle(m_thread);
	m_thread = NULL;
	m_threadID = 0;
	running = false;
	return true;
}

#else

Thread::Thread() : running(false), m_pipe_r(NULL), m_pipe_w(NULL) {
	pthread_attr_init(&m_thread_attr);
	int pipe_wr[] = {0, 0};
#ifdef HAVE_PIPE2
	if(pipe2(pipe_wr, O_CLOEXEC) == 0) {
#else
	if(pipe(pipe_wr) == 0) {
#endif
		m_pipe_r = fdopen(pipe_wr[0], "r");
		m_pipe_w = fdopen(pipe_wr[1], "w");
	}
}

Thread::~Thread() {
	cancel();
	fclose(m_pipe_r);
	fclose(m_pipe_w);
	pthread_attr_destroy(&m_thread_attr);
}

void Thread::doCleanup(void *data) {
	Thread *thread = (Thread *) data;
	thread->running = false;
	mpfr_free_cache2(MPFR_FREE_LOCAL_CACHE);
}

void Thread::enableAsynchronousCancel() {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
}

void *Thread::doRun(void *data) {

	pthread_cleanup_push(&Thread::doCleanup, data);

	Thread *thread = (Thread *) data;
	thread->run();

	pthread_cleanup_pop(1);
	return NULL;

}

bool Thread::start() {
	if(running) return true;
	running = pthread_create(&m_thread, &m_thread_attr, &Thread::doRun, this) == 0;
	return running;
}

bool Thread::cancel() {
	if(!running) return true;
	running = pthread_cancel(m_thread) != 0;
	return !running;
}


#endif
