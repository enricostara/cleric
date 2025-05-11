#include "parser.h"
#include "ast.h"
#include "memory/arena.h" // Include Arena for allocation
#include <stdio.h>
#include <stdlib.h> // For strtol
#include <stdarg.h> // For va_list in parser_error
#include <errno.h>  // For errno and ERANGE
#include <limits.h>
#include <string.h> // For strlen, strcpy

// --- Precedence Climbing: Operator Properties ---
#define LOWEST_BINARY_PRECEDENCE 1
#define UNARY_OPERATOR_PRECEDENCE 3 // Higher than any binary operator we have

// Returns precedence level (0 if not a relevant binary operator)
static int get_token_precedence(TokenType type) {
    switch (type) {
        case TOKEN_SYMBOL_PLUS:
        case TOKEN_SYMBOL_MINUS: // Binary minus
            return 1;
        case TOKEN_SYMBOL_STAR:
        case TOKEN_SYMBOL_SLASH:
        case TOKEN_SYMBOL_PERCENT:
            return 2;
        default:
            return 0; // Not a binary operator we handle with precedence climbing here
    }
}

static BinaryOperatorType token_to_binary_operator_type(TokenType type) {
    switch (type) {
        case TOKEN_SYMBOL_PLUS:
            return OPERATOR_ADD;
        case TOKEN_SYMBOL_MINUS: // Binary minus
            return OPERATOR_SUBTRACT;
        case TOKEN_SYMBOL_STAR:
            return OPERATOR_MULTIPLY;
        case TOKEN_SYMBOL_SLASH:
            return OPERATOR_DIVIDE;
        case TOKEN_SYMBOL_PERCENT:
            return OPERATOR_MODULO;
        default:
            // Should not happen if called correctly
            fprintf(stderr, "Parser Internal Error: Invalid token type for binary operator conversion: %d\n", type);
            // Potentially set an error flag on parser or return a specific error type if AST allows
            return (BinaryOperatorType)-1; // Indicate error
    }
}

// --- End Precedence Climbing Helpers ---

// Forward declarations for static helper functions (parsing rules)
static FuncDefNode *parse_function_definition(Parser *parser);
static AstNode *parse_statement(Parser *parser);
static ReturnStmtNode *parse_return_statement(Parser *parser);
static AstNode *parse_expression(Parser *parser);
static AstNode *parse_primary_expression(Parser *parser);
static AstNode *parse_expression_recursive(Parser *parser, int min_precedence);

// Helper function to consume a token and advance
static bool parser_consume(Parser *parser, TokenType expected_type);

// Helper function to report errors
static void parser_error(Parser *parser, const char *format, ...);

// Helper function to advance the parser state
static void parser_advance(Parser *parser);

// --- Public Parser Interface Implementation ---
void parser_init(Parser *parser, Lexer *lexer, Arena *arena)
{
    parser->lexer = lexer;
    parser->error_flag = false;
    parser->error_message = NULL; // Initialize error_message
    parser->arena = arena; // Store the arena pointer
    // Prime the parser: Fetch the first two tokens.
    // Use the provided arena for lexeme allocation.
    const bool success1 = lexer_next_token(parser->lexer, &parser->current_token);
    const bool success2 = lexer_next_token(parser->lexer, &parser->peek_token);

    if (!success1 || !success2) {
        // If lexing failed (e.g., arena alloc error), set error flag
        // No need for specific error message here as lexer prints one.
        parser->error_flag = true;
        // Set tokens to EOF to prevent further parsing attempts
        parser->current_token.type = TOKEN_EOF;
        parser->peek_token.type = TOKEN_EOF;
        return;
    }

    // Check for immediate errors (e.g., UNKNOWN token at start)
    if (parser->current_token.type == TOKEN_UNKNOWN) {
        parser_error(parser, "Syntax Error: Unrecognized token at start");
    }
    // Note: parser_advance already checks peek_token for UNKNOWN
}

ProgramNode *parse_program(Parser *parser) {
    // Parse the function definition
    FuncDefNode *func_def_node = parse_function_definition(parser);
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
    ProgramNode *program_node = create_program_node(func_def_node, parser->arena);
    return program_node;
}

