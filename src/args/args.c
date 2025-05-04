#include "args.h"
#include <string.h>
#include <stdio.h>

// Parses CLI arguments. Sets flags and returns input_file or NULL on error.
const char *parse_args(const int argc, char *argv[], bool *lex_only, bool *parse_only, bool *irgen_only, bool *codegen_only) {
    *lex_only = false;
    *parse_only = false;
    *irgen_only = false;
    *codegen_only = false;

    // Case 1: Standard usage (cleric <file>)
    if (argc == 2) {
        // Ensure the single argument isn't accidentally an option
        if (strcmp(argv[1], "--lex") != 0 && strcmp(argv[1], "--parse") != 0 && strcmp(argv[1], "--codegen") != 0 && strcmp(argv[1], "--irgen-only") != 0) {
            return argv[1]; // Valid filename
        }
        // If argv[1] is an option, fall through to error
    }
    // Case 2: Mode-specific usage (cleric --lex <file> or cleric --parse <file> or cleric --codegen <file> or cleric --irgen-only <file>)
    else if (argc == 3) {
        if (strcmp(argv[1], "--lex") == 0) {
            *lex_only = true;
            fprintf(stdout, "Lex-only mode enabled\n");
            return argv[2]; // Valid filename for lexing
        }
        if (strcmp(argv[1], "--parse") == 0) { 
            *parse_only = true;
            fprintf(stdout, "Parse-only mode enabled\n");
            return argv[2]; // Valid filename for parsing
        }
        if (strcmp(argv[1], "--codegen") == 0) {
            *codegen_only = true;
            fprintf(stdout, "Codegen-only mode enabled\n");
            return argv[2]; // Valid filename for codegen
        }
        if (strcmp(argv[1], "--irgen-only") == 0) {
            *irgen_only = true;
            fprintf(stdout, "IRGen-only mode enabled\n");
            return argv[2]; // Valid filename for irgen
        }
    }

    // If none of the valid cases matched, print usage and return NULL
    fprintf(stderr, "Usage: %s [--lex | --parse | --codegen] <input_file.c>\n", argv[0]);
    return NULL;
}
