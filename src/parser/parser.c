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
#define UNARY_OPERATOR_PRECEDENCE 7 // Adjusted: Higher than any binary operator

// Returns precedence level (0 if not a relevant binary operator)
static int get_token_precedence(TokenType type) {
    switch (type) {
        case TOKEN_SYMBOL_LOGICAL_OR:    // ||
            return 1;
        case TOKEN_SYMBOL_LOGICAL_AND:   // &&
            return 2;
        case TOKEN_SYMBOL_EQUAL_EQUAL:   // ==
        case TOKEN_SYMBOL_NOT_EQUAL:     // !=
            return 3;
        case TOKEN_SYMBOL_LESS:          // <
        case TOKEN_SYMBOL_GREATER:       // >
        case TOKEN_SYMBOL_LESS_EQUAL:    // <=
        case TOKEN_SYMBOL_GREATER_EQUAL: // >=
            return 4;
        case TOKEN_SYMBOL_PLUS:
        case TOKEN_SYMBOL_MINUS: // Binary minus
            return 5; // Adjusted from 1
        case TOKEN_SYMBOL_STAR:
        case TOKEN_SYMBOL_SLASH:
        case TOKEN_SYMBOL_PERCENT:
            return 6; // Adjusted from 2
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
        case TOKEN_SYMBOL_LOGICAL_OR:
            return OPERATOR_LOGICAL_OR;
        case TOKEN_SYMBOL_LOGICAL_AND:
            return OPERATOR_LOGICAL_AND;
        case TOKEN_SYMBOL_EQUAL_EQUAL:
            return OPERATOR_EQUAL_EQUAL;
        case TOKEN_SYMBOL_NOT_EQUAL:
            return OPERATOR_NOT_EQUAL;
        case TOKEN_SYMBOL_LESS:
            return OPERATOR_LESS;
        case TOKEN_SYMBOL_GREATER:
            return OPERATOR_GREATER;
        case TOKEN_SYMBOL_LESS_EQUAL:
            return OPERATOR_LESS_EQUAL;
        case TOKEN_SYMBOL_GREATER_EQUAL:
            return OPERATOR_GREATER_EQUAL;
        default:
            // Should not happen if called correctly
            fprintf(stderr, "Parser Internal Error: Invalid token type for binary operator conversion: %d\n", type);
            // Potentially set an error flag on parser or return a specific error type if AST allows
            return (BinaryOperatorType)-1; // Indicate error
    }
}

