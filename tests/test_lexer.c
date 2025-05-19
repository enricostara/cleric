#include <string.h>
#include <stdio.h> // For snprintf in helper

#include "../src/lexer/lexer.h"
#include "_unity/unity.h"
#include "memory/arena.h"

// --- Test Case Data Structures ---

typedef struct {
    TokenType type;
    const char *lexeme; // Expected lexeme (NULL if not applicable or checked)
} ExpectedToken;

typedef struct {
    const char *name; // Name of the test case
    const char *source;
    ExpectedToken *expected_tokens;
    size_t num_expected_tokens;
} LexerTestCase;

// --- Helper Function to Run a Single Test Case ---

static void run_single_lexer_test(const LexerTestCase *test_case, Arena *arena) {
    char message[256];
    snprintf(message, sizeof(message), "Test Case: %s", test_case->name);
    UNITY_TEST_ASSERT_NOT_NULL(test_case->source, __LINE__, "Test case source cannot be NULL.");
    // Only assert non-NULL expected_tokens if we actually expect tokens
    if (test_case->num_expected_tokens > 0) {
        UNITY_TEST_ASSERT_NOT_NULL(test_case->expected_tokens, __LINE__,
                                   "Test case expected tokens cannot be NULL for non-zero token count.");
    }

    Lexer lexer;
    lexer_init(&lexer, test_case->source, arena); // Pass arena to initializer
    Token tok;
    bool success; // To capture the return value
    size_t token_count = 0;

    for (size_t i = 0; i < test_case->num_expected_tokens; ++i) {
        success = lexer_next_token(&lexer, &tok); // Use lexer's internal arena
        snprintf(message, sizeof(message), "[%s] lexer_next_token failed unexpectedly (returned false) at token %zu",
                 test_case->name, i + 1);
        TEST_ASSERT_TRUE_MESSAGE(success, message); // Fail if lexer reported an error
        token_count++;

        // Prepare detailed message for assertion failures
        char assert_msg[512];
        char tok_str_buf[128];
        token_to_string(tok, tok_str_buf, sizeof(tok_str_buf));

        snprintf(assert_msg, sizeof(assert_msg),
                 "[%s] Token %zu: Type mismatch. Expected %d, Got %d (%s)",
                 test_case->name, i + 1, test_case->expected_tokens[i].type, tok.type, tok_str_buf);
        TEST_ASSERT_EQUAL_INT_MESSAGE(test_case->expected_tokens[i].type, tok.type, assert_msg);

        if (test_case->expected_tokens[i].lexeme) {
            snprintf(assert_msg, sizeof(assert_msg),
                     "[%s] Token %zu: Lexeme mismatch. Expected '%s', Got '%s'",
                     test_case->name, i + 1, test_case->expected_tokens[i].lexeme, tok.lexeme ? tok.lexeme : "NULL");
            TEST_ASSERT_NOT_NULL_MESSAGE(tok.lexeme, assert_msg);
            TEST_ASSERT_EQUAL_STRING_MESSAGE(test_case->expected_tokens[i].lexeme, tok.lexeme, assert_msg);
        }
    }

    // Check for EOF
    success = lexer_next_token(&lexer, &tok);
    snprintf(message, sizeof(message),
             "[%s] lexer_next_token failed unexpectedly (returned false) when checking for EOF", test_case->name);
    TEST_ASSERT_TRUE_MESSAGE(success, message); // Fail if lexer reported an error

    snprintf(message, sizeof(message), "[%s] Expected EOF after %zu tokens", test_case->name,
             test_case->num_expected_tokens);
    TEST_ASSERT_EQUAL_INT_MESSAGE(TOKEN_EOF, tok.type, message);

    // Check if any extra tokens were produced
    success = lexer_next_token(&lexer, &tok);
    snprintf(message, sizeof(message),
             "[%s] lexer_next_token failed unexpectedly (returned false) when checking for extra tokens",
             test_case->name);
    TEST_ASSERT_TRUE_MESSAGE(success, message); // Fail if lexer reported an error
    snprintf(message, sizeof(message), "[%s] Expected EOF, but got another token (%d)", test_case->name, tok.type);
    TEST_ASSERT_EQUAL_INT_MESSAGE(TOKEN_EOF, tok.type, message);
}

