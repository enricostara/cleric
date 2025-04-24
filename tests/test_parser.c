#include "unity.h"
#include "../src/lexer/lexer.h" // Include Lexer for tokenization
#include "../src/parser/parser.h" // Include Parser for parsing
#include "../src/parser/ast.h"    // Include AST for structure checks


// --- Test Helper Function ---

// Helper to parse input and return the AST root, managing lexer/parser init/destroy
// static ProgramNode* parse_test_input(const char* input) {
//     Lexer lexer;
//     lexer_init(&lexer, input);

//     Parser parser;
//     parser_init(&parser, &lexer);

//     ProgramNode *program = parse_program(&parser);

//     // Note: We don't free the lexer here because the parser keeps a pointer.
//     // The parser doesn't own the lexer, so the caller (driver or test) should manage lexer lifetime.
//     // However, tokens allocated by the lexer and held by the parser need care.
//     // For simplicity in testing, we assume parse_program consumes all relevant tokens,
//     // and the parser_init/advance handles freeing token lexemes.
//     // If the parser failed mid-way, some tokens might linger in parser.current/peek.
//     // A more robust test setup might require explicit parser cleanup function.

//     return program; // Caller must free the AST if not NULL
// }


// --- Test Cases ---

// Test parsing a simple valid program: "int main(void) { return 42; }"
void test_parse_valid_program(void) {
    const char *input = "int main(void) { return 42; }";
    // Assert parsing succeeded without error flag
    // TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input");
    // We need access to the parser state after parse_program to check the flag.
    // Let's modify parse_test_input or create the parser outside for this check.

    // Reworking the test slightly to check the flag:
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer);
    ProgramNode *program_reworked = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program_reworked, "Parser returned NULL for valid input");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input");

    // --- AST Structure Verification ---
    // Program -> FuncDef
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program_reworked->base.type);
    TEST_ASSERT_NOT_NULL(program_reworked->function);
    FuncDefNode *func_def = program_reworked->function;

    // FuncDef -> ReturnStmt
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);
    AstNode *stmt = func_def->body;
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, stmt->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) stmt;

    // ReturnStmt -> IntLiteral
    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, expr->type);
    IntLiteralNode *int_literal = (IntLiteralNode *) expr;

    // Check the integer value
    TEST_ASSERT_EQUAL_INT(42, int_literal->value);

    // Clean up the AST
    free_ast((AstNode *) program_reworked);
}


// Test parsing an invalid program (missing semicolon)
void test_parse_missing_semicolon(void) {
    const char *input = "int main(void) { return 42 }"; // Missing semicolon
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing semicolon");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing semicolon");

    // No AST to free as parsing should fail and return NULL
}

// Test parsing an invalid program (missing closing brace)
void test_parse_missing_brace(void) {
    const char *input = "int main(void) { return 42;"; // Missing brace
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing brace");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing brace");
}

// Add more test cases for different errors later...


// --- Test Runner ---
// Group parser tests into a single runner function
void run_parser_tests(void) {
    printf("\n--- Parser Tests ---\n");
    RUN_TEST(test_parse_valid_program);
    RUN_TEST(test_parse_missing_semicolon);
    RUN_TEST(test_parse_missing_brace);
    // Add RUN_TEST calls for other parser tests here
}
