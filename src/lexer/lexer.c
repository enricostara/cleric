#include "lexer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * Checks if the given string matches a reserved keyword.
 * If matched, sets out_type to the corresponding keyword token.
 * @param str    Pointer to start of candidate keyword
 * @param len    Length of the candidate
 * @param out_type Pointer to TokenType to set if matched
 * @return 1 if keyword, 0 otherwise
 */
static int is_keyword(const char* str, size_t len, TokenType* out_type) {
    if (len == 3 && strncmp(str, "int", 3) == 0) { *out_type = TOKEN_KEYWORD_INT; return 1; }
    if (len == 4 && strncmp(str, "void", 4) == 0) { *out_type = TOKEN_KEYWORD_VOID; return 1; }
    if (len == 6 && strncmp(str, "return", 6) == 0) { *out_type = TOKEN_KEYWORD_RETURN; return 1; }
    return 0;
}

/**
 * Initializes the lexer state for a given input string.
 */
void lexer_init(Lexer* lexer, const char* src) {
    lexer->src = src;
    lexer->pos = 0;
    lexer->len = strlen(src);
}

/**
 * Advances the lexer position past any whitespace characters.
 * @param lexer Pointer to Lexer
 */
static void skip_whitespace(Lexer* lexer) {
    while (lexer->pos < lexer->len && isspace(lexer->src[lexer->pos])) {
        lexer->pos++;
    }
}

/**
 * Scans and returns the next token from the input.
 * The returned token's lexeme must be freed by the caller.
 * If the end of input is reached, returns TOKEN_EOF.
 */
Token lexer_next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    size_t start = lexer->pos;
    if (lexer->pos >= lexer->len) {
        return (Token){TOKEN_EOF, NULL, lexer->pos};
    }
    const char c = lexer->src[lexer->pos];
    // Identifiers and keywords: [a-zA-Z_][a-zA-Z0-9_]*
    if (isalpha(c) || c == '_') {
        const size_t id_start = lexer->pos;
        lexer->pos++;
        while (lexer->pos < lexer->len && (isalnum(lexer->src[lexer->pos]) || lexer->src[lexer->pos] == '_'))
            lexer->pos++;
        const size_t id_len = lexer->pos - id_start;
        TokenType type;
        if (is_keyword(lexer->src + id_start, id_len, &type)) {
            // For keywords, do not allocate a lexeme (set to NULL)
            return (Token){type, NULL, id_start};
        }
        char* lexeme = strndup(lexer->src + id_start, id_len);
        return (Token){TOKEN_IDENTIFIER, lexeme, id_start};
    }
    // Integer constants: [0-9]+
    if (isdigit(c)) {
        const size_t num_start = lexer->pos;
        lexer->pos++;
        while (lexer->pos < lexer->len && isdigit(lexer->src[lexer->pos]))
            lexer->pos++;
        const size_t num_len = lexer->pos - num_start;
        char* lexeme = strndup(lexer->src + num_start, num_len);
        return (Token){TOKEN_CONSTANT, lexeme, num_start};
    }
    // Single-character symbols
    TokenType sym_type = TOKEN_UNKNOWN;
    switch (c) {
        case '(': sym_type = TOKEN_SYMBOL_LPAREN; break;
        case ')': sym_type = TOKEN_SYMBOL_RPAREN; break;
        case '{': sym_type = TOKEN_SYMBOL_LBRACE; break;
        case '}': sym_type = TOKEN_SYMBOL_RBRACE; break;
        case ';': sym_type = TOKEN_SYMBOL_SEMICOLON; break;
        default: break;
    }
    if (sym_type != TOKEN_UNKNOWN) {
        lexer->pos++;
        // No need to allocate lexeme for symbols; type is sufficient
        return (Token){sym_type, NULL, start};
    }
    // Unknown/unrecognized character: return as TOKEN_UNKNOWN
    lexer->pos++;
    char* lexeme = strndup(&c, 1);
    return (Token){TOKEN_UNKNOWN, lexeme, start};
}

/**
 * Frees memory allocated for a token's lexeme.
 * @param token Pointer to Token
 */
void token_free(const Token* token) {
    if (!token) return;
    switch (token->type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_CONSTANT:
            if (token->lexeme) free(token->lexeme);
            break;
        default:
            // Keyword and symbol tokens: lexeme is NULL/static, do not free
            break;
    }
}
