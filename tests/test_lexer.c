#include <string.h>

#include "../src/lexer/lexer.h"
#include "unity/unity.h"

void test_tokenize_minimal_c(void) {
    const char *src = "int main(void) { return 2; }\n";
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

void test_tokenize_unknown_token(void) {
    // The input contains an unrecognized character: '@'
    const char *src = "int main() { @ }";
    Lexer lexer;
    lexer_init(&lexer, src);
    Token tok;
    // Skip to the unknown token
    tok = lexer_next_token(&lexer);
    token_free(&tok); // int
    tok = lexer_next_token(&lexer);
    token_free(&tok); // main
    tok = lexer_next_token(&lexer);
    token_free(&tok); // (
    tok = lexer_next_token(&lexer);
    token_free(&tok); // )
    tok = lexer_next_token(&lexer);
    token_free(&tok); // {
    // Now should get TOKEN_UNKNOWN for '@'
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_UNKNOWN, tok.type);
    TEST_ASSERT_NOT_NULL(tok.lexeme);
    TEST_ASSERT_EQUAL_CHAR('@', tok.lexeme[0]);
    token_free(&tok);
    // Next should be '}'
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_SYMBOL_RBRACE, tok.type);
    token_free(&tok);
    // End of input
    tok = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL(TOKEN_EOF, tok.type);
    token_free(&tok);
}

void test_tokenize_invalid_identifier(void) {
    // Input: constant immediately followed by identifier (invalid in C): 1foo
    const char *src = "int main() { return 1foo; }";
    Lexer lexer;
    lexer_init(&lexer, src);
    Token tok;
    int found_invalid = 0;
    while ((tok = lexer_next_token(&lexer)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_UNKNOWN) {
            found_invalid = 1;
            TEST_ASSERT_NOT_NULL(tok.lexeme);
            // Should catch the 'f' as invalid after '1'
            TEST_ASSERT_EQUAL_CHAR('f', tok.lexeme[0]);
            token_free(&tok);
            break;
        }
        token_free(&tok);
    }
    TEST_ASSERT_TRUE_MESSAGE(found_invalid, "Lexer did not catch invalid identifier after constant");
}

void test_token_to_string_keyword(void) {
    char buffer[64];
    Token token = {TOKEN_KEYWORD_INT, NULL, 0};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("INT", buffer);

    token.type = TOKEN_KEYWORD_RETURN;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("RETURN", buffer);
}

void test_token_to_string_symbol(void) {
    char buffer[64];
    Token token = {TOKEN_SYMBOL_LPAREN, NULL, 0};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'('", buffer);

    token.type = TOKEN_SYMBOL_SEMICOLON;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("';'", buffer);
}

void test_token_to_string_identifier(void) {
    char buffer[64];
    char lexeme[] = "myVar"; // Faking lexeme allocation for test
    Token token = {TOKEN_IDENTIFIER, lexeme, strlen(lexeme)};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("IDENTIFIER('myVar')", buffer);
}

void test_token_to_string_constant(void) {
    char buffer[64];
    char lexeme[] = "12345"; // Faking lexeme allocation for test
    Token token = {TOKEN_CONSTANT, lexeme, strlen(lexeme)};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("CONSTANT('12345')", buffer);
}

void test_token_to_string_eof(void) {
    char buffer[64];
    Token token = {TOKEN_EOF, NULL, 0};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("EOF", buffer);
}

void test_token_to_string_unknown(void) {
    char buffer[64];
    char lexeme[] = "@"; // Faking lexeme allocation for test
    Token token = {TOKEN_UNKNOWN, lexeme, strlen(lexeme)};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("UNKNOWN('@')", buffer);
}

void test_token_to_string_buffer_limit(void) {
    char small_buffer[10];
    char lexeme[] = "long_identifier";
    Token token = {TOKEN_IDENTIFIER, lexeme, strlen(lexeme)};
    token_to_string(token, small_buffer, sizeof(small_buffer));
    // Check expected truncated output and null termination
    TEST_ASSERT_EQUAL_STRING("IDENTIFIE", small_buffer);
    TEST_ASSERT_EQUAL('\0', small_buffer[sizeof(small_buffer) - 1]);
}

void run_lexer_tests(void) {
    RUN_TEST(test_tokenize_minimal_c);
    RUN_TEST(test_tokenize_unknown_token);
    RUN_TEST(test_tokenize_invalid_identifier);
    RUN_TEST(test_token_to_string_keyword);
    RUN_TEST(test_token_to_string_symbol);
    RUN_TEST(test_token_to_string_identifier);
    RUN_TEST(test_token_to_string_constant);
    RUN_TEST(test_token_to_string_eof);
    RUN_TEST(test_token_to_string_unknown);
    RUN_TEST(test_token_to_string_buffer_limit);
}
