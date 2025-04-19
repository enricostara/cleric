#include <stdio.h>

#include "driver/driver.h"


/**
 * The main entry point of the program.
 * 
 * This program takes a single command-line argument, the input C file.
 * It checks if the input file has a .c extension, and if so, it runs the
 * gcc preprocessor on the file and writes the output to a file with the
 * same name but with a .i extension.
 * 
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    // Check for correct number of arguments
    if (argc != 2) {
        // Print usage message if incorrect number of arguments
        fprintf(stderr, "Usage: %s <input_file.c>\n", argv[0]);
        return 1;
    }

    // Get the input file name from the command-line argument
    char *input_file = argv[1];

    // Call the preprocessor logic
    return run_preprocessor(input_file);
}
