#include "validator.h"
#include "../parser/ast.h"
#include "symbol_table.h"
#include "../memory/arena.h"

#include <stdio.h> // For temporary error printing
#include <string.h> // For strcmp, etc.

// Forward declarations for static visitor functions
static bool validate_node(AstNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_program_node(ProgramNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_func_def_node(FuncDefNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_block_node(BlockNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_var_decl_node(VarDeclNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_identifier_node(IdentifierNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_return_stmt_node(ReturnStmtNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_unary_op_node(UnaryOpNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_binary_op_node(BinaryOpNode* node, SymbolTable* st, Arena* error_arena);
static bool validate_int_literal_node(IntLiteralNode* node, SymbolTable* st, Arena* error_arena);

// Main validation function
bool validate_program(AstNode *program_node, Arena* error_arena) {
    if (!program_node || program_node->type != NODE_PROGRAM) {
        // Handle case where root is not a program node, or is NULL
        // This might be an assertion or a specific error message
        fprintf(stderr, "Error: validate_program called with invalid root node.\n");
        return false;
    }

    SymbolTable st;
    // Assuming the error_arena can also be used by the symbol table for its internal allocations if needed,
    // or the symbol table uses its own part of a larger arena system.
    // For now, symbol_table_init takes an arena, let's use the error_arena for simplicity.
    // In a more complex setup, the symbol table might have its own dedicated arena.
    symbol_table_init(&st, error_arena); // Or a dedicated arena for the symbol table

    bool is_valid = validate_node(program_node, &st, error_arena);

    // symbol_table_free(&st); // Arena handles memory, but good for resetting if ST reused
    return is_valid;
}

// Dispatcher function
static bool validate_node(AstNode* node, SymbolTable* st, Arena* error_arena) {
    if (!node) return true; // Or handle as an error, depending on context

    switch (node->type) {
        case NODE_PROGRAM:       return validate_program_node((ProgramNode*)node, st, error_arena);
        case NODE_FUNC_DEF:      return validate_func_def_node((FuncDefNode*)node, st, error_arena);
        case NODE_BLOCK:         return validate_block_node((BlockNode*)node, st, error_arena);
        case NODE_VAR_DECL:      return validate_var_decl_node((VarDeclNode*)node, st, error_arena);
        case NODE_IDENTIFIER:    return validate_identifier_node((IdentifierNode*)node, st, error_arena);
        case NODE_RETURN_STMT:   return validate_return_stmt_node((ReturnStmtNode*)node, st, error_arena);
        case NODE_UNARY_OP:      return validate_unary_op_node((UnaryOpNode*)node, st, error_arena);
        case NODE_BINARY_OP:     return validate_binary_op_node((BinaryOpNode*)node, st, error_arena);
        case NODE_INT_LITERAL:   return validate_int_literal_node((IntLiteralNode*)node, st, error_arena);
        // Add cases for other node types as they are implemented
        // e.g., NODE_EXPRESSION_STMT, NODE_IF_STMT, NODE_WHILE_STMT, etc.
        default:
            fprintf(stderr, "Warning: validate_node encountered unhandled AST node type: %d\n", node->type);
            return true; // Or false if unhandled nodes mean an error
    }
}

// --- Stub implementations for specific node types --- 

static bool validate_program_node(ProgramNode* node, SymbolTable* st, Arena* error_arena) {
    // A program usually consists of a list of function definitions or other top-level statements.
    // For cleric, it's currently a single function definition.
    if (node->function) {
        return validate_node((AstNode*)node->function, st, error_arena);
    }
    return true;
}

static bool validate_func_def_node(FuncDefNode* node, SymbolTable* st, Arena* error_arena) {
    // TODO: Add function name to symbol table? (depends on function scope handling)
    // TODO: Process parameters - add to a new scope for the function body.
    // For now, just validate the body.
    if (node->body) {
        return validate_node((AstNode*)node->body, st, error_arena);
    }
    return true;
}

static bool validate_block_node(BlockNode* node, SymbolTable* st, Arena* error_arena) {
    if (!symbol_table_enter_scope(st)) {
        fprintf(stderr, "Error: Failed to enter new scope.\n"); // Should ideally use error_arena
        return false;
    }

    for (int i = 0; i < node->num_items; ++i) {
        if (!validate_node(node->items[i], st, error_arena)) {
            // Error already printed by the failing validate_node call, or will be.
            symbol_table_exit_scope(st); // Ensure scope is exited on error path
            return false; // Propagate error
        }
    }
    symbol_table_exit_scope(st);
    return true;
}

static bool validate_var_decl_node(VarDeclNode* node, SymbolTable* st, Arena* error_arena) {
    // Synthesize a dummy token for the declaration.
    // Ideally, AST nodes would store or link back to original tokens for accurate error reporting.
    Token dummy_decl_token; // Not fully initialized, symbol_table_add_symbol only uses .lexeme effectively from it if arena_strdup is used for name
    // However, symbol_table_add_symbol copies the token structure, so we should make it valid.
    dummy_decl_token.type = TOKEN_IDENTIFIER; // Arbitrary, but makes some sense
    dummy_decl_token.lexeme = (char*)node->var_name; // Will be strdup'd by symbol_table_add_symbol if it uses arena_strdup
    dummy_decl_token.position = 0; // Placeholder
    // dummy_decl_token.line, dummy_decl_token.col, dummy_decl_token.length are missing from Token struct

    if (!symbol_table_add_symbol(st, node->var_name, dummy_decl_token)) {
        // TODO: Improve error message with line/col if Token struct is enhanced.
        //       For now, the VarDeclNode doesn't store this info directly.
        fprintf(stderr, "Error: Redeclaration of variable '%s'.\n", node->var_name); // Should use error_arena
        return false;
    }

    if (node->initializer) {
        if (!validate_node(node->initializer, st, error_arena)) {
            return false; // Propagate error from initializer validation
        }
    }
    return true;
}

static bool validate_identifier_node(IdentifierNode* node, SymbolTable* st, Arena* error_arena) {
    const Symbol* found_symbol = symbol_table_lookup_symbol(st, node->name);
    if (!found_symbol) {
        // TODO: Improve error message with line/col from the identifier's token.
        //       IdentifierNode currently only has `name`. The original token isn't directly linked.
        fprintf(stderr, "Error: Undeclared variable '%s'.\n", node->name); // Should use error_arena
        return false;
    }
    return true;
}

static bool validate_return_stmt_node(ReturnStmtNode* node, SymbolTable* st, Arena* error_arena) {
    if (node->expression) {
        return validate_node(node->expression, st, error_arena);
    }
    return true; // Return without expression is valid for void, but cleric is int-only for now.
}

static bool validate_unary_op_node(UnaryOpNode* node, SymbolTable* st, Arena* error_arena) {
    if (node->operand) {
        return validate_node(node->operand, st, error_arena);
    }
    return true;
}

static bool validate_binary_op_node(BinaryOpNode* node, SymbolTable* st, Arena* error_arena) {
    bool left_valid = true;
    bool right_valid = true;
    if (node->left) {
        left_valid = validate_node(node->left, st, error_arena);
    }
    if (node->right) {
        right_valid = validate_node(node->right, st, error_arena);
    }
    return left_valid && right_valid;
}

static bool validate_int_literal_node(IntLiteralNode* node, SymbolTable* st, Arena* error_arena) {
    // Integer literals are generally always valid by themselves in terms of semantic checks at this level.
    // Type checking might happen here later if types are more complex.
    (void)node; // Unused for now
    (void)st;   // Unused for now
    (void)error_arena; // Unused for now
    return true;
}
