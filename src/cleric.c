#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "driver/driver.h"
#include "files/files.h"


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

// Parses CLI arguments. Sets *lex_only and returns input_file or NULL on error.
static const char *parse_args(int argc, char *argv[], bool *lex_only) {
    *lex_only = false;
    if (argc == 3 && strcmp(argv[1], "--lex") == 0) {
        *lex_only = true;
        fprintf(stdout, "Lex-only mode enabled\n");
        return argv[2];
    }
    if (argc == 2) {
        return argv[1];
    }
    fprintf(stderr, "Usage: %s [--lex] <input_file.c>\n", argv[0]);
    return NULL;
}

int main(int argc, char *argv[]) {
    bool lex_only = false;
    const char *input_file = parse_args(argc, argv, &lex_only);
    if (!input_file) return 1;
    if (run_preprocessor(input_file) != 0) return 1;
    char i_file[1024];
    if (!filename_replace_ext(input_file, ".i", i_file, sizeof(i_file))) {
        fprintf(stderr, "Failed to construct .i filename\n");
        return 1;
    }
    if (run_compiler(i_file, lex_only) != 0) return 1;
    if (!lex_only) {
        char s_file[1024];
        if (!filename_replace_ext(input_file, ".s", s_file, sizeof(s_file))) {
            fprintf(stderr, "Failed to construct .s filename\n");
            return 1;
        }
        return run_assembler_linker(s_file);
    }
    return 0;
}