// --- Test Data ---

// Expected tokens for "int main(void) { return 2; }\n"
ExpectedToken minimal_c_expected[] = {
    {TOKEN_KEYWORD_INT, NULL},
    {TOKEN_IDENTIFIER, "main"},
    {TOKEN_SYMBOL_LPAREN, NULL},
    {TOKEN_KEYWORD_VOID, NULL},
    {TOKEN_SYMBOL_RPAREN, NULL},
    {TOKEN_SYMBOL_LBRACE, NULL},
    {TOKEN_KEYWORD_RETURN, NULL},
    {TOKEN_CONSTANT, "2"},
    {TOKEN_SYMBOL_SEMICOLON, NULL},
    {TOKEN_SYMBOL_RBRACE, NULL}
};

// Expected tokens for "int main() { @ }"
ExpectedToken unknown_token_expected[] = {
    {TOKEN_KEYWORD_INT, NULL},
    {TOKEN_IDENTIFIER, "main"},
    {TOKEN_SYMBOL_LPAREN, NULL},
    {TOKEN_SYMBOL_RPAREN, NULL},
    {TOKEN_SYMBOL_LBRACE, NULL},
    {TOKEN_UNKNOWN, "@"},
    {TOKEN_SYMBOL_RBRACE, NULL}
};

// Expected tokens for "int main() { return 1foo; }"
// Note: We expect UNKNOWN('f') after consuming '1', then IDENTIFIER('oo')
ExpectedToken invalid_identifier_expected[] = {
    {TOKEN_KEYWORD_INT, NULL},
    {TOKEN_IDENTIFIER, "main"},
    {TOKEN_SYMBOL_LPAREN, NULL},
    {TOKEN_SYMBOL_RPAREN, NULL},
    {TOKEN_SYMBOL_LBRACE, NULL},
    {TOKEN_KEYWORD_RETURN, NULL},
    {TOKEN_UNKNOWN, "f"}, // Token 7: Invalid char after '1'
    {TOKEN_IDENTIFIER, "oo"}, // Token 8: Remaining part
    {TOKEN_SYMBOL_SEMICOLON, NULL},
    {TOKEN_SYMBOL_RBRACE, NULL}
};

// Expected tokens for "~ -- - "
ExpectedToken operators_expected[] = {
    {TOKEN_SYMBOL_TILDE, NULL},
    {TOKEN_SYMBOL_DECREMENT, NULL},
    {TOKEN_SYMBOL_MINUS, NULL}
};

// Expected tokens for "+ * / %"
ExpectedToken binary_ops_expected[] = {
    {TOKEN_SYMBOL_PLUS, NULL},
    {TOKEN_SYMBOL_STAR, NULL},
    {TOKEN_SYMBOL_SLASH, NULL},
    {TOKEN_SYMBOL_PERCENT, NULL}
};

// Define an empty array for tests that expect zero tokens
ExpectedToken no_tokens_expected[] = {};

// Array of all test cases
LexerTestCase test_cases[] = {
    {
        "Minimal C function",
        "int main(void) { return 2; }\n",
        minimal_c_expected,
        sizeof(minimal_c_expected) / sizeof(ExpectedToken)
    },
    {
        "Unknown character",
        "int main() { @ }",
        unknown_token_expected,
        sizeof(unknown_token_expected) / sizeof(ExpectedToken)
    },
    {
        "Invalid identifier after constant",
        "int main() { return 1foo; }",
        invalid_identifier_expected,
        sizeof(invalid_identifier_expected) / sizeof(ExpectedToken)
    },
    {
        "Operators ~ -- -",
        "~ -- - ",
        operators_expected,
        sizeof(operators_expected) / sizeof(ExpectedToken)
    },
    {
        "Binary Operators + * / %",
        "+ * / %",
        binary_ops_expected,
        sizeof(binary_ops_expected) / sizeof(ExpectedToken)
    },
    // Add more test cases here easily
    {
        "Empty Input",
        "",
        no_tokens_expected, // Use the empty array
        0
    },
    {
        "Whitespace only",
        "  \t \n ",
        no_tokens_expected, // Use the empty array
        0
    }
};

