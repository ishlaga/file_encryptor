#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include "encrypt-module.h"

// Flag to indicate if the reader thread has reached EOF
int is_end_of_file = 0;

// Input buffer attributes
int input_cursor_count = 0;   // Cursor index for counting characters
int input_cursor_encrypt = 0; // Cursor index for encrypting characters
int input_buffer_tail = 0;    // Tail index for adding characters
int input_buffer_capacity = -1;  // Size of the input buffer

// Output buffer attributes
int output_cursor_count = 0;  // Cursor index for counting characters
int output_cursor_write = 0;  // Cursor index for writing characters
int output_buffer_tail = 0;   // Tail index for adding characters
int output_buffer_capacity = -1;  // Size of the output buffer

char* input_data_buffer;      // Input buffer
char* output_data_buffer;     // Output buffer

// Semaphores for managing buffer spaces and tasks
sem_t sem_space_input_count;    // Available space for counting in the input buffer
sem_t sem_space_input_encrypt;  // Available space for encrypting in the input buffer
sem_t sem_space_output_count;   // Available space for counting in the output buffer
sem_t sem_space_output_writer;  // Available space for writing in the output buffer
sem_t sem_work_input_count;     // Tasks available for input counting
sem_t sem_work_encrypt;         // Tasks available for encryption
sem_t sem_work_output_count;    // Tasks available for output counting
sem_t sem_work_write;           // Tasks available for writing

// Mutex semaphores for synchronizing access
sem_t sem_input_buffer_mutex;   // Mutex for input buffer
sem_t sem_output_buffer_mutex;  // Mutex for output buffer
sem_t sem_reader_thread_mutex;  // Mutex for reader thread

// Thread identifiers
pthread_t reader_thread;        // Reader thread
pthread_t input_counter_thread; // Input counter thread
pthread_t encrypter_thread;     // Encryption thread
pthread_t output_counter_thread;// Output counter thread
pthread_t writer_thread;        // Writer thread

// Add a character to the input buffer
void buffer_input_append(char item) {
  input_data_buffer[input_buffer_tail++] = item;
  input_buffer_tail %= input_buffer_capacity;
}

// Remove a character from the input buffer for counting
char buffer_input_get_for_count() {
  char item = input_data_buffer[input_cursor_count++];
  input_cursor_count %= input_buffer_capacity;
  return item;
}

// Remove a character from the input buffer for encrypting
char buffer_input_get_for_encrypt() {
  char item = input_data_buffer[input_cursor_encrypt++];
  input_cursor_encrypt %= input_buffer_capacity;
  return item;
}

// Add a character to the output buffer
void buffer_output_append(char item) {
  output_data_buffer[output_buffer_tail++] = item;
  output_buffer_tail %= output_buffer_capacity;
}

// Remove a character from the output buffer for counting
char buffer_output_get_for_count() {
  char item = output_data_buffer[output_cursor_count++];
  output_cursor_count %= output_buffer_capacity;
  return item;
}

// Remove a character from the output buffer for writing
char buffer_output_get_for_write() {
  char item = output_data_buffer[output_cursor_write++];
  output_cursor_write %= output_buffer_capacity;
  return item;
}

// Print the current state of the input buffer
void debug_print_input_buffer() {
  printf("Input Buffer:\n");
  printf("Size: %d\n", input_buffer_capacity);
  for (int i = 0; i < input_buffer_capacity; i++) {
    printf("%d: %c\n", i, input_data_buffer[i]);
  }
  printf("\n");
}

// Print the current state of the output buffer
void debug_print_output_buffer() {
  printf("Output Buffer:\n");
  printf("Size: %d\n", output_buffer_capacity);
  for (int i = 0; i < output_buffer_capacity; i++) {
    printf("%d: %c\n", i, output_data_buffer[i]);
  }
  printf("\n");
}

// Check if the buffers are ready to reset
int is_buffer_reset_ready() {
  int val1, val2, val3, val4;
  int is_ready = 0;
  sem_getvalue(&sem_work_input_count, &val1);
  sem_getvalue(&sem_work_encrypt, &val2);
  sem_getvalue(&sem_work_output_count, &val3);
  sem_getvalue(&sem_work_write, &val4);
  sem_wait(&sem_input_buffer_mutex);
  sem_wait(&sem_output_buffer_mutex);

  if (   val1 == 0
      && val2 == 0
      && val3 == 0
      && val4 == 0 
      && get_input_total_count() == get_output_total_count() ) {
    is_ready = 1;
  }
  sem_post(&sem_input_buffer_mutex);
  sem_post(&sem_output_buffer_mutex);
  return is_ready;
}

// Check if the program has completed execution
int is_program_complete() {
  int val1, val2, val3, val4, val5, val6;
  sem_getvalue(&sem_work_input_count, &val1);
  sem_getvalue(&sem_work_encrypt, &val2);
  sem_getvalue(&sem_work_output_count, &val3);
  sem_getvalue(&sem_work_write, &val4); 
  sem_getvalue(&sem_input_buffer_mutex, &val5);
  sem_getvalue(&sem_output_buffer_mutex, &val6);

  if (is_end_of_file
      && val1 == 0
      && val2 == 0
      && val3 == 0
      && val4 == 0 
      && val5 == 1
      && val6 == 1 ) {
    return 1;
  }
  return 0;
}

