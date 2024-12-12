#include "encrypt-module.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

FILE *input_file;
FILE *output_file;
FILE *log_file;
int input_counts[256];
int output_counts[256];
int input_total_count;
int output_total_count;
int key = 1;
int read_count = 0;
sem_t *sem_char_read;

void clear_counts() {
	memset(input_counts, 0, sizeof(input_counts));
	memset(output_counts, 0, sizeof(output_counts));
	input_total_count = 0;
	output_total_count = 0;
}

void *random_reset() {
	while (1) {
		sem_wait(sem_char_read);
		read_count++;
		if (read_count == 200) {
			reset_requested();
			key += 5;
			clear_counts();
			reset_finished();
			read_count = 0;
		}
	}
}

void init(char *inputFileName, char *outputFileName, char *logFileName) {
	pthread_t pid;
	sem_char_read = sem_open("/sem_test_reset", O_CREAT, 0644, 0);
	sem_unlink("/sem_test_reset");
	pthread_create(&pid, NULL, &random_reset, NULL);
	input_file = fopen(inputFileName, "r");
	output_file = fopen(outputFileName, "w");
	log_file = fopen(logFileName, "w");
}

int read_input() {
	sem_post(sem_char_read);
	return fgetc(input_file);
}

void write_output(int c) {
	fputc(c, output_file);
}

int encrypt(int c) {
	return (c + key - 32) % 94 + 32;
}


void log_counts() {
    fprintf(log_file, "Reset requested.\n");
    fprintf(log_file, "Total input count with current key is %d.\n", input_total_count);

    // Print plaintext frequency counts in formatted style
    fprintf(log_file, "Plaintext frequencies:\n");
    for (int i = 'A'; i <= 'Z'; i++) {
        fprintf(log_file, "%c:%d ", i, input_counts[i]);
        if ((i - 'A' + 1) % 6 == 0) { // Newline after every 6 characters
            fprintf(log_file, "\n");
        }
    }
    fprintf(log_file, "\n");

    fprintf(log_file, "Total output count with current key is %d.\n", output_total_count);

    // Print ciphertext frequency counts in formatted style
    fprintf(log_file, "Ciphertext frequencies:\n");
    for (int i = 'A'; i <= 'Z'; i++) {
        fprintf(log_file, "%c:%d ", i, output_counts[i]);
        if ((i - 'A' + 1) % 6 == 0) { // Newline after every 6 characters
            fprintf(log_file, "\n");
        }
    }
    fprintf(log_file, "\n");

    fprintf(log_file, "Reset finished.\n\n");
}





void count_input(int c) {
    if (isalpha(c)) { // Check if the character is a letter
        c = toupper(c); // Convert to uppercase for case-insensitive counting
        input_counts[c]++;
    }
    input_total_count++;
}


void count_output(int c) {
    if (isalpha(c)) { // Check if the character is a letter
        c = toupper(c); // Convert to uppercase for case-insensitive counting
        output_counts[c]++;
    }
    output_total_count++;
}


int get_input_count(int c) {
	return input_counts[c];
}

int get_output_count(int c) {
	return output_counts[c];
}

int get_input_total_count() {
	return input_total_count;
}

int get_output_total_count() {
	return output_total_count;
}