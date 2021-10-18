#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

void print_usage () {
	puts("Usage: ./src/unittest [filename]");
	puts("If filename is omited, it will run all unit tests");
}

pid_t popen2(const char *command, int *infp, int *outfp) {
	int p_stdin[2], p_stdout[2];
	pid_t pid;

	if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
		return -1;

	pid = fork();

	if (pid < 0) {
		return pid;
	} else if (pid == 0) {
		close(p_stdin[PIPE_WRITE]);
		dup2(p_stdin[PIPE_READ], PIPE_READ);
		close(p_stdout[PIPE_READ]);
		dup2(p_stdout[PIPE_WRITE], PIPE_WRITE);

		execl("/bin/sh", "sh", "-c", command, NULL);
		perror("execl");
		exit(1);
	}

	if (infp == NULL) {
		close(p_stdin[PIPE_WRITE]);
	} else {
		*infp = p_stdin[PIPE_WRITE];
	}

	if (outfp == NULL) {
		close(p_stdout[PIPE_READ]);
	} else {
		*outfp = p_stdout[PIPE_READ];
	}

	return pid;
}

void run_unit_test(char *filename) {
	FILE *ftestcase = fopen(filename, "r");
	if (ftestcase == NULL) {
		puts(RED "Cannot open file!" RESET);
		exit(EXIT_FAILURE);
	}

	int p_qalc_stdin;
	int p_qalc_stdout;

	if (popen2("./src/qalc -t -f -", &p_qalc_stdin, &p_qalc_stdout) <= 0) {
		puts(RED "Cannot start qalc!" RESET);
		exit(EXIT_FAILURE);
	}

	FILE* p_qalc_in = fdopen(p_qalc_stdin, "w");
	FILE* p_qalc_out = fdopen(p_qalc_stdout, "r");

	char query[1024 * 1024] = "";
	char input[1024 * 1024] = "";
	char result[1024 * 1024] = "";
	int line = 0;
	int tests = 0;

	while (fgets(input, sizeof(input), ftestcase)) {
		if (input[0] != '\t') {
			input[strcspn(input, "\r\n")] = '\n';
			int written = fputs(input, p_qalc_in);
			if (!written) {
				printf(RED "Unexpected error at writing line number %d, '%s'" RESET, line, input);
				exit(EXIT_FAILURE);
			}
			fflush(p_qalc_in);
			input[strcspn(input, "\r\n")] = 0;
			strcpy(query, input);
			// puts(input);
		} else {
			char *expected = &input[1];

			if (fgets(result, sizeof(result), p_qalc_out) == NULL) {
				printf(RED "Unexpected end at line %d, '%s'" RESET, line, input);
				exit(EXIT_FAILURE);
			}

			expected[strcspn(expected, "\r\n")] = 0;
			result[strcspn(result, "\r\n")] = 0;
			if (strcmp(expected, result) != 0) {
				printf(RED "\nMismatch detected at line %d\n%s\nexpected '%s'\nreceived '%s'\n\n" RESET, line, query, expected, result);
				exit(EXIT_FAILURE);
			}
			tests++;
		}

		line++;
	}

	if (tests == 0) {
		printf(RED "\nWARNING: 0 tests were runned (indentation needs to be tab-based)\n\n" RESET);
	} else {
		printf(GRN "\n%s - %d tests passed\n\n" RESET, filename, tests);
	}
}

int ends_with (const char *str, const char *suffix) {
	if (!str || !suffix)
		return 0;
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if (lensuffix >  lenstr)
		return 0;
	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int main(int argc, char *argv[]) {
	if (argc == 1) {
		puts("Running all unit tests\n");
		if (chdir("..") != 0) {
			printf(RED "\nCannot change directory\n\n" RESET);
		}
		char path[] = "./tests";
		DIR *d;
		struct dirent *dir;
		d = opendir(path);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				if (dir->d_type != DT_REG) continue;
				char *filename = dir->d_name;
				if (!ends_with(filename, ".batch")) continue;
				printf("Running unit tests from '%s'\n", filename);
				char fullPath[4096] = "";
				strcat(fullPath, path);
				strcat(fullPath, "/");
				strcat(fullPath, filename);
				run_unit_test(fullPath);
			}
			closedir(d);
		}
	} else if (argc == 2) {
		char *filename = argv[1];
		run_unit_test(filename);
	} else {
		print_usage();
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
