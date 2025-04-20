#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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
    bool lex_only = false;
    char *input_file = NULL;
    if (argc == 3 && strcmp(argv[1], "--lex") == 0) {
        lex_only = true;
        input_file = argv[2];
    } else if (argc == 2) {
        input_file = argv[1];
    } else {
        fprintf(stderr, "Usage: %s [--lex] <input_file.c>\n", argv[0]);
        return 1;
    }
    if (run_preprocessor(input_file) != 0) return 1;
    size_t len = strlen(input_file);
    char i_file[1024];
    strncpy(i_file, input_file, len - 2);
    i_file[len - 2] = '\0';
    strcat(i_file, ".i");
    if (run_compiler(i_file, lex_only) != 0) return 1;
    if (!lex_only) {
        char s_file[1024];
        strncpy(s_file, input_file, len - 2);
        s_file[len - 2] = '\0';
        strcat(s_file, ".s");
        return run_assembler_linker(s_file);
    }
    return 0;
}
