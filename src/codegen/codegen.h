#ifndef CODEGEN_H
#define CODEGEN_H

#include "../parser/ast.h" // Include AST definitions

/**
 * Generates assembly code (x86-64 macOS AT&T syntax) from the AST.
 *
 * @param program The root node of the Abstract Syntax Tree.
 * @return A dynamically allocated string containing the assembly code,
 *         or NULL on error. The caller is responsible for freeing the string.
 */
char *codegen_generate_assembly(ProgramNode *program);

#endif // CODEGEN_H
