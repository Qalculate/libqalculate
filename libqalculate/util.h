/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UTIL_H
#define UTIL_H

#include <libqalculate/includes.h>
/* threads */
#ifdef _WIN32
#	undef WINVER
#	undef _WIN32_WINNT
#	define WINVER 0x0600
#	define _WIN32_WINNT 0x0600
#	include <winsock2.h>
#	include <windows.h>
#else
#	include <pthread.h>
#	include <unistd.h>
#endif

#ifdef _WIN32
	#define srand48 srand
#endif

/** @file */

/// \cond
struct eqstr {
    bool operator()(const char *s1, const char *s2) const;
};
/// \endcond

void sleep_ms(int milliseconds);

std::string& gsub(const std::string &pattern, const std::string &sub, std::string &str);
std::string& gsub(const char *pattern, const char *sub, std::string &str);
std::string d2s(double value, int precision = 100);
std::string i2s(long int value);
std::string u2s(unsigned long int value);
const char *b2yn(bool b, bool capital = true);
const char *b2tf(bool b, bool capital = true);
const char *b2oo(bool b, bool capital = true);
std::string p2s(void *o);
long int s2i(const std::string& str);
long int s2i(const char *str);
void *s2p(const std::string& str);
void *s2p(const char *str);

void now(int &hour, int &min, int &sec);

size_t find_ending_bracket(const std::string &str, size_t start, int *missing = NULL);

char op2ch(MathOperation op);

std::string& wrap_p(std::string &str);
std::string& remove_blanks(std::string &str);
std::string& remove_duplicate_blanks(std::string &str);
std::string& remove_blank_ends(std::string &str);
std::string& remove_parenthesis(std::string &str);

bool is_in(const char *str, char c);
bool is_not_in(const char *str, char c);
bool is_in(const std::string &str, char c);
bool is_not_in(const std::string &str, char c);
int sign_place(std::string *str, size_t start = 0);
int gcd(int i1, int i2);

#ifdef _WIN32
std::string utf8_encode(const std::wstring &wstr);
#endif
char *locale_to_utf8(const char *str);
char *locale_from_utf8(const char *str);
char *utf8_strdown(const char *str, int l = -1);
char *utf8_strup(const char *str, int l = -1);
size_t unicode_length(const std::string &str);
size_t unicode_length(const char *str);
size_t unicode_length(const std::string &str, size_t l);
size_t unicode_length(const char *str, size_t l);
bool text_length_is_one(const std::string &str);
std::string sub_suffix_html(const std::string &name);
std::string sub_suffix(const std::string &name, const std::string &tag_begin, const std::string &tag_end);
bool equalsIgnoreCase(const std::string &str1, const std::string &str2);
bool equalsIgnoreCase(const std::string &str1, const char *str2);

void parse_qalculate_version(std::string qalculate_version, int *qalculate_version_numbers);

std::string getOldLocalDir();
std::string getLocalDir();
std::string getPackageDataDir();
std::string getGlobalDefinitionsDir();
std::string getPackageLocaleDir();
std::string getLocalDataDir();
std::string getLocalTmpDir();
std::string buildPath(std::string dir, std::string filename);
std::string buildPath(std::string dir1, std::string dir2, std::string filename);
std::string buildPath(std::string dir1, std::string dir2, std::string dir3, std::string filename);
bool dirExists(std::string dirpath);
bool fileExists(std::string dirpath);
bool makeDir(std::string dirpath);
bool recursiveMakeDir(std::string dirpath);
bool removeDir(std::string dirpath);
bool move_file(const char *from_file, const char *to_file);

int checkAvailableVersion(const char *version_id, const char *current_version, std::string *available_version = NULL, int timeout = 5);
int checkAvailableVersion(const char *version_id, const char *current_version, std::string *available_version, std::string *url, int timeout = 5);

class Thread {
public:
	Thread();
	virtual ~Thread();
	bool start();
	bool cancel();
	template <class T> bool write(T data) {
#ifdef _WIN32
		int ret = PostThreadMessage(m_threadID, WM_USER, (WPARAM) data, 0);
		return (ret != 0);
#else
		int ret = fwrite(&data, sizeof(T), 1, m_pipe_w);
		if(ret != 1) return false;
		fflush(m_pipe_w);
		return true;
#endif
	}

	// FIXME: this is technically wrong -- needs memory barriers (std::atomic?)
	volatile bool running;

protected:
	virtual void run() = 0;
	void enableAsynchronousCancel();
	template <class T> T read() {
#ifdef _WIN32
		MSG msg;
		int ret = GetMessage(&msg, NULL, WM_USER, WM_USER);
		return (T) msg.wParam;
#else
		T x;
		fread(&x, sizeof(T), 1, m_pipe_r);
		return x;
#endif
	}
	template <class T> bool read(T *data) {
#ifdef _WIN32
		MSG msg;
		int ret = GetMessage(&msg, NULL, WM_USER, WM_USER);
		if(ret == 0 || ret == -1) return false;
		*data = (T) msg.wParam;
		return true;
#else
		int ret = fread(data, sizeof(T), 1, m_pipe_r);
		return ret == 1;
#endif
	}

private:
#ifdef _WIN32
	static DWORD WINAPI doRun(void *data);

	HANDLE m_thread, m_threadReadyEvent;
	DWORD m_threadID;
#else
	static void doCleanup(void *data);
	static void *doRun(void *data);

	pthread_t m_thread;
	pthread_attr_t m_thread_attr;
	FILE *m_pipe_r, *m_pipe_w;
#endif
};

#endif