// Wait until buffers are safe to reset, then log counts
void reset_requested() {
  sem_wait(&sem_reader_thread_mutex);
  while(!is_buffer_reset_ready()) {}  
  log_counts(); 
}

// Allow the reader thread to run again after reset
void reset_finished() {
  sem_post(&sem_reader_thread_mutex);
}

// Reader thread: reads characters from the input file and adds them to the input buffer
void* thread_reader() {
  char current_char;
  while (!is_program_complete()) {
    while ((current_char = read_input()) != EOF) {
      sem_wait(&sem_space_input_count);
      sem_wait(&sem_space_input_encrypt);
      sem_wait(&sem_reader_thread_mutex);
      sem_wait(&sem_input_buffer_mutex);
      buffer_input_append(current_char);
      sem_post(&sem_reader_thread_mutex);
      sem_post(&sem_input_buffer_mutex);
      sem_post(&sem_work_encrypt);
      sem_post(&sem_work_input_count);
    }
    is_end_of_file = 1;
  }
}

// Input counter thread: counts characters from the input buffer
void* thread_input_counter() {
  while (1) {
    sem_wait(&sem_work_input_count);
    sem_wait(&sem_input_buffer_mutex);
    char current_char = buffer_input_get_for_count();
    sem_post(&sem_input_buffer_mutex);
    count_input(current_char);
    sem_post(&sem_space_input_count);
  }
}

// Encryption thread: encrypts characters from the input buffer and adds them to the output buffer
void* thread_encrypter() {
  while (1) {
    sem_wait(&sem_work_encrypt);
    sem_wait(&sem_input_buffer_mutex);
    char current_char = buffer_input_get_for_encrypt();
    sem_post(&sem_input_buffer_mutex);
    sem_post(&sem_space_input_encrypt);
    current_char = encrypt(current_char);
    sem_wait(&sem_space_output_count);
    sem_wait(&sem_space_output_writer);
    sem_wait(&sem_output_buffer_mutex);
    buffer_output_append(current_char);
    sem_post(&sem_output_buffer_mutex);
    sem_post(&sem_work_output_count);
    sem_post(&sem_work_write);
  }
}

// Output counter thread: counts characters from the output buffer
void* thread_output_counter() {
  while (1) {
    sem_wait(&sem_work_output_count);
    sem_wait(&sem_output_buffer_mutex);
    char current_char = buffer_output_get_for_count();
    sem_post(&sem_output_buffer_mutex);
    count_output(current_char);
    sem_post(&sem_space_output_count);
  }
}

// Writer thread: writes characters from the output buffer to the output file
void* thread_writer() {
  while (1) {
    sem_wait(&sem_work_write);
    sem_wait(&sem_output_buffer_mutex);
    char current_char = buffer_output_get_for_write();
    sem_post(&sem_space_output_writer);
    sem_post(&sem_output_buffer_mutex);
    write_output(current_char);
  }
}

// Main function: initializes the program and creates threads
int main(int argc, char *argv[]) {
  if(argc < 3) {
    printf("Please include input, output, and log filenames as arguments\n");
    return 0;
  }
  init(argv[1], argv[2], argv[3]);
  while (input_buffer_capacity < 1 || output_buffer_capacity < 1) {
    printf("Size of input buffer N:\n");
    scanf("%d", &input_buffer_capacity);
    printf("Size of output buffer M:\n");
    scanf("%d", &output_buffer_capacity);
    if (input_buffer_capacity < 1 || output_buffer_capacity < 1) {
      printf("Buffer sizes must be > 1\n");
    }
  }

  input_data_buffer = (char*) malloc(sizeof(char) * (input_buffer_capacity + 1));
  output_data_buffer = (char*) malloc(sizeof(char) * (output_buffer_capacity + 1));

  // Initialize semaphores
  sem_init(&sem_space_input_encrypt, 0, input_buffer_capacity);
  sem_init(&sem_space_input_count, 0, input_buffer_capacity);
  sem_init(&sem_space_output_count, 0, output_buffer_capacity);
  sem_init(&sem_space_output_writer, 0, output_buffer_capacity);

  sem_init(&sem_work_input_count, 0, 0);
  sem_init(&sem_work_encrypt, 0, 0);
  sem_init(&sem_work_output_count, 0, 0);
  sem_init(&sem_work_write, 0, 0);

  sem_init(&sem_input_buffer_mutex, 0, 1);
  sem_init(&sem_output_buffer_mutex, 0, 1);
  sem_init(&sem_reader_thread_mutex, 0, 1);

  // Create threads
  pthread_create(&reader_thread, NULL, thread_reader, NULL);
  pthread_create(&input_counter_thread, NULL, thread_input_counter, NULL);
  pthread_create(&encrypter_thread, NULL, thread_encrypter, NULL);
  pthread_create(&output_counter_thread, NULL, thread_output_counter, NULL);
  pthread_create(&writer_thread, NULL, thread_writer, NULL);

  // Wait for the reader thread to finish
  pthread_join(reader_thread, NULL);

  printf("End of file reached.\n");
  log_counts();
}

