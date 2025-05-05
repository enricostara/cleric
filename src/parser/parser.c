#include "parser.h"
#include "ast.h"
#include "memory/arena.h" // Include Arena for allocation
#include <stdio.h>
#include <stdlib.h> // For strtol
#include <stdarg.h> // For va_list in parser_error
#include <errno.h>  // For errno and ERANGE
#include <limits.h>
#include <string.h> // For strlen, strcpy

// Forward declarations for static helper functions (parsing rules)
static FuncDefNode *parse_function_definition(Parser *parser, Arena *arena);

static AstNode *parse_statement(Parser *parser, Arena *arena);

static ReturnStmtNode *parse_return_statement(Parser *parser, Arena *arena);

static AstNode *parse_exp(Parser *parser, Arena *arena); // Renamed from parse_primary_expression

// Helper function to consume a token and advance
static bool parser_consume(Parser *parser, Arena *arena, TokenType expected_type);

// Helper function to report errors
static void parser_error(Parser *parser, const char *format, ...);

// Helper function to advance the parser state
static void parser_advance(Parser *parser, Arena *arena);

// --- Public Parser Interface Implementation ---
void parser_init(Parser *parser,  Lexer *lexer, Arena *arena)
{
    parser->lexer = lexer;
    parser->error_flag = false;
    // Prime the parser: Fetch the first two tokens.
    // Use the provided arena for lexeme allocation.
    parser->current_token = lexer_next_token(parser->lexer, arena);
    parser->peek_token = lexer_next_token(parser->lexer, arena);

    // Check for immediate errors (e.g., UNKNOWN token at start)
    if (parser->current_token.type == TOKEN_UNKNOWN) {
        parser_error(parser, "Syntax Error: Unrecognized token at start");
    }
    // Note: parser_advance already checks peek_token for UNKNOWN
}

ProgramNode *parse_program(Parser *parser, Arena *arena) {
    // Parse the function definition
    FuncDefNode *func_def_node = parse_function_definition(parser, arena);
    if (parser->error_flag || !func_def_node) {
        // No need to free_ast; arena handles cleanup
        return NULL;
    }

    // Expect end of file
    if (parser->current_token.type != TOKEN_EOF) {
        char current_token_str[128];
        token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
        parser_error(parser, "Expected end of file after function definition, but got %s", current_token_str);
        // No need to free_ast; arena handles cleanup
        return NULL;
    }

    // Create a ProgramNode from the FuncDefNode
    ProgramNode *program_node = create_program_node(func_def_node, arena);
    return program_node;
}

void parser_destroy(const Parser *parser) {
    // No explicit freeing needed for tokens as lexemes are in the arena.
    // The arena itself is managed externally (e.g., by the compiler).
}

// --- Static Helper Function Implementation ---
static void parser_advance(Parser *parser, Arena *arena) {
    // Important: Only advance if not already at EOF to avoid issues
    if (parser->current_token.type == TOKEN_EOF) {
        return; // Already at the end, don't advance further
    }

    // No need to free current_token's lexeme; it's in the arena.
    parser->current_token = parser->peek_token;
    // Pass the arena to lexer_next_token
    parser->peek_token = lexer_next_token(parser->lexer, arena);

    // Check if the lexer encountered an unknown token
    if (parser->peek_token.type == TOKEN_UNKNOWN && !parser->error_flag) {
        char peek_token_str[128];
        token_to_string(parser->peek_token, peek_token_str, sizeof(peek_token_str));
        parser_error(parser, "Syntax Error: Unrecognized token %s", peek_token_str);
    }
}

static bool parser_consume(Parser *parser, Arena *arena, TokenType expected_type) {
    if (parser->current_token.type == expected_type) {
        parser_advance(parser, arena);
        // Check if parser_advance itself encountered an error (like TOKEN_UNKNOWN)
        return !parser->error_flag; // Return true if no error occurred during advance
    }
    char current_token_str[128];
    token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));

    char expected_token_str[128];
    // Create a dummy token just for getting the string representation
    token_to_string((Token){expected_type, "expected", 0}, expected_token_str, sizeof(expected_token_str));

    parser_error(parser, "Expected token %s, but got %s", expected_token_str, current_token_str);
    return false; // Indicate failure
}

