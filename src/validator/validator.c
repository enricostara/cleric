#include "validator.h"
#include "../parser/ast.h"
#include "symbol_table.h"
#include "../memory/arena.h"

#include <stdio.h>  // For fprintf
#include <string.h> // For strcmp, strlen
#include <stdarg.h> // For va_list, va_start, va_end

#include "strings/strings.h"

// --- Validator Context --- 
typedef struct {
    SymbolTable* st;
    Arena* arena; // For errors and decorated names
    int tac_temp_id_counter;
    char* error_message; // Store the first error message
    bool error_flag;     // Flag to indicate if an error occurred
    // TODO: Add current function context for return type checking if needed
} ValidatorContext;

// --- Error Reporting --- 
// Helper to report a validation error. Only the first error is stored.
// Node is optional, for future use with line numbers.
static void validator_error(ValidatorContext *ctx, AstNode *node, const char *format, ...) {
    if (ctx->error_flag) {
        return; // Only store the first error
    }
    ctx->error_flag = true;

    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Store in arena
    size_t len = strlen(buffer);
    ctx->error_message = arena_alloc(ctx->arena, len + 1);
    if (ctx->error_message) {
        strcpy(ctx->error_message, buffer);
    }
    // For now, also print to stderr for immediate visibility
    // TODO: Integrate with a more robust error reporting mechanism that can show line numbers from 'node'
    if (node && node->type == NODE_VAR_DECL) { // Example: use token from VarDeclNode
        Token t = ((VarDeclNode*)node)->declaration_token;
        fprintf(stderr, "Validation Error (position %zu, near '%s'): %s\n", t.position, t.lexeme, buffer);
    } else if (node && node->type == NODE_IDENTIFIER) { // Example: use token info if available (not yet in IdentifierNode)
        // Token t = ((IdentifierNode*)node)->token; // Assuming IdentifierNode would store its token
        // fprintf(stderr, "Validation Error (line %d, col %d): %s\n", t.line, t.column, buffer);
        fprintf(stderr, "Validation Error: %s\n", buffer); // Fallback if no token info
    } else {
        fprintf(stderr, "Validation Error: %s\n", buffer);
    }
}

// --- Forward declarations for static visitor functions --- 
static bool validate_node(AstNode* node, ValidatorContext* ctx);
static bool validate_program_node(ProgramNode* node, ValidatorContext* ctx);
static bool validate_func_def_node(FuncDefNode* node, ValidatorContext* ctx);
static bool validate_block_node(BlockNode* node, ValidatorContext* ctx);
static bool validate_var_decl_node(VarDeclNode* node, ValidatorContext* ctx);
static bool validate_identifier_node(IdentifierNode* node, ValidatorContext* ctx);
static bool validate_return_stmt_node(ReturnStmtNode* node, ValidatorContext* ctx);
static bool validate_unary_op_node(UnaryOpNode* node, ValidatorContext* ctx);
static bool validate_binary_op_node(BinaryOpNode* node, ValidatorContext* ctx);
static bool validate_int_literal_node(IntLiteralNode* node, ValidatorContext* ctx);
static bool validate_assignment_exp_node(AssignmentExpNode* node, ValidatorContext* ctx);

// --- Main validation function --- 
bool validate_program(AstNode *program_node, Arena* error_arena) {
    if (!program_node || program_node->type != NODE_PROGRAM) {
        // This error is before context is set up, so direct fprintf
        fprintf(stderr, "Critical Error: validate_program called with invalid root node.\n");
        return false;
    }

    SymbolTable st;
    ValidatorContext ctx;
    ctx.st = &st;
    ctx.arena = error_arena;
    ctx.tac_temp_id_counter = 0;
    ctx.error_message = NULL;
    ctx.error_flag = false;

    symbol_table_init(ctx.st, ctx.arena);

    bool is_valid = validate_node(program_node, &ctx);

    // symbol_table_free(ctx.st); // Arena handles memory
    
    if (ctx.error_flag && ctx.error_message) {
        // Optionally, make error_message accessible to the caller if needed
        // For now, it's printed by validator_error
    }
    return !ctx.error_flag; // Program is valid if no error flag was set
}

