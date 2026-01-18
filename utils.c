#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define INITIAL_CAPACITY 32

static void clearLine(void);

static void clearLine(void) {
	int ch = 0;
	do {
		ch = getchar();
	} while (ch != '\n' && ch != EOF);
}

int getInt(const char* prompt) {
	int value = 0;

	if (prompt != NULL) {
		printf("%s", prompt);
	}

	if (scanf("%d", &value) != 1) {
		clearLine();
		return 0;
	}

	clearLine();
	return value;
}

char* getString(const char* prompt) {
	size_t cap = INITIAL_CAPACITY;
	size_t len = 0;
	char* buffer = (char*)malloc(cap * sizeof(char));
	int ch = 0;

	if (buffer == NULL) {
		exit(1);
	}

	if (prompt != NULL) {
		printf("%s", prompt);
	}

	while (1) {
		ch = getchar();
		if (ch == '\n' || ch == EOF) {
			break;
		}

		if (len + 1 >= cap) {
			size_t newCap = cap * 2;
			char* tmp = (char*)realloc(buffer, newCap * sizeof(char));
			if (tmp == NULL) {
				free(buffer);
				exit(1);
			}
			buffer = tmp;
			cap = newCap;
		}

		buffer[len] = (char)ch;
		len++;
	}

	buffer[len] = '\0';
	return buffer;
}
