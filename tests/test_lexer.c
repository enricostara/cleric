#include "../src/lexer/lexer.h"
#include "unity/unity.h"
#include <string.h>

void test_tokenize_minimal_c(void) {
    const char* src = "int main(void) { return 2; }\n";
    Lexer lexer;
    lexer_init(&lexer, src);
    Token tok;

    // KEYWORD: int
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_KEYWORD_INT, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // IDENTIFIER: main
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_IDENTIFIER, tok.type);
    TEST_ASSERT_NOT_NULL(tok.lexeme);
    TEST_ASSERT_EQUAL_STRING("main", tok.lexeme);
    token_free(&tok);

    // SYMBOL: (
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_LPAREN, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // KEYWORD: void
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_KEYWORD_VOID, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // SYMBOL: )
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_RPAREN, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // SYMBOL: {
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_LBRACE, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // KEYWORD: return
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_KEYWORD_RETURN, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // CONSTANT: 2
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_CONSTANT, tok.type);
    TEST_ASSERT_NOT_NULL(tok.lexeme);
    TEST_ASSERT_EQUAL_STRING("2", tok.lexeme);
    token_free(&tok);

    // SYMBOL: ;
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_SEMICOLON, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // SYMBOL: }
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_RBRACE, tok.type);
    TEST_ASSERT_NULL(tok.lexeme);
    token_free(&tok);

    // End of input
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_EOF, tok.type);
    token_free(&tok);
}
