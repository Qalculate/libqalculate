#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <libqalculate/util.h>
#include <cstdio>
#include <cstdlib>

#ifndef _WIN32
class TestThread : public Thread {
	void run() {}
public:
	bool readValue(int *value) { return read(value); }
};
#endif

int main() {
#ifdef _WIN32
	return 77;
#else
	TestThread thread;
	int value = 0;
	if(!thread.write(42) || !thread.readValue(&value) || value != 42) {
		fputs("Thread pipe communication failed\n", stderr);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
#endif
}
