#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"

/**
 * Runs the gcc preprocessor on the input file and writes output to .i file.
 * Returns 0 on success, 1 on failure.
 */
int run_preprocessor(const char *input_file) {
    size_t len = strlen(input_file);
    if (len < 3 || strcmp(input_file + len - 2, ".c") != 0) {
        fprintf(stderr, "Input file should have a .c extension\n");
        return 1;
    }
    char output_file[1024];
    strncpy(output_file, input_file, len - 2);
    output_file[len - 2] = '\0';
    strcat(output_file, ".i");
    char command[2048];
    snprintf(command, sizeof(command), "gcc -E -P %s -o %s", input_file, output_file);
    const int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Failed to preprocess %s\n", input_file);
        return 1;
    }
    printf("Preprocessed output written to %s\n", output_file);
    return 0;
}

