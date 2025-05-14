#include "lexer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include "memory/arena.h" // Include Arena for allocation

/**
 * Checks if the given string matches a reserved keyword.
 * If matched, sets out_type to the corresponding keyword token.
 * @param str    Pointer to start of candidate keyword
 * @param len    Length of the candidate
 * @param out_type Pointer to TokenType to set if matched
 * @return 1 if keyword, 0 otherwise
 */
static int is_keyword(const char *str, const size_t len, TokenType *out_type) {
    if (len == 3 && strncmp(str, "int", 3) == 0) {
        *out_type = TOKEN_KEYWORD_INT;
        return 1;
    }
    if (len == 4 && strncmp(str, "void", 4) == 0) {
        *out_type = TOKEN_KEYWORD_VOID;
        return 1;
    }
    if (len == 6 && strncmp(str, "return", 6) == 0) {
        *out_type = TOKEN_KEYWORD_RETURN;
        return 1;
    }
    return 0;
}

/**
 * Initializes the lexer state for a given input string.
 */
void lexer_init(Lexer *lexer, const char *src, Arena *arena) {
    lexer->src = src;
    lexer->pos = 0;
    lexer->len = strlen(src);
    lexer->arena = arena; // Store the arena pointer
}

/**
 * Resets the lexer's position to the beginning of the source string.
 */
void lexer_reset(Lexer *lexer) {
    lexer->pos = 0;
}

/**
 * Advances the lexer position past any whitespace characters.
 * @param lexer Pointer to Lexer
 */
static void skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->len && isspace(lexer->src[lexer->pos])) {
        lexer->pos++;
    }
}

/**
 * Scans and returns the next token from the input.
 * The returned token's lexeme (if applicable, e.g., identifiers, constants)
 * will be allocated within the lexer's arena.
 * If the end of input is reached, returns TOKEN_EOF.
 */
