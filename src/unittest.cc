#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <dirent.h>
#include <errno.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

void print_usage() {
	puts("Usage: ./src/unittest [filename]");
	puts("If filename is omitted, it will run all unit tests");
}

void run_unit_test(char *filename) {
	char buffer[1000];
#ifdef _WIN32
	snprintf(buffer, 1000, "src\\qalc.exe --test-file=\"%s\"", filename);
#else
	snprintf(buffer, 1000, "src/qalc --test-file=\"%s\"", filename);
#endif
	int ret = system(buffer);
	if(ret == EXIT_SUCCESS) return;
	if(ret == -1 && ret != EXIT_FAILURE) puts(RED "Cannot start qalc!" RESET);
	exit(EXIT_FAILURE);
}

int ends_with(const char *str, const char *suffix) {
	if(!str || !suffix) return 0;
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if(lensuffix >  lenstr) return 0;
	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int main(int argc, char *argv[]) {

	if (argc == 1) {
		puts("Running all unit tests\n");
		struct stat info;
		if(stat("./tests", &info) != 0) {
			if(chdir("..") != 0) {
				printf(RED "\nCannot change directory\n\n" RESET);
			}
		}
		char path[] = "./tests";
		DIR *d;
		struct dirent *dir;
		d = opendir(path);
		if(d) {
			while((dir = readdir(d)) != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
				if(dir->d_type != DT_REG) continue;
#endif
				char *filename = dir->d_name;
				if(!ends_with(filename, ".batch")) continue;
				printf("Running unit tests from '%s'\n", filename);
				char fullPath[4096] = "";
				strcat(fullPath, path);
#ifdef _WIN32
				fflush(stdout);
				strcat(fullPath, "\\");
#else
				strcat(fullPath, "/");
#endif
				strcat(fullPath, filename);
				run_unit_test(fullPath);
			}
			closedir(d);
		}
	} else if(argc == 2) {
		char *filename = argv[1];
		run_unit_test(filename);
	} else {
		print_usage();
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
