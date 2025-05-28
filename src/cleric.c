/*
 * cleric.c - Main entry point for the Cleric C compiler
 *
 * This file handles command-line argument parsing, orchestrates the build pipeline, and manages
 * the integration of the preprocessor, compiler, and assembler/linker stages.
 *
 * Pipeline:
 *   1. Preprocess: Converts <input>.c to <input>.i using the system's C preprocessor (e.g., gcc -E).
 *   2. Compile Core: Processes the preprocessed <input>.i file. This stage involves:
 *      - Lexing: Converts the source text into a stream of tokens.
 *      - Parsing: Builds an Abstract Syntax Tree (AST) from the tokens, verifying syntax.
 *                 Currently supports expressions with operator precedence and provides detailed error reporting.
 *      - [Planned] Intermediate Representation (IR): Converts the AST to Three-Address Code (TAC).
 *      - [Planned] Code Generation: Translates TAC into <input>.s (assembly code).
 *      (The compiler can be instructed to stop and output after lexing, parsing, TAC generation, or assembly via options.)
 *   3. Assemble & Link: Converts <input>.s to an executable (if the full compilation pipeline is run, i.e.,
 *                       no --lex, --parse, --tac, or --codegen options are used).
 *
 * Usage:
 *   cleric [<options>] <input_file.c>
 *     --lex      : Lex the input, print tokens to stdout, and exit.
 *     --parse    : Lex and parse the input, print the AST to stdout, and exit.
 *     --tac      : Lex, parse, and generate Three-Address Code; print TAC to stdout, and exit.
 *     --codegen  : Lex, parse, generate TAC, and then assembly; print assembly to stdout, and exit.
 *     (No options): Run the full pipeline to create an executable.
 *
 * This main file delegates file manipulation to src/files/files.c, argument parsing to src/args/args.c,
 * and uses driver logic in src/driver/driver.c.
 *
 * Test suites are provided in tests/ for each module.
 */

#include <string.h>
#include <stdbool.h>
#include "compiler/driver.h"
#include "files/files.h"
#include "args/args.h"


int main(int argc, char *argv[]) {
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool tac_only = false;
    bool codegen_only = false;
    const char *input_file = parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &tac_only, &codegen_only);
    if (!input_file) return 1;
    if (run_preprocessor(input_file) != 0) return 1;
    char i_file[1024];
    if (!filename_replace_ext(input_file, ".i", i_file, sizeof(i_file))) {
        fprintf(stderr, "Failed to construct .i filename\n");
        return 1;
    }
    // Pass all flags to the compiler driver
    if (run_compiler(i_file, lex_only, parse_only, validate_only, tac_only, codegen_only) != 0) return 1;

    // Skip assembly/linking if any "only" mode is active
    if (!lex_only && !parse_only && !validate_only && !tac_only && !codegen_only) {
        char s_file[1024];
        if (!filename_replace_ext(input_file, ".s", s_file, sizeof(s_file))) {
            fprintf(stderr, "Failed to construct .s filename\n");
            return 1;
        }
        return run_assembler_linker(s_file);
    }
    return 0;
}