// --- Static Helper Function Implementation ---
static void parser_advance(Parser *parser) {
    // Important: Only advance if not already at EOF to avoid issues
    if (parser->current_token.type == TOKEN_EOF) {
        return; // Already at the end, don't advance further
    }

    // No need to free current_token's lexeme; it's in the arena.
    parser->current_token = parser->peek_token;
    // Pass the arena to lexer_next_token
    const bool success = lexer_next_token(parser->lexer, &parser->peek_token);
    if (!success) {
        // Lexer failed (e.g., arena error), set error flag and stop
        parser->error_flag = true;
        parser->peek_token.type = TOKEN_EOF; // Prevent further issues
        return;
    }

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
    if (parser->error_flag) return; // Only report the first error

    parser->error_flag = true;
    // Static fallback message in case arena allocation fails for the detailed message.
    static const char *ALLOC_FAILURE_MSG = "Parser Internal Error: Could not allocate memory for error message.";
    // Also, a fallback for general formatting issues.
    static const char *FORMATTING_FAILURE_MSG = "Parser Internal Error: Failed to format error message.";

    char temp_buffer[1024]; // Assuming errors won't exceed this
    int prefix_len = snprintf(temp_buffer, sizeof(temp_buffer),
                              "Parse Error (near pos %zu): ", parser->current_token.position);

    if (prefix_len < 0 || (size_t)prefix_len >= sizeof(temp_buffer)) {
        // snprintf error for prefix or buffer too small (highly unlikely for prefix alone)
        parser->error_message = (char *)FORMATTING_FAILURE_MSG;
        return;
    }

    va_list args;
    va_start(args, format);
    int msg_len = vsnprintf(temp_buffer + prefix_len, sizeof(temp_buffer) - prefix_len,
                            format, args);
    va_end(args);

    if (msg_len < 0) {
        // vsnprintf error for the main message part
        parser->error_message = (char *)FORMATTING_FAILURE_MSG;
        return;
    }

    // Total length for arena allocation
    size_t total_len = (size_t)prefix_len + (size_t)msg_len;

    // Allocate memory from the arena for the complete message and copy it
    char *allocated_message = arena_alloc(parser->arena, total_len + 1); // +1 for null terminator
    if (allocated_message) {
        strcpy(allocated_message, temp_buffer);
        parser->error_message = allocated_message;
    } else {
        // Arena allocation failed
        parser->error_message = (char *)ALLOC_FAILURE_MSG;
    }
}

// --- Recursive Descent Parsing Functions ---

// FunctionDefinition: 'int' Identifier '(' 'void' ')' '{' Statement '}'
static FuncDefNode *parse_function_definition(Parser *parser) {
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
    char *func_name = arena_alloc(parser->arena, name_len + 1);
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

    AstNode *body_statement = NULL; // Initialize to NULL, representing an empty body.

    // If the next token is not '}', then we try to parse a statement.
    // This allows for an empty body {}
    if (parser->current_token.type != TOKEN_SYMBOL_RBRACE) {
        body_statement = parse_statement(parser);
        if (!body_statement) {
            // parse_statement failed (and should have set the error flag).
            return NULL;
        }
    }
    // Now, body_statement is either NULL (for an empty body) or a parsed statement.

    // Expect '}' to close the function body
    if (!parser_consume(parser, TOKEN_SYMBOL_RBRACE)) {
        // Error handled by parser_consume.
        return NULL; // Error handled and flag set inside parser_consume
    }

    // Create the function definition node using the name from the arena
    // create_func_def_node and FuncDefNode must be able to handle body_statement being NULL.
    FuncDefNode *func_node = create_func_def_node(func_name, body_statement, parser->arena);
    // No need to free func_name; it was allocated in the arena and create_func_def_node
    // now uses the pointer directly (or copies into the arena if it didn't before).
    return func_node;
}

// Statement: ReturnStatement | ... (more statement types later)
static AstNode *parse_statement(Parser *parser) {
    if (parser->error_flag) return NULL;

    switch (parser->current_token.type) {
        case TOKEN_KEYWORD_RETURN:
            return (AstNode *) parse_return_statement(parser);
        default: {
            char current_token_str[128];
            token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
            parser_error(parser, "Expected statement (e.g., 'return'), but got %s", current_token_str);
            return NULL;
        }
    }
}

// ReturnStatement: 'return' Expression ';'
static ReturnStmtNode *parse_return_statement(Parser *parser) {
    // ReSharper disable once CppDFAConstantConditions
    // ReSharper disable once CppDFAUnreachableCode
    if (parser->error_flag) return NULL;

    // Consume 'return' keyword
    if (!parser_consume(parser, TOKEN_KEYWORD_RETURN)) {
        return NULL; // Error handled inside consume
    }

    // Parse the expression to be returned
    AstNode *expression = parse_expression(parser);
    if (!expression) {
        // Error flag should be set by the expression parser if something went wrong
        return NULL;
    }

    // Expect ';' after the expression
    if (!parser_consume(parser, TOKEN_SYMBOL_SEMICOLON)) {
        // Error handled in parser_consume
        return NULL;
    }

    // Create the return statement node
    ReturnStmtNode *return_node = create_return_stmt_node(expression, parser->arena);
    if (!return_node) {
        parser_error(parser, "Memory allocation failed for return statement node");
        return NULL;
    }
    return return_node;
}

