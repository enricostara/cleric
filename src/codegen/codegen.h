#ifndef CODEGEN_H
#define CODEGEN_H

#include "../parser/ast.h" // Include AST definitions
#include "../strings/strings.h" // Include StringBuffer definition
#include <stdbool.h>      // For bool return type

/**
 * Generates assembly code (x86-64 macOS AT&T syntax) from the AST.
 *
 * @param sb Pointer to the StringBuffer to store the generated assembly code.
 * @param program The root node of the Abstract Syntax Tree.
 * @return true if code generation was successful, false otherwise.
 */
bool codegen_generate_program(StringBuffer *sb, ProgramNode *program);

#endif // CODEGEN_H
