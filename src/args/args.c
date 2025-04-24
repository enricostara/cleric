#include "args.h"
#include <string.h>
#include <stdio.h>

// Parses CLI arguments. Sets flags and returns input_file or NULL on error.
const char *parse_args(const int argc, char *argv[], bool *lex_only, bool *parse_only) {
    *lex_only = false;
    *parse_only = false;

    // Case 1: Standard usage (cleric <file>)
    if (argc == 2) {
        // Ensure the single argument isn't accidentally an option
        if (strcmp(argv[1], "--lex") != 0 && strcmp(argv[1], "--parse") != 0) {
            return argv[1]; // Valid filename
        }
        // If argv[1] is an option, fall through to error
    }
    // Case 2: Mode-specific usage (cleric --lex <file> or cleric --parse <file>)
    else if (argc == 3) {
        if (strcmp(argv[1], "--lex") == 0) {
            *lex_only = true;
            fprintf(stdout, "Lex-only mode enabled\n");
            return argv[2]; // Valid filename for lexing
        }
        if (strcmp(argv[1], "--parse") == 0) { // Removed else-if to allow potential future flags
            *parse_only = true;
            fprintf(stdout, "Parse-only mode enabled\n");
            return argv[2]; // Valid filename for parsing
        }
        // If argv[1] is not --lex or --parse, fall through to error
    }

    // If none of the valid cases matched, print usage and return NULL
    fprintf(stderr, "Usage: %s [--lex | --parse] <input_file.c>\n", argv[0]);
    return NULL;
}