static void parser_error(Parser *parser, const char *format, ...) {
    if (!parser->error_flag) {
        // Report only the first error
        parser->error_flag = true;
        fprintf(stderr, "Parse Error (near pos %zu): ", parser->current_token.position);
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

// --- Recursive Descent Parsing Functions ---

// FunctionDefinition: 'int' Identifier '(' 'void' ')' '{' Statement '}'
static FuncDefNode *parse_function_definition(Parser *parser, Arena *arena) {
    if (parser->error_flag) return NULL; // Don't proceed if an error already occurred

    // Expect 'int' keyword
    if (!parser_consume(parser, arena, TOKEN_KEYWORD_INT)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Expect function name (identifier)
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        char current_token_str[128];
        token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
        parser_error(parser, "Expected function name (identifier) after 'int', but got %s", current_token_str);
        return NULL;
    }
    // Copy the lexeme into the arena
    const size_t name_len = strlen(parser->current_token.lexeme);
    char *func_name = arena_alloc(arena, name_len + 1);
    if (!func_name) {
        parser_error(parser, "Memory allocation failed for function name using arena");
        return NULL;
    }
    strcpy(func_name, parser->current_token.lexeme); // Copy the name

    parser_advance(parser, arena); // Consume identifier
    // ReSharper disable once CppDFAConstantConditions
    // ReSharper disable once CppDFAUnreachableCode
    if (parser->error_flag) return NULL; // Check error after explicit advance

    // Expect '('
    if (!parser_consume(parser, arena, TOKEN_SYMBOL_LPAREN)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Simplified: Assume 'void' parameter for now
    if (!parser_consume(parser, arena, TOKEN_KEYWORD_VOID)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Expect ')'
    if (!parser_consume(parser, arena, TOKEN_SYMBOL_RPAREN)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Expect '{'
    if (!parser_consume(parser, arena, TOKEN_SYMBOL_LBRACE)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Parse the statement block (simplified to one statement)
    AstNode *body_statement = parse_statement(parser, arena);
    if (!body_statement) {
        // Check if statement parsing failed (error flag should be set)
        // No need to free func_name, it's in the arena
        return NULL;
    }

    // Expect '}' after the statement
    if (!parser_consume(parser, arena, TOKEN_SYMBOL_RBRACE)) {
        // No need to free_ast or free func_name; arena handles cleanup
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Create the function definition node using the name from the arena
    FuncDefNode *func_node = create_func_def_node(func_name, body_statement, arena);
    // No need to free func_name; it was allocated in the arena and create_func_def_node
    // now uses the pointer directly (or copies into the arena if it didn't before).
    return func_node;
}

// Statement: ReturnStatement | ... (more statement types later)
static AstNode *parse_statement(Parser *parser, Arena *arena) {
    if (parser->error_flag) return NULL;

    switch (parser->current_token.type) {
        case TOKEN_KEYWORD_RETURN:
            return (AstNode *) parse_return_statement(parser, arena);
        default: {
            char current_token_str[128];
            token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
            parser_error(parser, "Expected statement (e.g., 'return'), but got %s", current_token_str);
            return NULL;
        }
    }
}

// ReturnStatement: 'return' Expression ';'
static ReturnStmtNode *parse_return_statement(Parser *parser, Arena *arena) {
    // ReSharper disable once CppDFAConstantConditions
    // ReSharper disable once CppDFAUnreachableCode
    if (parser->error_flag) return NULL;

    // Consume 'return' keyword
    if (!parser_consume(parser, arena, TOKEN_KEYWORD_RETURN)) {
        return NULL; // Error handled inside consume
    }

    // Parse the expression that follows using the new function
    AstNode *expression_node = parse_exp(parser, arena);
    if (!expression_node) {
        // Error should have been reported by parse_exp or earlier
        return NULL;
    }

    // Expect ';' after the expression
    if (!parser_consume(parser, arena, TOKEN_SYMBOL_SEMICOLON)) {
        // Error handled in parser_consume
        return NULL;
    }

    // Create the return statement node
    ReturnStmtNode *return_node = create_return_stmt_node(expression_node, arena);
    if (!return_node) {
        parser_error(parser, "Memory allocation failed for return statement node");
        return NULL;
    }
    return return_node;
}

// Expression parsing: Handles Integers, Unary Ops (- ~), and Parentheses
// <exp> ::= <int> | <unop> <exp> | "(" <exp> ")"
// <unop> ::= "-" | "~"
static AstNode *parse_exp(Parser *parser, Arena *arena) { // NOLINT(*-no-recursion)
    if (parser->error_flag) return NULL;

    // Handle Unary Operators
    if (parser->current_token.type == TOKEN_SYMBOL_MINUS || parser->current_token.type == TOKEN_SYMBOL_TILDE) {
        const UnaryOperatorType op_type = parser->current_token.type == TOKEN_SYMBOL_MINUS
                                      ? OPERATOR_NEGATE
                                      : OPERATOR_COMPLEMENT;

        parser_advance(parser, arena); // Consume the operator ('-' or '~')
        if (parser->error_flag) return NULL; // Check if advance caused an error

        AstNode *operand_node = parse_exp(parser, arena); // Recursively parse the operand
        if (!operand_node) {
            // Error already reported by recursive call or advance
            return NULL;
        }

        // Create the UnaryOpNode
        UnaryOpNode* unary_node = create_unary_op_node(op_type, operand_node, arena);
        if (!unary_node) {
             parser_error(parser, "Memory allocation failed for unary operator node");
             return NULL;
        }
        return (AstNode*)unary_node;
    }

    // Handle Integer Literals
    if (parser->current_token.type == TOKEN_CONSTANT) {
        char *end_ptr;
        errno = 0; // Reset errno before call
        const long val_long = strtol(parser->current_token.lexeme, &end_ptr, 10);

        // Error checking for strtol
        if (parser->current_token.lexeme == end_ptr) { // No digits found
            parser_error(parser, "Invalid integer literal format: %s", parser->current_token.lexeme);
            return NULL;
        }
        if (*end_ptr != '\0') { // Extra characters after number (should have been caught by lexer, but double-check)
            parser_error(parser, "Invalid characters after integer literal: %s", parser->current_token.lexeme);
            return NULL;
        }
        if (errno == ERANGE || val_long < INT_MIN || val_long > INT_MAX) {
            parser_error(parser, "Integer literal out of range: %s", parser->current_token.lexeme);
            return NULL;
        }

        const int value = (int) val_long;
        IntLiteralNode* node = create_int_literal_node(value, arena);
        if (!node) {
            parser_error(parser, "Memory allocation failed for integer literal node");
            return NULL;
        }
        parser_advance(parser, arena); // Consume the constant token
        return (AstNode*)node;
    }

    // Handle Parenthesized Expressions
    if (parser->current_token.type == TOKEN_SYMBOL_LPAREN) {
        parser_advance(parser, arena); // Consume '('
        if (parser->error_flag) return NULL;

        AstNode *inner_exp_node = parse_exp(parser, arena); // Recursively parse the inner expression
        if (!inner_exp_node) {
            // Error already reported
            return NULL;
        }

        // Consume ')'
        if (!parser_consume(parser, arena, TOKEN_SYMBOL_RPAREN)) {
            // Error reported by parser_consume
            return NULL;
        }
        return inner_exp_node; // Return the node from inside the parentheses
    }

    // Handle Error Case
    char current_token_str[128];
    token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
    parser_error(parser, "Expected expression (integer, unary op, or '('), but got %s", current_token_str);
    return NULL;
}