static bool token_to_unary_operator_type(TokenType token_type, UnaryOperatorType *op_type) {
    switch (token_type) {
        case TOKEN_SYMBOL_MINUS: *op_type = OPERATOR_NEGATE; return true;
        case TOKEN_SYMBOL_TILDE: *op_type = OPERATOR_COMPLEMENT; return true;
        case TOKEN_SYMBOL_BANG:  *op_type = OPERATOR_LOGICAL_NOT; return true;
        default: return false;
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
static BlockNode *parse_block(Parser *parser);
static AstNode *parse_declaration(Parser *parser); // Full implementation will be in parser.c

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

    // Handle parameters: either 'void' or empty '()'
    if (parser->current_token.type == TOKEN_KEYWORD_VOID) {
        parser_advance(parser); // Consume 'void'
        if (parser->error_flag) return NULL; // Check error after explicit advance
    } else if (parser->current_token.type == TOKEN_SYMBOL_RPAREN) {
        // This is an empty parameter list like main(), which is fine.
        // The next parser_consume for ')' will handle it.
    } else {
        // For now, any other token here is an error for a function without parameters.
        // Later, this is where actual parameter parsing (e.g. 'int a, int b') would go.
        char current_token_str[128];
        token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
        parser_error(parser, "Expected 'void' or ')' for function parameters, but got %s", current_token_str);
        return NULL;
    }

    // Expect ')'
    if (!parser_consume(parser, TOKEN_SYMBOL_RPAREN)) {
        return NULL; // Error handled and flag set inside parser_consume
    }

    // The function body is a block, parse it using parse_block()
    // parse_block() will handle consuming '{' and '}' and parsing declarations/statements.
    BlockNode *body_block = parse_block(parser);
    if (!body_block) {
        // parse_block should have set an error flag if it failed.
        // If not, it's an unexpected state, but we propagate the NULL anyway.
        if(!parser->error_flag && func_name) { // Add a more specific error if parse_block didn't
             parser_error(parser, "Failed to parse function body for '%s'.", func_name);
        }
        return NULL;
    }

    // Create the function definition node using the name from the arena
    FuncDefNode *func_node = create_func_def_node(func_name, body_block, parser->arena);
    // No need to free func_name; it was allocated in the arena and create_func_def_node
    // now uses the pointer directly (or copies into the arena if it didn't before).
    return func_node;
}

// Statement: ReturnStatement | Declaration | ... (more statement types later)
static AstNode *parse_statement(Parser *parser) {
    if (parser->error_flag) return NULL;

    AstNode *stmt_node = NULL;

    switch (parser->current_token.type) {
        case TOKEN_KEYWORD_RETURN:
            stmt_node = (AstNode *)parse_return_statement(parser);
            break;
        case TOKEN_SYMBOL_LBRACE: // Compound statement (nested block)
            // Note: parse_block itself returns a BlockNode*, which is an AstNode*.
            // It handles consuming '{' and '}'.
            stmt_node = (AstNode *)parse_block(parser);
            break;
        case TOKEN_SYMBOL_SEMICOLON: // Empty statement
            parser_advance(parser); // Consume ';'
            // Empty statements don't produce an AST node to be added to a block.
            // parse_block will see a NULL item and skip adding it.
            stmt_node = NULL; 
            break;
        default:
            // Attempt to parse as an expression statement.
            // An expression statement is an expression followed by a semicolon.
            stmt_node = parse_expression(parser);
            if (parser->error_flag) { // Error during expression parsing
                return NULL;
            }

            if (stmt_node) { // If an expression was parsed
                if (!parser_consume(parser, TOKEN_SYMBOL_SEMICOLON)) {
                    // Expected semicolon after expression statement.
                    // parser_consume sets the error. stmt_node from arena, no manual free needed.
                    return NULL; 
                }
                // The expression node itself (stmt_node) serves as the AST representation
                // for the expression statement.
            } else {
                // parse_expression returned NULL without setting error_flag.
                // This means the current token doesn't start any known expression.
                // So, it's not a return, block, empty, or expression statement.
                char current_token_str[128];
                token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
                parser_error(parser, "Expected statement (e.g., 'return', expression, '{', ';'), but got %s.", current_token_str);
                return NULL;
            }
            break;
    }

    // If error_flag was set by a sub-parser (e.g. parse_return_statement, parse_block, parse_expression)
    if (parser->error_flag) {
        // stmt_node might be partially formed but is from arena, so no leak.
        return NULL;
    }

    return stmt_node; // This can be NULL for empty statements, or a valid AST node.
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

    // Handle Unary Operators: '-', '~', '!'
    UnaryOperatorType un_op_type;
    if (token_to_unary_operator_type(parser->current_token.type, &un_op_type)) {
        parser_advance(parser); // Consume the unary operator token
        if (parser->error_flag) return NULL;

        // Recursively parse the operand with unary precedence
        AstNode *operand = parse_expression_recursive(parser, UNARY_OPERATOR_PRECEDENCE);
        if (parser->error_flag || !operand) {
            if (!parser->error_flag) {
                 // If operand is NULL but no error flag, it means an unexpected EOF or missing operand after unary op.
                 char current_token_str[128];
                 token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
                 parser_error(parser, "Syntax Error: Expected expression after unary operator, but got %s", current_token_str);
            }
            return NULL; // Error already set or operand missing
        }
        return (AstNode *)create_unary_op_node(un_op_type, operand, parser->arena);
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
    if (!left_node || parser->error_flag) { // Check error_flag after parse_primary_expression
        // Error already reported by parse_primary_expression or child call
        return NULL;
    }

    while (true) {
        // Check the CURRENT token as the operator.
        int op_precedence = get_token_precedence(parser->current_token.type);

        if (op_precedence < min_precedence) {
            // Not an operator, or operator of lower precedence than we are looking for.
            break;
        }

        // current_token is our operator.
        Token operator_token_details = parser->current_token;
        BinaryOperatorType op_type = token_to_binary_operator_type(operator_token_details.type);
        
        if ((int)op_type == -1 && !parser->error_flag) { // Check if conversion failed
            char current_token_str[128];
            token_to_string(parser->current_token, current_token_str, sizeof(current_token_str));
            parser_error(parser, "Internal Error: Unexpected token %s for binary operator in parse_expression_recursive", current_token_str);
            return NULL;
        }
        if(parser->error_flag) return NULL; // If op_type conversion itself failed and set error

        parser_advance(parser); // Consume the operator. current_token is now the start of the RHS.
        if (parser->error_flag) return NULL; // Check error after advance

        // Parse the right-hand side. For left-associativity, recursive call needs higher precedence.
        AstNode *right_node = parse_expression_recursive(parser, op_precedence + 1);
        if (!right_node) { 
            // Error should be set by the recursive call or primary_expression if !right_node
            // If no specific error was set by a deeper call, provide a generic one here.
            if (!parser->error_flag) {
                 char op_str[128];
                 token_to_string(operator_token_details, op_str, sizeof(op_str));
                 char next_tok_str[128];
                 token_to_string(parser->current_token, next_tok_str, sizeof(next_tok_str));
                 parser_error(parser, "Syntax Error: Expected expression after operator %s, but got %s", op_str, next_tok_str);
            }
            return NULL; 
        }
        // No need to check error_flag again if right_node is valid, as prior calls would return NULL on error.

        left_node = (AstNode *) create_binary_op_node(op_type, left_node, right_node, parser->arena);
        if (!left_node) {
            // Allocation failed
            if (!parser->error_flag) { // Should be rare if arena is robust
                 parser_error(parser, "Failed to create binary operation node due to allocation failure.");
            }
            return NULL;
        }
        // Loop continues. current_token is now the token AFTER the parsed RHS.
    }
    return left_node;
}

// --- Implementation of Block and Declaration Parsing ---

// Parses a block of code: '{' (declaration | statement)* '}'
static BlockNode *parse_block(Parser *parser) {
    if (!parser_consume(parser, TOKEN_SYMBOL_LBRACE)) {
        // Error already reported by parser_consume
        return NULL;
    }

    BlockNode *block_node = create_block_node(parser->arena);
    if (!block_node) {
        if (!parser->error_flag) {
            parser_error(parser, "Memory allocation failed for block node.");
        }
        return NULL;
    }

    // Loop to parse declarations or statements until '}' or EOF
    while (parser->current_token.type != TOKEN_SYMBOL_RBRACE && 
           parser->current_token.type != TOKEN_EOF && 
           !parser->error_flag) {
        
        AstNode *item = NULL;
        // Check if the current token indicates the start of a declaration.
        // For now, we only check for 'int'. This can be expanded later.
        if (parser->current_token.type == TOKEN_KEYWORD_INT) {
            item = parse_declaration(parser);
        } else {
            // Otherwise, assume it's a statement.
            item = parse_statement(parser);
        }

        if (parser->error_flag) { // If parse_declaration or parse_statement encountered an error
            return NULL; // Error already set, propagate up.
        }
        if (item) { // If a declaration or statement was successfully parsed
            if (!block_node_add_item(block_node, item, parser->arena)) {
                if (!parser->error_flag) {
                    parser_error(parser, "Memory allocation failed when adding item to block node.");
                }
                return NULL; // Return NULL as block construction failed
            }
        } 
        // If item is NULL and error_flag is false (checked above),
        // it means a skippable construct like an empty statement was parsed.
        // The sub-parser (e.g., parse_statement) already advanced the token.
        // No further action is needed here; the loop will continue with the next token.
        // The previous 'else' block that generated an error here was incorrect.
    }

    // After the loop, check for errors one last time (e.g., if loop exited due to error_flag)
    if (parser->error_flag) return NULL;

    if (!parser_consume(parser, TOKEN_SYMBOL_RBRACE)) {
        // Error (expected '}') already reported by parser_consume
        return NULL;
    }

    return block_node;
}

// Parses a variable declaration, e.g., "int x;"
// For now, only supports "int" type.
static AstNode *parse_declaration(Parser *parser) {
    const char* type_str = NULL;

    // 1. Expect a type keyword (currently only TOKEN_KEYWORD_INT)
    if (parser->current_token.type == TOKEN_KEYWORD_INT) {
        type_str = "int"; 
    } else {
        // This function is called when a declaration is expected.
        // If it's not 'int', it's an error for this simplified version.
        // A more advanced parser might check for other type keywords here.
        char token_str_buf[128];
        token_to_string(parser->current_token, token_str_buf, sizeof(token_str_buf));
        parser_error(parser, "Expected type keyword (e.g., 'int') for declaration, got %s.", token_str_buf);
        return NULL;
    }

    parser_advance(parser); // Consume the type token (e.g., 'int')
    if (parser->error_flag) return NULL;

    // 2. Expect an identifier for the variable name
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        char token_str_buf[128];
        token_to_string(parser->current_token, token_str_buf, sizeof(token_str_buf));
        parser_error(parser, "Expected identifier for variable name, got %s.", token_str_buf);
        return NULL;
    }
    
    // The lexeme from the token is allocated in the arena, so it's safe to use.
    char *var_name = parser->current_token.lexeme;

    parser_advance(parser); // Consume the identifier token
    if (parser->error_flag) return NULL;

    // 3. Expect a semicolon
    if (!parser_consume(parser, TOKEN_SYMBOL_SEMICOLON)) {
        // parser_consume already sets an error if the token doesn't match
        // and reports "Expected ';' after variable declaration."
        // Ensure a more specific message if needed, or rely on parser_consume's default.
        if (!parser->error_flag) { // If parser_consume didn't set one for some reason (should not happen)
            parser_error(parser, "Expected ';' after variable declaration name '%s'.", var_name);
        }
        return NULL;
    }

    // 4. Create and return the VarDeclNode
    // type_str is already set (e.g., to "int")
    VarDeclNode *decl_node = create_var_decl_node(type_str, var_name, NULL, parser->arena); // Pass NULL for initializer
    if (!decl_node) {
        if (!parser->error_flag) { // If create_node failed and didn't set an error
            parser_error(parser, "Memory allocation failed for variable declaration node for '%s'.", var_name);
        }
        return NULL;
    }

    return (AstNode *)decl_node;
}
