#include "args.h"
#include <string.h>
#include <stdio.h>

// Parses CLI arguments. Sets flags and returns input_file or NULL on error.
void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [<options>] <input_file.c>\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --lex          Lex the input, print tokens to stdout, and exit.\n");
    fprintf(stderr, "  --parse        Lex and parse the input, print the AST to stdout, and exit.\n");
    fprintf(stderr, "  --validate     Lex, parse, and validate the input, then exit.\n");
    fprintf(stderr, "  --tac          Lex, parse, validate, and generate Three-Address Code; print TAC to stdout, and exit.\n");
    fprintf(stderr, "  --codegen      Lex, parse, validate, generate TAC, and then assembly; print assembly to stdout, and exit.\n");
    fprintf(stderr, "  (No options)   Run the full pipeline to create an executable.\n");
}

const char *parse_args(const int argc, char *argv[], bool *lex_only, bool *parse_only, bool *validate_only, bool *tac_only, bool *codegen_only) {
    *lex_only = false;
    *parse_only = false;
    *validate_only = false;
    *tac_only = false;
    *codegen_only = false;

    // Case 1: Standard usage (cleric <file>)
    if (argc == 2) {
        // Ensure the single argument isn't accidentally an option
        if (strcmp(argv[1], "--lex") != 0 && 
            strcmp(argv[1], "--parse") != 0 && 
            strcmp(argv[1], "--validate") != 0 && 
            strcmp(argv[1], "--tac") != 0 && 
            strcmp(argv[1], "--tacky") != 0 && 
            strcmp(argv[1], "--codegen") != 0) {
            return argv[1]; // Valid filename
        }
        // If argv[1] is an option, fall through to error
    }
    // Case 2: Mode-specific usage (cleric --option <file>)
    else if (argc == 3) {
        if (strcmp(argv[1], "--lex") == 0) {
            *lex_only = true;
            fprintf(stdout, "Lex-only mode enabled\n");
            return argv[2];
        }
        if (strcmp(argv[1], "--parse") == 0) {
            *parse_only = true;
            fprintf(stdout, "Parse-only mode enabled\n");
            return argv[2];
        }
        if (strcmp(argv[1], "--validate") == 0) {
            *validate_only = true;
            fprintf(stdout, "Validate-only mode enabled\n");
            return argv[2];
        }
        if (strcmp(argv[1], "--tac") == 0 || strcmp(argv[1], "--tacky") == 0) {
            *tac_only = true;
            fprintf(stdout, "TAC-only mode enabled (Three-Address Code)\n");
            return argv[2];
        }
        if (strcmp(argv[1], "--codegen") == 0) {
            *codegen_only = true;
            fprintf(stdout, "Codegen-only mode enabled (Assembly)\n");
            return argv[2];
        }
    }

    // If none of the valid cases matched, print usage and return NULL
    print_usage(argv[0]);
    return NULL;
}
