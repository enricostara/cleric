#include "args.h"
#include <string.h>
#include <stdio.h>

// Parses CLI arguments. Sets *lex_only and returns input_file or NULL on error.
const char *parse_args(int argc, char *argv[], bool *lex_only) {
    *lex_only = false;

    // Case 1: Standard usage (cleric <file>)
    if (argc == 2) {
        // Ensure the single argument isn't accidentally '--lex'
        if (strcmp(argv[1], "--lex") != 0) {
            return argv[1]; // Valid filename
        }
        // If argv[1] is "--lex", fall through to error
    }
    // Case 2: Lex-only usage (cleric --lex <file>)
    else if (argc == 3 && strcmp(argv[1], "--lex") == 0) {
        *lex_only = true;
        fprintf(stdout, "Lex-only mode enabled\n");
        return argv[2]; // Valid filename for lexing
    }

    // If none of the valid cases matched, print usage and return NULL
    fprintf(stderr, "Usage: %s [--lex] <input_file.c>\n", argv[0]);
    return NULL;
}
