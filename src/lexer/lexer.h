#ifndef LEXER_H
#define LEXER_H

#include "memory/arena.h" // Include Arena for allocation

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
    TOKEN_SYMBOL_TILDE, // '~'
    TOKEN_SYMBOL_MINUS, // '-'
    TOKEN_SYMBOL_PLUS, // '+'
    TOKEN_SYMBOL_STAR, // '*'
    TOKEN_SYMBOL_SLASH, // '/'
    TOKEN_SYMBOL_PERCENT, // '%'
    // New relational, logical, and assignment operators
    TOKEN_SYMBOL_LESS,          // '<'
    TOKEN_SYMBOL_GREATER,       // '>'
    TOKEN_SYMBOL_LESS_EQUAL,    // '<='
    TOKEN_SYMBOL_GREATER_EQUAL, // '>='
    TOKEN_SYMBOL_EQUAL_EQUAL,   // '=='
    TOKEN_SYMBOL_NOT_EQUAL,     // '!='
    TOKEN_SYMBOL_LOGICAL_AND,   // '&&'
    TOKEN_SYMBOL_LOGICAL_OR,    // '||'
    TOKEN_SYMBOL_LOGICAL_NOT,   // '!'
    TOKEN_SYMBOL_ASSIGN,        // '='    
    TOKEN_SYMBOL_DECREMENT, // '--'
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
 * - arena: Pointer to the arena used for allocations
 */
typedef struct {
    const char *src;
    size_t pos;
    size_t len;
    Arena *arena; // Pointer to the arena used for allocations
} Lexer;

/**
 * Initializes the lexer state for a given input string.
 * @param lexer Pointer to a Lexer struct
 * @param src   Null-terminated source string to tokenize
 * @param arena Pointer to the Arena to use for allocations
 */
void lexer_init(Lexer *lexer, const char *src, Arena *arena);

/**
 * Retrieves the next token from the lexer's source string.
 * Advances the lexer's position.
 * The lexeme (if any) is allocated within the lexer's internal arena.
 * Do not free the lexeme; it will be freed when the arena is destroyed.
 *
 * @param lexer Pointer to the initialized Lexer.
 * @param out_token Pointer to a Token struct where the result will be stored.
 * @return true if a token (including TOKEN_EOF) was successfully retrieved,
 *         false if an error occurred (e.g., memory allocation failure).
 *         If false is returned, the contents of out_token are undefined.
 *         If true is returned and the token type is TOKEN_UNKNOWN, it indicates
 *         an unrecognized character was encountered.
 */
bool lexer_next_token(Lexer *lexer, Token *out_token);

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

#endif // LEXER_H
