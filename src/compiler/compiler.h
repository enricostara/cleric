#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include "../strings/strings.h" // For StringBuffer
#include "../parser/ast.h"    // For AstNode

/**
 * @brief Core compilation logic: Source String -> Assembly String Buffer.
 *
 * Takes source code as a string and runs the lexer, parser, and optionally
 * the code generator.
 *
 * @param source_code The C source code string to compile.
 * @param lex_only If true, stop after lexing and print tokens.
 * @param parse_only If true, stop after parsing and print AST.
 * @param irgen_only If true, stop after IR generation.
 * @param codegen_only If true, stop after code generation and print assembly to stdout.
 * @param output_assembly_sb Pointer to an initialized StringBuffer where the generated assembly code will be stored (if not codegen_only).
 *                           The caller is responsible for destroying this buffer.
 * @return true if the requested compilation stage (or full compilation) succeeded, false otherwise.
 */
bool compile(const char *source_code,
             // Input source code string
             const bool lex_only,
             // Stop after IR generation?
             const bool parse_only,
             // Stop after lexing?
             const bool irgen_only,
             // Stop after parsing?
             const bool codegen_only,
             // Stop after code generation?
             StringBuffer *output_assembly_sb
             // Output buffer for assembly
             );                          // AST is managed internally via arena

#endif // COMPILER_H
