#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include "../strings/strings.h" // For StringBuffer
#include "../memory/arena.h"  // For Arena

/**
 * @brief Core compilation logic: Source String -> Assembly String Buffer.
 *
 * Takes source code as a string and runs the lexer, parser, and optionally
 * the code generator.
 *
 * @param source_code The C source code string to compile.
 * @param lex_only If true, stop after lexing and print tokens.
 * @param parse_only If true, stop after parsing and print AST.
 * @param validate_only If true, stop after validation.
 * @param tac_only If true, stop after IR generation.
 * @param codegen_only If true, stop after code generation and print assembly to stdout.
 * @param output_assembly_sb Pointer to an initialized StringBuffer where the generated assembly code will be stored (if not codegen_only).
 *                           The caller is responsible for destroying this buffer.
 * @param arena The arena to use for memory allocation.
 * @return true if the requested compilation stage (or full compilation) succeeded, false otherwise.
 */
bool compile(const char *source_code,
             bool lex_only,
             bool parse_only,
             bool validate_only,
             bool tac_only,
             bool codegen_only,
             StringBuffer *output_assembly_sb,
             Arena *arena);

#endif // COMPILER_H