// --- Dispatcher function --- 
static bool validate_node(AstNode* node, ValidatorContext* ctx) {
    if (!node || ctx->error_flag) return !ctx->error_flag; 

    switch (node->type) {
        case NODE_PROGRAM:       return validate_program_node((ProgramNode*)node, ctx);
        case NODE_FUNC_DEF:      return validate_func_def_node((FuncDefNode*)node, ctx);
        case NODE_BLOCK:         return validate_block_node((BlockNode*)node, ctx);
        case NODE_VAR_DECL:      return validate_var_decl_node((VarDeclNode*)node, ctx);
        case NODE_IDENTIFIER:    return validate_identifier_node((IdentifierNode*)node, ctx);
        case NODE_RETURN_STMT:   return validate_return_stmt_node((ReturnStmtNode*)node, ctx);
        case NODE_UNARY_OP:      return validate_unary_op_node((UnaryOpNode*)node, ctx);
        case NODE_BINARY_OP:     return validate_binary_op_node((BinaryOpNode*)node, ctx);
        case NODE_INT_LITERAL:   return validate_int_literal_node((IntLiteralNode*)node, ctx);
        case NODE_ASSIGNMENT_EXP: return validate_assignment_exp_node((AssignmentExpNode*)node, ctx);
        default:
            // Use validator_error for unhandled node types if considered an error
            // validator_error(ctx, node, "Unhandled AST node type: %d", node->type);
            // For now, treat as warning and continue validation if possible
            fprintf(stderr, "Warning: validate_node encountered unhandled AST node type: %d\n", node->type);
            return true; // Or false if unhandled nodes mean an error that stops validation
    }
}

// --- Implementations for specific node types --- 

static bool validate_program_node(ProgramNode* node, ValidatorContext* ctx) {
    // ProgramNode should contain one or more function definitions (or global declarations)
    // For now, assuming one function definition as per current grammar
    if (!validate_node((AstNode*)node->function, ctx)) return false;
    return !ctx->error_flag;
}

static bool validate_func_def_node(FuncDefNode* node, ValidatorContext* ctx) {
    // TODO: Add function symbol to a global function symbol table if needed
    // For now, functions don't have parameters or complex return types to validate here
    // but we'd check for redefinition, validate parameter types, etc.

    // Enter a new scope for the function body
    symbol_table_enter_scope(ctx->st);
    if (!validate_node((AstNode*)node->body, ctx)) {
        symbol_table_exit_scope(ctx->st); // Ensure scope is exited even on error
        return false;
    }
    symbol_table_exit_scope(ctx->st);
    return !ctx->error_flag;
}

static bool validate_block_node(BlockNode* node, ValidatorContext* ctx) {
    symbol_table_enter_scope(ctx->st);
    for (size_t i = 0; i < node->num_items; ++i) {
        if (!validate_node(node->items[i], ctx)) {
            symbol_table_exit_scope(ctx->st); // Ensure scope is exited even on error
            return false;
        }
        if (ctx->error_flag) { // Stop processing items in block if an error occurred
            symbol_table_exit_scope(ctx->st);
            return false;
        }
    }
    symbol_table_exit_scope(ctx->st);
    return !ctx->error_flag;
}

// Placeholder for generate_decorated_name - to be implemented next
static char* generate_decorated_name(ValidatorContext *ctx, const char *original_name, int id) {
    // Max length: original_name + '.' + 10 digits for id + null terminator
    // Ensure buffer is large enough or use dynamic allocation with arena
    char buffer[256]; // Adjust size as needed, or use arena_sprintf
    int written = snprintf(buffer, sizeof(buffer), "%s.%d", original_name, id);
    if (written < 0 || written >= sizeof(buffer)) {
        // Handle snprintf error or truncation
        validator_error(ctx, NULL, "Failed to generate decorated name for %s", original_name);
        return NULL; // Or a default error name
    }
    return arena_strdup(ctx->arena, buffer);
}

static bool validate_var_decl_node(VarDeclNode* node, ValidatorContext* ctx) {
    // 1. Check for type validity (e.g., is "int" a known type?)
    // For now, only "int" is supported, so this is trivial.
    if (strcmp(node->type_name, "int") != 0) {
        validator_error(ctx, (AstNode*)node, "Unknown type '%s' for variable '%s'.", node->type_name, node->var_name);
        return false;
    }

    // 2. Generate TAC ID and decorated name
    ctx->tac_temp_id_counter++;
    int current_tac_id = ctx->tac_temp_id_counter;
    char* decorated_name = generate_decorated_name(ctx, node->var_name, current_tac_id);
    if (!decorated_name) {
        // Error already set by generate_decorated_name
        return false;
    }

    // 3. Add to symbol table
    Symbol* existing_symbol = symbol_table_lookup_symbol_in_current_scope(ctx->st, node->var_name);
    if (existing_symbol) {
        validator_error(ctx, (AstNode*)node, "Variable '%s' redeclared in the same scope.", node->var_name);
        // decorated_name was arena_alloc'd, will be cleaned up with arena, no explicit free needed
        return false;
    }

    if (!symbol_table_add_symbol(ctx->st, node->var_name, node->declaration_token, current_tac_id, decorated_name)) {
        // This typically means an allocation error within symbol_table_add_symbol
        validator_error(ctx, (AstNode*)node, "Failed to add variable '%s' to symbol table.", node->var_name);
        return false;
    }

    // 4. Annotate AST node
    node->tac_temp_id = current_tac_id;
    node->tac_name_hint = decorated_name; // This is arena allocated

    // 5. Validate initializer if it exists
    if (node->initializer) {
        if (!validate_node(node->initializer, ctx)) return false;
        // TODO: Type check: initializer type must be assignable to variable type
    }
    return !ctx->error_flag;
}

