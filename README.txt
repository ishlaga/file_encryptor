# Concurrent Character Encryption Program

## Project Overview
This program implements a multi-threaded character encryption system with maximum concurrency, utilizing producer-consumer patterns and synchronization mechanisms.

## Source Files

### encrypt-module.c
- Responsible for core encryption logic
- Provides initialization, input/output file handling
- Implements character-level encryption algorithm
- Manages input and output character counting
- Handles logging of encryption statistics

### encrypt-driver.c
- Implements the main multi-threaded architecture
- Manages circular input and output buffers
- Coordinates five concurrent threads:
  1. Reader Thread
  2. Input Counter Thread
  3. Encryption Thread
  4. Output Counter Thread
  5. Writer Thread
- Uses semaphores for thread synchronization
- Ensures maximum concurrency across buffer operations

## Synchronization Approach
- Uses semaphores to manage:
  - Buffer space availability
  - Task scheduling
  - Mutual exclusion for shared resources
- Allows concurrent processing of different buffer slots
- Prevents thread blocking through careful semaphore management

## Thread Responsibilities

### Reader Thread
- Reads characters from input file
- Adds characters to input buffer
- Signals availability of new characters

### Input Counter Thread
- Counts input characters
- Processes characters from input buffer
- Runs concurrently with other threads

### Encryption Thread
- Transforms characters using encryption algorithm
- Moves encrypted characters to output buffer
- Operates independently on different buffer slots

### Output Counter Thread
- Counts output (encrypted) characters
- Processes characters from output buffer
- Runs concurrently with other threads


###Usage
Usage
Compilation
To compile the project, use the provided Makefile. Run the following command in the terminal:

bash
Copy code
make
This will compile the encrypt-driver.c and encrypt-module.c files, and generate the executable named encrypt.

Running the Program
After successful compilation, run the program using the following syntax:

bash
Copy code
./encrypt <input_file> <output_file> <log_file>
<input_file>: The name of the file containing the plaintext data to be encrypted.
<output_file>: The name of the file where the encrypted text will be written.
<log_file>: The name of the file where logs, including frequency counts, will be written.
Example
To encrypt the file input.txt and save the output in output.txt with logs written to log.txt, use the following command:

bash
Copy code
./encrypt input.txt output.txt log.txt
Reset Behavior
The program automatically triggers a reset after processing 200 characters. The reset performs the following:

Updates the encryption key by incrementing it by 5.
Logs the character frequency counts (both plaintext and ciphertext) to the specified log file.
Resets internal counters for accurate logging.
Input and Output
Input File: Must contain the text to be encrypted. Ensure the file is accessible and contains valid text characters.
Output File: Contains the encrypted version of the text.
Log File: Contains detailed logging information, including:
Total character counts.
Frequency counts for plaintext and ciphertext.
Reset event notifications.
Cleaning Up
To clean up the compiled files, run:

bash
Copy code
 ## make clean
This removes the executable file and any intermediate compilation artifacts.


### Writer Thread
- Writes encrypted characters to output file
- Retrieves characters from output buffer

## Compilation
Use the provided Makefile:




