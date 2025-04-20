#include "unity/unity.h"
#include "../src/lexer/lexer.h"
#include <string.h>

void test_tokenize_minimal_c(void) {
    const char* src = "int main(void) { return 2; }\n";
    Lexer lexer;
    lexer_init(&lexer, src);
    Token tok;
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_KEYWORD_INT, tok.type);
    TEST_ASSERT_EQUAL_STRING("int", tok.lexeme);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_IDENTIFIER, tok.type);
    TEST_ASSERT_EQUAL_STRING("main", tok.lexeme);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_LPAREN, tok.type);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_KEYWORD_VOID, tok.type);
    TEST_ASSERT_EQUAL_STRING("void", tok.lexeme);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_RPAREN, tok.type);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_LBRACE, tok.type);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_KEYWORD_RETURN, tok.type);
    TEST_ASSERT_EQUAL_STRING("return", tok.lexeme);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_CONSTANT, tok.type);
    TEST_ASSERT_EQUAL_STRING("2", tok.lexeme);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_SEMICOLON, tok.type);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_RBRACE, tok.type);
    token_free(&tok);
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_EOF, tok.type);
}