bool lexer_next_token(Lexer *lexer, Token *out_token) {
    // Initialize out_token to a safe default (e.g., UNKNOWN, NULL lexeme)
    // Although if false is returned, its state is undefined per the header comment.
    *out_token = (Token){TOKEN_UNKNOWN, NULL, lexer->pos};

    skip_whitespace(lexer);
    size_t start = lexer->pos;
    if (lexer->pos >= lexer->len) {
        *out_token = (Token){TOKEN_EOF, NULL, lexer->pos};
        return true; // Successfully found EOF
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
            *out_token = (Token){type, NULL, id_start};
            return true;
        }
        // Allocate lexeme in the arena
        char *lexeme = arena_alloc(lexer->arena, id_len + 1);
        if (!lexeme) {
            fprintf(stderr, "Lexer Error: Arena allocation failed for identifier lexeme.\n");
            return false; // Allocation failure
        }
        memcpy(lexeme, lexer->src + id_start, id_len);
        lexeme[id_len] = '\0'; // Null-terminate
        *out_token = (Token){TOKEN_IDENTIFIER, lexeme, id_start};
        return true;
    }
    // Integer constants: [0-9]+
    if (isdigit(c)) {
        const size_t const_start = lexer->pos;
        lexer->pos++; // Consume the first digit
        while (lexer->pos < lexer->len && isdigit(lexer->src[lexer->pos])) {
            lexer->pos++; // Consume subsequent digits
        }
        // Check for invalid trailing identifier part (e.g., 1foo)
        if (lexer->pos < lexer->len && (isalpha(lexer->src[lexer->pos]) || lexer->src[lexer->pos] == '_')) {
            // Invalid character found after digits. Consume it.
            char bad_char = lexer->src[lexer->pos];
            lexer->pos++; // Advance past the bad character
            // Allocate space for the single bad character in the arena
            char *lexeme = arena_alloc(lexer->arena, 2); // 1 char + null terminator
            if (!lexeme) {
                fprintf(stderr, "Lexer Error: Arena allocation failed for unknown token lexeme (suffix).\n");
                return false; // Allocation failure
            }
            lexeme[0] = bad_char;
            lexeme[1] = '\0';
            *out_token = (Token){TOKEN_UNKNOWN, lexeme, lexer->pos - 1}; // Position is where the bad char was
            return true;
        }
        // If no invalid char followed, it's a valid constant.
        const size_t const_len = lexer->pos - const_start;
        char *lexeme = arena_alloc(lexer->arena, const_len + 1);
        if (!lexeme) {
            fprintf(stderr, "Lexer Error: Arena allocation failed for constant lexeme.\n");
            return false; // Allocation failure
        }
        memcpy(lexeme, lexer->src + const_start, const_len);
        lexeme[const_len] = '\0';
        *out_token = (Token){TOKEN_CONSTANT, lexeme, const_start};
        return true;
    }
    // Single-character symbols and potential multi-character symbols
    TokenType sym_type = TOKEN_UNKNOWN;
    switch (c) {
        case '(': sym_type = TOKEN_SYMBOL_LPAREN;
            break;
        case ')': sym_type = TOKEN_SYMBOL_RPAREN;
            break;
        case '{': sym_type = TOKEN_SYMBOL_LBRACE;
            break;
        case '}': sym_type = TOKEN_SYMBOL_RBRACE;
            break;
        case ';': sym_type = TOKEN_SYMBOL_SEMICOLON;
            break;
        case '~': sym_type = TOKEN_SYMBOL_TILDE;
            break;
        case '-':
            // Check for '--' (decrement)
            if (lexer->pos + 1 < lexer->len && lexer->src[lexer->pos + 1] == '-') {
                lexer->pos += 2; // Consume both '-'
                *out_token = (Token){TOKEN_SYMBOL_DECREMENT, NULL, start};
                return true;
            }
        // Otherwise, it's just '-' (minus)
            sym_type = TOKEN_SYMBOL_MINUS;
            break;
        case '+': sym_type = TOKEN_SYMBOL_PLUS;
            break;
        case '*': sym_type = TOKEN_SYMBOL_STAR;
            break;
        case '/': sym_type = TOKEN_SYMBOL_SLASH;
            break;
        case '%': sym_type = TOKEN_SYMBOL_PERCENT;
            break;
        case '<':
            if (lexer->pos + 1 < lexer->len && lexer->src[lexer->pos + 1] == '=') {
                lexer->pos += 2; // Consume both '<='
                *out_token = (Token){TOKEN_SYMBOL_LESS_EQUAL, NULL, start};
                return true;
            }
            sym_type = TOKEN_SYMBOL_LESS;
            break;
        case '>':
            if (lexer->pos + 1 < lexer->len && lexer->src[lexer->pos + 1] == '=') {
                lexer->pos += 2; // Consume both '>='
                *out_token = (Token){TOKEN_SYMBOL_GREATER_EQUAL, NULL, start};
                return true;
            }
            sym_type = TOKEN_SYMBOL_GREATER;
            break;
        case '=':
            if (lexer->pos + 1 < lexer->len && lexer->src[lexer->pos + 1] == '=') {
                lexer->pos += 2; // Consume both '=='
                *out_token = (Token){TOKEN_SYMBOL_EQUAL_EQUAL, NULL, start};
                return true;
            }
            sym_type = TOKEN_SYMBOL_ASSIGN;
            break;
        case '!':
            if (lexer->pos + 1 < lexer->len && lexer->src[lexer->pos + 1] == '=') {
                lexer->pos += 2; // Consume both '!='
                *out_token = (Token){TOKEN_SYMBOL_NOT_EQUAL, NULL, start};
                return true;
            }
            sym_type = TOKEN_SYMBOL_LOGICAL_NOT;
            break;
        case '&':
            if (lexer->pos + 1 < lexer->len && lexer->src[lexer->pos + 1] == '&') {
                lexer->pos += 2; // Consume both '&&'
                *out_token = (Token){TOKEN_SYMBOL_LOGICAL_AND, NULL, start};
                return true;
            }
            // Single '&' will fall through to TOKEN_UNKNOWN
            break;
        case '|':
            if (lexer->pos + 1 < lexer->len && lexer->src[lexer->pos + 1] == '|') {
                lexer->pos += 2; // Consume both '||'
                *out_token = (Token){TOKEN_SYMBOL_LOGICAL_OR, NULL, start};
                return true;
            }
            // Single '|' will fall through to TOKEN_UNKNOWN
            break;
        default: break;
    }
    if (sym_type != TOKEN_UNKNOWN) {
        lexer->pos++;
        // No need to allocate lexeme for symbols; type is sufficient
        *out_token = (Token){sym_type, NULL, start};
        return true;
    }
    // Unknown/unrecognized character: return as TOKEN_UNKNOWN
    lexer->pos++;
    // Allocate space for the single unknown character in the arena
    char *lexeme = arena_alloc(lexer->arena, 2); // 1 char + null terminator
    if (!lexeme) {
        fprintf(stderr, "Lexer Error: Arena allocation failed for unknown token lexeme (char).\n");
        return false; // Allocation failure
    }
    lexeme[0] = c;
    lexeme[1] = '\0';
    *out_token = (Token){TOKEN_UNKNOWN, lexeme, start};
    return true;
}

