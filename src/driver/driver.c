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

// Mock assembly payload from return2.s
static const char *mock_asm_payload = ".globl\t_main\n_main:\n\tmovl\t$2, %eax\n\tretq\n";

// New function: mock compilation from .i to .s
int run_compiler(const char *input_file) {
    size_t len = strlen(input_file);
    if (len < 3 || strcmp(input_file + len - 2, ".i") != 0) {
        fprintf(stderr, "Input file should have a .i extension\n");
        return 1;
    }
    char output_file[1024];
    strncpy(output_file, input_file, len - 2);
    output_file[len - 2] = '\0';
    strcat(output_file, ".s");
    FILE *f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to write to %s\n", output_file);
        return 1;
    }
    if (fputs(mock_asm_payload, f) == EOF) {
        fclose(f);
        fprintf(stderr, "Failed to write payload to %s\n", output_file);
        return 1;
    }
    fclose(f);
    // Remove the .i file if all is fine
    if (remove(input_file) != 0) {
        fprintf(stderr, "Warning: could not remove %s\n", input_file);
    }
    printf("Mock compilation output written to %s\n", output_file);
    return 0;
}
