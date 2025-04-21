/*
 * cleric.c - Main entry point for the Cleric C compiler
 *
 * This file handles command-line argument parsing, orchestrates the build pipeline, and manages
 * the integration of the preprocessor, compiler, and assembler/linker stages.
 *
 * Pipeline:
 *   1. Preprocess: Converts <input>.c to <input>.i using the system's C preprocessor (gcc -E).
 *   2. Compile:    Converts <input>.i to <input>.s (mocked, with optional lex-only mode).
 *   3. Assemble & Link: Converts <input>.s to an executable (unless in lex-only mode).
 *
 * Usage:
 *   cleric [--lex] <input_file.c>
 *     --lex  : Only lex the input and print tokens (no codegen or linking)
 *
 * This main file delegates file manipulation to src/files/files.c and uses driver logic in src/driver/driver.c.
 *
 * Test suites are provided in tests/ for each module.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "driver/driver.h"
#include "lexer/lexer.h"
#include "files/files.h"
#include "args/args.h"


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