// Function to create a string representation of a token
void token_to_string(Token token, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return; // Safety check

    const char *type_str;
    bool has_lexeme = false;

    switch (token.type) {
        // Keywords
        case TOKEN_KEYWORD_INT: type_str = "INT";
            break;
        case TOKEN_KEYWORD_VOID: type_str = "VOID";
            break;
        case TOKEN_KEYWORD_RETURN: type_str = "RETURN";
            break;
        // Add other keywords here...

        // Identifiers and Literals
        case TOKEN_IDENTIFIER: type_str = "IDENTIFIER";
            has_lexeme = true;
            break;
        case TOKEN_CONSTANT: type_str = "CONSTANT";
            has_lexeme = true;
            break;

        // Symbols
        case TOKEN_SYMBOL_LPAREN: type_str = "'('";
            break;
        case TOKEN_SYMBOL_RPAREN: type_str = "')'";
            break;
        case TOKEN_SYMBOL_LBRACE: type_str = "'{'";
            break;
        case TOKEN_SYMBOL_RBRACE: type_str = "'}'";
            break;
        case TOKEN_SYMBOL_SEMICOLON: type_str = "';'";
            break;
        case TOKEN_SYMBOL_TILDE: type_str = "'~'";
            break;
        case TOKEN_SYMBOL_MINUS: type_str = "'-'";
            break;
        case TOKEN_SYMBOL_DECREMENT: type_str = "'--'";
            break;
        case TOKEN_SYMBOL_PLUS: type_str = "'+'";
            break;
        case TOKEN_SYMBOL_STAR: type_str = "'*'";
            break;
        case TOKEN_SYMBOL_SLASH: type_str = "'/'";
            break;
        case TOKEN_SYMBOL_PERCENT: type_str = "'%'";
            break;
        case TOKEN_SYMBOL_LESS: type_str = "'<'";
            break;
        case TOKEN_SYMBOL_GREATER: type_str = "'>'";
            break;
        case TOKEN_SYMBOL_LESS_EQUAL: type_str = "'<='";
            break;
        case TOKEN_SYMBOL_GREATER_EQUAL: type_str = "'>='";
            break;
        case TOKEN_SYMBOL_EQUAL_EQUAL: type_str = "'=='";
            break;
        case TOKEN_SYMBOL_NOT_EQUAL: type_str = "'!='";
            break;
        case TOKEN_SYMBOL_LOGICAL_AND: type_str = "'&&'";
            break;
        case TOKEN_SYMBOL_LOGICAL_OR: type_str = "'||'";
            break;
        case TOKEN_SYMBOL_LOGICAL_NOT: type_str = "'!'";
            break;
        case TOKEN_SYMBOL_ASSIGN: type_str = "'='";
            break;
        // Special Tokens
        case TOKEN_EOF: type_str = "EOF";
            break;
        case TOKEN_UNKNOWN: type_str = "UNKNOWN";
            has_lexeme = true;
            break; // Show unknown lexeme

        default: type_str = "INVALID_TYPE";
            break;
    }

    if (has_lexeme && token.lexeme) {
        snprintf(buffer, buffer_size, "%s('%s')", type_str, token.lexeme);
    } else {
        snprintf(buffer, buffer_size, "%s", type_str);
    }
    // Ensure null termination even if snprintf truncates
    buffer[buffer_size - 1] = '\0';
}