// Expression parsing: Handles Integers, Unary Ops (- ~), and Parentheses
// <exp> ::= <int> | <unop> <exp> | "(" <exp> ")"
// <unop> ::= "-" | "~"
static AstNode *parse_expression(Parser *parser) {
    if (parser->error_flag) return NULL;
    return parse_expression_recursive(parser, LOWEST_BINARY_PRECEDENCE);
}

static AstNode *parse_primary_expression(Parser *parser) { // NOLINT(*-no-recursion) // Retaining for now, can remove if linter is fine
    if (parser->error_flag) return NULL;

    // Handle Unary Operators
    if (parser->current_token.type == TOKEN_SYMBOL_MINUS || parser->current_token.type == TOKEN_SYMBOL_TILDE) {
        UnaryOperatorType un_op_type;
        if (parser->current_token.type == TOKEN_SYMBOL_MINUS) {
            un_op_type = OPERATOR_NEGATE;
        } else { // TOKEN_SYMBOL_TILDE
            un_op_type = OPERATOR_COMPLEMENT;
        }
        parser_advance(parser); // Consume the operator ('-' or '~')
        if (parser->error_flag) return NULL; // Check if advance caused an error

        // This ensures that -a * b is parsed as (-a) * b
        AstNode *operand_node = parse_expression_recursive(parser, UNARY_OPERATOR_PRECEDENCE); // Recursively parse the operand with high precedence
        if (!operand_node) {
            // Error already reported by recursive call or advance
            return NULL;
        }

        // Create the UnaryOpNode
        UnaryOpNode* unary_node = create_unary_op_node(un_op_type, operand_node, parser->arena);
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
        IntLiteralNode* node = create_int_literal_node(value, parser->arena);
        if (!node) {
            parser_error(parser, "Memory allocation failed for integer literal node");
            return NULL;
        }
        parser_advance(parser); // Consume the constant token
        return (AstNode*)node;
    }

    // Handle Parenthesized Expressions
    if (parser->current_token.type == TOKEN_SYMBOL_LPAREN) {
        parser_advance(parser); // Consume '('
        if (parser->error_flag) return NULL;

        AstNode *inner_exp_node = parse_expression(parser); // Recursively parse the inner expression
        if (!inner_exp_node) {
            // Error already reported
            return NULL;
        }

        // Consume ')'
        if (!parser_consume(parser, TOKEN_SYMBOL_RPAREN)) {
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

static AstNode *parse_expression_recursive(Parser *parser, int min_precedence) {
    if (parser->error_flag) return NULL;
    AstNode *left_node = parse_primary_expression(parser);
    if (!left_node) {
        // Error already reported
        return NULL;
    }

    while (true) { // Loop while there are operators with sufficient precedence
        // Peek at the current token to decide if it's an operator we should handle
        TokenType current_op_token_type = parser->current_token.type;
        int current_op_actual_precedence = get_token_precedence(current_op_token_type);

        // If token is not an operator or its precedence is less than min_precedence, stop.
        if (current_op_actual_precedence == 0 || current_op_actual_precedence < min_precedence) {
            break;
        }

        // We have an operator to process
        BinaryOperatorType bin_op_enum = token_to_binary_operator_type(current_op_token_type);
        if (bin_op_enum == (BinaryOperatorType)-1) { // Error from conversion
            // token_to_binary_operator_type should have set an error or printed one.
            // If not, we might need to set parser->error_flag here.
            // For now, assume it handles its own error reporting if type is unexpected.
            parser_error(parser, "Internal parser error: Unexpected token for binary operator.");
            return NULL;
        }

        parser_advance(parser); // Consume the operator
        if (parser->error_flag) return NULL;

        // For left-associative operators, the next minimum precedence for the RHS is current_op_precedence + 1
        // For right-associative (not used here yet), it would be current_op_precedence
        // This ensures expressions like a - b - c are grouped as (a - b) - c
        const int next_min_precedence_for_rhs = current_op_actual_precedence + 1;
        AstNode *right_node = parse_expression_recursive(parser, next_min_precedence_for_rhs);
        if (!right_node) {
            // Error already reported
            return NULL;
        }

        // Create the BinaryOpNode
        BinaryOpNode* binary_node = create_binary_op_node(bin_op_enum, left_node, right_node, parser->arena);
        if (!binary_node) {
             parser_error(parser, "Memory allocation failed for binary operator node");
             return NULL;
        }
        left_node = (AstNode*)binary_node; // The result becomes the new left_node for the next iteration
    }

    return left_node;
}
