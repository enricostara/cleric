#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h" // Need Lexer and Token types
#include "ast.h"           // Need AST node types (specifically ProgramNode)
#include "memory/arena.h" // Include arena header
#include <stdbool.h>       // For bool type

// Parser state structure
typedef struct {
    Lexer *lexer;           // Pointer to the lexer providing tokens
    Token current_token;    // The current token being processed
    Token peek_token;       // The next token (lookahead)
    bool error_flag;        // Flag to indicate if a syntax error occurred
    // Add more fields later if needed (e.g., symbol table, error messages buffer)
} Parser;

// --- Public Parser Interface ---

/**
 * @brief Initializes the parser state.
 *
 * Sets up the parser with the given lexer and fetches the first two tokens
 * (current and peek) to prime the parsing process.
 *
 * @param parser Pointer to the Parser struct to initialize.
 * @param lexer Pointer to the Lexer struct to use for tokenization.
 */
void parser_init(Parser *parser, Lexer *lexer);

/**
 * @brief Parses the entire token stream from the lexer using the provided arena.
 *
 * This is the main entry point for parsing. It attempts to parse the input
 * according to the language grammar and builds an Abstract Syntax Tree (AST)
 * allocating nodes from the provided arena.
 *
 * @param parser Pointer to the initialized Parser struct.
 * @param arena Pointer to the memory arena to use for AST node allocations.
 * @return ProgramNode* Pointer to the root of the generated AST (ProgramNode)
 *         if parsing is successful, otherwise NULL if a syntax error occurs
 *         or memory allocation fails. The memory is owned by the arena.
 */
ProgramNode *parse_program(Parser *parser, Arena *arena);

// Note: We don't declare parser_advance, parser_consume, or the specific
// parse_* grammar rule functions here because they are internal implementation
// details managed within parser.c.

#endif // PARSER_H
