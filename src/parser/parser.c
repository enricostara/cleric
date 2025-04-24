#include "parser.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> // For va_list in parser_error

// Forward declarations for static helper functions (parsing rules)
static AstNode *parse_function_definition(Parser *parser);
static AstNode *parse_statement(Parser *parser);
static AstNode *parse_return_statement(Parser *parser);
static AstNode *parse_expression(Parser *parser);
static AstNode *parse_primary_expression(Parser *parser);

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
    parser->current_token = (Token){TOKEN_UNKNOWN, NULL, 0};
    parser->peek_token = (Token){TOKEN_UNKNOWN, NULL, 0};
    parser_advance(parser);
    parser_advance(parser);
}

ProgramNode *parse_program(Parser *parser) {
    AstNode *func_def_node = parse_function_definition(parser);
    if (parser->error_flag || !func_def_node) {
        free_ast(func_def_node); // Clean up partial AST if error
        return NULL;
    }

    if (parser->current_token.type != TOKEN_EOF) {
        char current_token_str[128];
        token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
        parser_error(parser, "Expected end of file after function definition, but got %s", current_token_str);
        free_ast(func_def_node);
        return NULL;
    }

    ProgramNode *program_node = create_program_node((FuncDefNode *)func_def_node);
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
    if (!parser->error_flag) { // Report only the first error
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
static AstNode *parse_function_definition(Parser *parser) {
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
    // Ignore the function name for now, as the simple AST doesn't store it.
    parser_advance(parser); // Consume identifier - Note: parser_advance doesn't return status
    // ReSharper disable once CppDFAConstantConditions
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
    AstNode *body_statement = parse_statement(parser);
    if (!body_statement) { // Check if statement parsing failed (error flag should be set)
        return NULL;
    }

    // Expect '}'
    if (!parser_consume(parser, TOKEN_SYMBOL_RBRACE)) {
        free_ast(body_statement); // Clean up allocated statement node on error
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Use the creation function from ast.h, which only takes the body
    return create_func_def_node(body_statement); // Aligned with ast.h
}


// Statement: ReturnStatement | ... (more statement types later)
static AstNode *parse_statement(Parser *parser) {
    if (parser->error_flag) return NULL;

    switch (parser->current_token.type) {
        case TOKEN_KEYWORD_RETURN:
            return parse_return_statement(parser);
        default: {
            char current_token_str[128];
            token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
            parser_error(parser, "Expected statement (e.g., 'return'), but got %s", current_token_str);
            return NULL;
        }
    }
}

// ReturnStatement: 'return' Expression ';'
static AstNode *parse_return_statement(Parser *parser) {
    if (parser->error_flag) return NULL;

    parser_consume(parser, TOKEN_KEYWORD_RETURN); // Consume 'return'

    AstNode *expression = parse_expression(parser);
    if (parser->error_flag || !expression) {
        free_ast(expression); // Clean up potentially partial expression
        return NULL;
    }

    parser_consume(parser, TOKEN_SYMBOL_SEMICOLON);
    if (parser->error_flag) {
        free_ast(expression); // Clean up expression if semicolon is missing
        return NULL;
    }

    // Use the creation function from ast.h
    return create_return_stmt_node(expression); // Aligned with ast.h
}


// Expression: PrimaryExpression
static AstNode *parse_expression(Parser *parser) {
    if (parser->error_flag) return NULL;
    return parse_primary_expression(parser);
}


// PrimaryExpression: IntegerConstant | Identifier | '(' Expression ')'
static AstNode *parse_primary_expression(Parser *parser) {
    if (parser->error_flag) return NULL;

    switch (parser->current_token.type) {
        case TOKEN_CONSTANT: {
            // Need error checking for atoi? Maybe later. Consider strtol for robustness.
            long value = atol(parser->current_token.lexeme); // Use atol for potential future larger ints
            // TODO: Add error handling for conversion (e.g., using strtol)
            parser_advance(parser); // Consume the constant token
            // Use the creation function from ast.h
            return create_int_literal_node(value); // Aligned with ast.h
        }
        // Add cases for identifiers, parentheses, etc. later
        default: {
            char current_token_str[128];
            token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
            parser_error(parser, "Expected expression (e.g., integer constant), but got %s", current_token_str);
            return NULL;
        }
    }
}
