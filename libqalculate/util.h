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

string& gsub(const string &pattern, const string &sub, string &str);
string& gsub(const char *pattern, const char *sub, string &str);
string d2s(double value, int precision = 100);
string i2s(int value);
string i2s(long int value);
string i2s(unsigned int value);
string i2s(unsigned long int value);
const char *b2yn(bool b, bool capital = true);
const char *b2tf(bool b, bool capital = true);
const char *b2oo(bool b, bool capital = true);
string p2s(void *o);
int s2i(const string& str);
int s2i(const char *str);
void *s2p(const string& str);
void *s2p(const char *str);

void now(int &hour, int &min, int &sec);

size_t find_ending_bracket(const string &str, size_t start, int *missing = NULL);

char op2ch(MathOperation op);

string& wrap_p(string &str);
string& remove_blanks(string &str);
string& remove_duplicate_blanks(string &str);
string& remove_blank_ends(string &str);
string& remove_parenthesis(string &str);

bool is_in(const char *str, char c);
bool is_not_in(const char *str, char c);
bool is_in(const string &str, char c);
bool is_not_in(const string &str, char c);
int sign_place(string *str, size_t start = 0);
int gcd(int i1, int i2);

#ifdef _WIN32
string utf8_encode(const wstring &wstr);
#endif
char *locale_to_utf8(const char *str);
char *locale_from_utf8(const char *str);
char *utf8_strdown(const char *str, int l = -1);
size_t unicode_length(const string &str);
size_t unicode_length(const char *str);
bool text_length_is_one(const string &str);
bool equalsIgnoreCase(const string &str1, const string &str2);
bool equalsIgnoreCase(const string &str1, const char *str2);

void parse_qalculate_version(string qalculate_version, int *qalculate_version_numbers);

string getOldLocalDir();
string getLocalDir();
string getPackageDataDir();
string getGlobalDefinitionsDir();
string getPackageLocaleDir();
string getLocalDataDir();
string getLocalTmpDir();
string buildPath(string dir, string filename);
string buildPath(string dir1, string dir2, string filename);
string buildPath(string dir1, string dir2, string dir3, string filename);
bool dirExists(string dirpath);
bool fileExists(string dirpath);
bool makeDir(string dirpath);
bool recursiveMakeDir(string dirpath);
bool removeDir(string dirpath);
bool move_file(const char *from_file, const char *to_file);

int checkAvailableVersion(const char *version_id, const char *current_version, string *avaible_version = NULL, int timeout = 5);

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