static bool validate_identifier_node(IdentifierNode* node, ValidatorContext* ctx) {
    Symbol* symbol = symbol_table_lookup_symbol(ctx->st, node->name);
    if (!symbol) {
        validator_error(ctx, (AstNode*)node, "Undeclared identifier '%s'.", node->name);
        return false;
    }

    // Annotate AST node with info from symbol table for TAC generation
    node->tac_temp_id = symbol->tac_temp_id;
    node->tac_name_hint = symbol->decorated_name; // This is already arena allocated
    
    // TODO: Could also store a direct pointer to the Symbol in IdentifierNode if useful
    // node->symbol = symbol;
    return !ctx->error_flag;
}

static bool validate_return_stmt_node(ReturnStmtNode* node, ValidatorContext* ctx) {
    if (node->expression) {
        if (!validate_node(node->expression, ctx)) return false;
        // TODO: Type check: return expression type must match function's declared return type.
        // This requires knowing the current function's return type, which should be in ValidatorContext.
    }
    // TODO: If function expects a non-void return and expression is NULL, it's an error.
    // TODO: If function is void and expression is not NULL, it's an error (unless it's a void expression).
    return !ctx->error_flag;
}

static bool validate_unary_op_node(UnaryOpNode* node, ValidatorContext* ctx) {
    if (!validate_node(node->operand, ctx)) return false;
    // TODO: Type check: operand must be of a type compatible with the unary operator.
    // E.g., for '!', operand should be convertible to boolean (int in C).
    // For '-', '~', operand should be integer.
    // The result type of the expression also needs to be determined.
    return !ctx->error_flag;
}

static bool validate_binary_op_node(BinaryOpNode* node, ValidatorContext* ctx) {
    if (!validate_node(node->left, ctx)) return false;
    if (!validate_node(node->right, ctx)) return false;
    // TODO: Type check: left and right operands must be compatible with the binary operator.
    // E.g., for arithmetic ops, both should be integers.
    // For logical ops, both should be convertible to boolean (int).
    // The result type of the expression also needs to be determined.
    return !ctx->error_flag;
}

static bool validate_int_literal_node(IntLiteralNode* node, ValidatorContext* ctx) {
    // Integer literals are always valid by themselves.
    // Type is 'int'.
    (void)node; // Unused parameter
    (void)ctx;  // Unused parameter
    return true;
}

static bool validate_assignment_exp_node(AssignmentExpNode* node, ValidatorContext* ctx) {
    if (!node) {
        validator_error(ctx, NULL, "Assignment node itself is NULL.");
        return false;
    }
    if (!node->target) {
        validator_error(ctx, (AstNode*)node, "Assignment node target is NULL.");
        return false;
    }

    NodeType target_type = node->target->type;
    bool is_target_identifier = (target_type == NODE_IDENTIFIER);

    if (!is_target_identifier) { // This is equivalent to target_type != NODE_IDENTIFIER
        validator_error(ctx, node->target, "Invalid left-hand side in assignment expression. Expected an identifier.");
        return false;
    }

    // 1. Validate the target (which we now assume is an IdentifierNode)
    if (!validate_node(node->target, ctx)) return false; // Validates the identifier itself (e.g., declared?)

    // 2. Validate the value (right-hand side)
    if (!validate_node(node->value, ctx)) return false;

    // 3. TODO: Type Checking
    // The type of 'value' must be assignable to the type of 'target'.
    // This requires knowing the types of both expressions.
    // For now, assume types match if both sides are valid.

    // 4. Annotate AssignmentExpNode for TAC if needed (e.g. with target's TAC info)
    // The IdentifierNode target will already be annotated by its validation.
    // node->tac_temp_id = ((IdentifierNode*)node->target)->tac_temp_id;
    // node->tac_name_hint = ((IdentifierNode*)node->target)->tac_name_hint;

    return !ctx->error_flag;
}