// --- Test Runner Function for Tokenization ---

// This function is called by Unity for each test case
void test_lexer_runner(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);

    size_t num_test_cases = sizeof(test_cases) / sizeof(LexerTestCase);
    for (size_t i = 0; i < num_test_cases; ++i) {
        // Use Unity's test case execution mechanism if needed, or just call directly
        // For simplicity here, we call directly. If using advanced Unity features,
        // you might register each test case name.
        run_single_lexer_test(&test_cases[i], &test_arena);
    }

    arena_destroy(&test_arena);
}

// --- Tests for token_to_string (can remain separate) ---

void test_token_to_string_keyword(void) {
    char buffer[64];
    Token token = {TOKEN_KEYWORD_INT, NULL, 0};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("INT", buffer);

    token.type = TOKEN_KEYWORD_VOID;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("VOID", buffer);

    token.type = TOKEN_KEYWORD_RETURN;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("RETURN", buffer);
}

void test_token_to_string_symbol(void) {
    char buffer[64];
    Token token = {TOKEN_SYMBOL_LPAREN, NULL, 0};
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'('", buffer);

    token.type = TOKEN_SYMBOL_RPAREN;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("')'", buffer);

    token.type = TOKEN_SYMBOL_LBRACE;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'{'", buffer);

    token.type = TOKEN_SYMBOL_RBRACE;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'}'", buffer);

    token.type = TOKEN_SYMBOL_SEMICOLON;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("';'", buffer);

    token.type = TOKEN_SYMBOL_TILDE;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'~'", buffer);

    token.type = TOKEN_SYMBOL_MINUS;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'-'", buffer);

    token.type = TOKEN_SYMBOL_DECREMENT;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'--'", buffer);

    token.type = TOKEN_SYMBOL_PLUS;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'+'", buffer);

    token.type = TOKEN_SYMBOL_STAR;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'*'", buffer);

    token.type = TOKEN_SYMBOL_SLASH;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'/'", buffer);

    token.type = TOKEN_SYMBOL_PERCENT;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'%'", buffer);

    // Relational Operators
    token.type = TOKEN_SYMBOL_LESS;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'<'", buffer);

    token.type = TOKEN_SYMBOL_GREATER;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'>'", buffer);

    token.type = TOKEN_SYMBOL_LESS_EQUAL;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'<='", buffer);

    token.type = TOKEN_SYMBOL_GREATER_EQUAL;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'>='", buffer);

    token.type = TOKEN_SYMBOL_EQUAL_EQUAL;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'=='", buffer);

    token.type = TOKEN_SYMBOL_NOT_EQUAL;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'!='", buffer);

    // Logical Operators
    token.type = TOKEN_SYMBOL_LOGICAL_AND;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'&&'", buffer);

    token.type = TOKEN_SYMBOL_LOGICAL_OR;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'||'", buffer);

    token.type = TOKEN_SYMBOL_BANG;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'!'", buffer);

    // Assignment Operator
    token.type = TOKEN_SYMBOL_ASSIGN;
    token_to_string(token, buffer, 64);
    TEST_ASSERT_EQUAL_STRING("'='", buffer);
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
    // Run the consolidated tokenization tests
    RUN_TEST(test_lexer_runner);

    // Run the token_to_string tests
    RUN_TEST(test_token_to_string_keyword);
    RUN_TEST(test_token_to_string_symbol);
    RUN_TEST(test_token_to_string_identifier);
    RUN_TEST(test_token_to_string_constant);
    RUN_TEST(test_token_to_string_eof);
    RUN_TEST(test_token_to_string_unknown);
    RUN_TEST(test_token_to_string_buffer_limit);
}
