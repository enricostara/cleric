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

static AstNode *parse_expression(Parser *parser, Arena *arena);

static AstNode *parse_primary_expression(Parser *parser, Arena *arena);

// Helper function to consume a token and advance
static bool parser_consume(Parser *parser, TokenType expected_type);

// Helper function to report errors
static void parser_error(Parser *parser, const char *format, ...);

// Helper function to advance the parser state
static void parser_advance(Parser *parser);

// --- Public Parser Interface Implementation ---
void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->error_flag = false;
    // Initialize current_token and peek_token safely
    // Fetch first token into peek_token
    parser->peek_token = lexer_next_token(parser->lexer);
    // Initialize current_token to a known safe state (e.g., EOF or UNKNOWN with NULL lexeme)
    // Using EOF is reasonable as it won't be freed by token_free
    parser->current_token = (Token){TOKEN_EOF, NULL, 0};

    // Now perform the first real advance to load current_token and the next peek_token
    parser_advance(parser);
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
    // Free any remaining tokens if necessary (e.g., if parsing stopped mid-way)
    token_free(&parser->current_token);
    token_free(&parser->peek_token);
    // No other dynamically allocated memory in the Parser struct itself yet
}

// --- Static Helper Function Implementation ---
static void parser_advance(Parser *parser) {
    token_free(&parser->current_token); // Free the lexeme of the *previous* current_token
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(parser->lexer);

    // Check if the lexer encountered an unknown token
    if (parser->peek_token.type == TOKEN_UNKNOWN && !parser->error_flag) {
        char peek_token_str[128];
        token_to_string(parser->peek_token, peek_token_str, sizeof(peek_token_str));
        parser_error(parser, "Syntax Error: Unrecognized token %s", peek_token_str);
    }
}

static bool parser_consume(Parser *parser, const TokenType expected_type) {
    if (parser->current_token.type == expected_type) {
        parser_advance(parser);
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
    if (!parser_consume(parser, TOKEN_KEYWORD_INT)) {
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

    parser_advance(parser); // Consume identifier
    // ReSharper disable once CppDFAConstantConditions
    // ReSharper disable once CppDFAUnreachableCode
    if (parser->error_flag) return NULL; // Check error after explicit advance

    // Expect '('
    if (!parser_consume(parser, TOKEN_SYMBOL_LPAREN)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Simplified: Assume 'void' parameter for now
    if (!parser_consume(parser, TOKEN_KEYWORD_VOID)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Expect ')'
    if (!parser_consume(parser, TOKEN_SYMBOL_RPAREN)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Expect '{'
    if (!parser_consume(parser, TOKEN_SYMBOL_LBRACE)) {
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
    if (!parser_consume(parser, TOKEN_SYMBOL_RBRACE)) {
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
    if (!parser_consume(parser, TOKEN_KEYWORD_RETURN)) {
        return NULL; // Error handled inside consume
    }

    // Parse the expression to be returned
    AstNode *expression = parse_expression(parser, arena);
    if (!expression) {
        // Error occurred during expression parsing
        return NULL;
    }

    // Expect ';' after the expression
    if (!parser_consume(parser, TOKEN_SYMBOL_SEMICOLON)) {
        return NULL;
    }

    // Create the return statement node
    return create_return_stmt_node(expression, arena); // Pass arena
}

// Expression: PrimaryExpression
static AstNode *parse_expression(Parser *parser, Arena *arena) {
    if (parser->error_flag) return NULL;
    // For now, our expression grammar is very simple: only primary expressions
    return parse_primary_expression(parser, arena);
}

// PrimaryExpression: IntegerConstant | Identifier | '(' Expression ')'
static AstNode *parse_primary_expression(Parser *parser, Arena *arena) {
    // ReSharper disable once CppDFAConstantConditions
    // ReSharper disable once CppDFAUnreachableCode
    if (parser->error_flag) return NULL;

    switch (parser->current_token.type) {
        case TOKEN_CONSTANT: {
            errno = 0; // Reset errno before calling strtol
            char *end_ptr;
            const long val = strtol(parser->current_token.lexeme, &end_ptr, 10);

            // Check for conversion errors
            if (errno == ERANGE || *end_ptr != '\0') {
                parser_error(parser, "Invalid integer literal: %s", parser->current_token.lexeme);
                return NULL;
            }
            // Check if the value fits within an int
            if (val < INT_MIN || val > INT_MAX) {
                parser_error(parser, "Integer literal out of range: %s", parser->current_token.lexeme);
                return NULL;
            }

            const int value = (int) val;
            parser_advance(parser); // Consume the integer token
            // ReSharper disable once CppDFAConstantConditions
            // ReSharper disable once CppDFAUnreachableCode
            if (parser->error_flag) return NULL; // Check error after explicit advance
            return (AstNode *) create_int_literal_node(value, arena); // Pass arena
        }
        // Add cases for Identifier, Parenthesized Expression, etc. later
        default: {
            char current_token_str[128];
            token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
            parser_error(parser, "Expected expression (e.g., integer constant), but got %s", current_token_str);
            return NULL;
        }
    }
}
