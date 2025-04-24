#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/**
 * Token types for a minimal C subset lexer.
 * Extend this enum as you add more C language features.
 */
typedef enum {
    TOKEN_IDENTIFIER, // e.g., main, variable names
    TOKEN_CONSTANT, // e.g., 2, 42
    TOKEN_KEYWORD_INT, // 'int'
    TOKEN_KEYWORD_VOID, // 'void'
    TOKEN_KEYWORD_RETURN, // 'return'
    TOKEN_SYMBOL_LPAREN, // '('
    TOKEN_SYMBOL_RPAREN, // ')'
    TOKEN_SYMBOL_LBRACE, // '{'
    TOKEN_SYMBOL_RBRACE, // '}'
    TOKEN_SYMBOL_SEMICOLON, // ';'
    TOKEN_EOF, // End of input
    TOKEN_UNKNOWN // Unrecognized character/token
} TokenType;

/**
 * Represents a single token.
 * - type: the kind of token (see TokenType)
 * - lexeme: dynamically allocated string holding the matched text
 * - position: byte offset in the source string where the token starts
 */
typedef struct {
    TokenType type;
    char *lexeme;
    size_t position;
} Token;

/**
 * Lexer state object.
 * - src: pointer to the input source string
 * - pos: current position in the source string
 * - len: total length of the input source string
 */
typedef struct {
    const char *src;
    size_t pos;
    size_t len;
} Lexer;

/**
 * Initializes the lexer state for a given input string.
 * @param lexer Pointer to a Lexer struct
 * @param src   Null-terminated source string to tokenize
 */
void lexer_init(Lexer *lexer, const char *src);

/**
 * Returns the next token from the input stream.
 * Skips whitespace and advances the lexer's position.
 * The returned token's lexeme must be freed by the caller using token_free().
 * @param lexer Pointer to initialized Lexer
 * @return      Next Token in the input stream
 */
Token lexer_next_token(Lexer *lexer);

/**
 * Resets the lexer's position to the beginning of the source string.
 * @param lexer Pointer to initialized Lexer
 */
void lexer_reset(Lexer *lexer);

/**
 * Function to create a string representation of a token (for debugging/printing)
 * Writes the representation into the provided buffer.
 */
void token_to_string(Token token, char *buffer, size_t buffer_size);

/**
 * Frees resources held by a token (specifically, the lexeme string).
 * @param token Pointer to Token
 */
void token_free(const Token *token);

#endif // LEXER_H
