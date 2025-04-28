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
 * @param codegen_only If true, stop after code generation and print assembly to stdout.
 * @param output_assembly_sb Pointer to an initialized StringBuffer where the generated assembly code will be stored (if not codegen_only).
 *                           The caller is responsible for destroying this buffer.
 * @param out_ast_root Pointer to an AstNode pointer. If parsing succeeds,
 *                     this will be set to the root of the AST. The memory for the AST
 *                     is managed by an internal arena within this function and is freed
 *                     before the function returns. The pointer becomes invalid after the function returns.
 * @return true if the requested compilation stage (or full compilation) succeeded, false otherwise.
 */
bool compile(const char *source_code,
             bool lex_only,
             bool parse_only,
             bool codegen_only,
             StringBuffer *output_assembly_sb, // Output buffer for assembly
             AstNode **out_ast_root);

#endif // COMPILER_H
