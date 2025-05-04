#ifndef CLERIC_AST_TO_TAC_H
#define CLERIC_AST_TO_TAC_H

#include "../parser/ast.h" // For AST node types (ProgramNode)
#include "tac.h"           // For TAC types (TacProgram)
#include "../memory/arena.h" // For Arena allocator

/**
 * @brief Translates the abstract syntax tree (AST) into three-address code (TAC).
 *
 * This function traverses the input AST root node and generates an equivalent
 * program represented in TAC instructions, managed within a TacProgram structure.
 * All allocations required for the TAC representation are made using the provided arena.
 *
 * @param ast_root A pointer to the root node of the AST (ProgramNode).
 * @param arena A pointer to the arena allocator to use for TAC generation.
 * @return A pointer to the generated TacProgram, or NULL if translation fails.
 */
TacProgram* ast_to_tac(ProgramNode* ast_root, Arena* arena);

#endif // CLERIC_AST_TO_TAC_H
