#ifndef DRIVER_H
#define DRIVER_H

#include <stdbool.h>

/**
 * Runs the gcc preprocessor on the input file and writes output to .i file.
 * Returns 0 on success, 1 on failure.
 */
int run_preprocessor(const char *input_file);

// Compiles a .i file to a .s file (or performs lex/parse only)
int run_compiler(const char *input_file, bool lex_only, bool parse_only);

// Assembles and links a .s file to an executable, removes .s on success
int run_assembler_linker(const char *input_file);

#endif // DRIVER_H
