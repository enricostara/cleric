#ifndef CLERIC_VALIDATOR_H
#define CLERIC_VALIDATOR_H

#include "../parser/ast.h"
#include "../memory/arena.h"
#include <stdbool.h>

/**
 * @brief Validates the entire program represented by the AST.
 * @param program_node The root node of the AST (should be a ProgramNode).
 * @param error_arena Arena to allocate memory for error messages (if any).
 * @return true if the program is valid, false otherwise.
 *         More detailed error reporting mechanisms will be added later.
 */
bool validate_program(AstNode *program_node, Arena* error_arena);

#endif //CLERIC_VALIDATOR_H
