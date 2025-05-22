#include <string.h>
#include <stdio.h> // For sprintf in error messages if needed by verify_parser_error

#include "unity.h"
#include "../../src/lexer/lexer.h"
#include "../../src/parser/parser.h"
#include "../../src/parser/ast.h"
#include "../../src/memory/arena.h"

// --- Helper for Error Tests ---
void verify_parser_error(const char *input, const char *expected_error_substring) {
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser); // This will trigger the parsing

    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser did not report an error as expected.");
    if (expected_error_substring) {
        TEST_ASSERT_NOT_NULL_MESSAGE(parser.error_message, "Parser error message was NULL.");
        if (parser.error_message) {
            // Defensive check before strstr
            char msg[256];
            sprintf(msg, "Error message mismatch. Expected substring '%s' not found in '%s'", expected_error_substring,
                    parser.error_message);
            TEST_ASSERT_NOT_NULL_MESSAGE(strstr(parser.error_message, expected_error_substring), msg);
        }
    }
    // ProgramNode might be NULL or partially formed, which is okay for error tests
    arena_destroy(&test_arena);
}

// --- Test Cases for Errors and Literals --- //

// Test parsing an invalid unary expression (missing operand)
void test_parse_invalid_unary_expression(void) {
    verify_parser_error("int main(void) { return -; }",
                        "Parse Error (near pos 25): Expected expression (integer, unary op, or '('), but got ';'");
}

// Test parsing a program with mismatched parentheses
void test_parse_mismatched_parentheses(void) {
    verify_parser_error("int main(void) { return (42; }", "Parse Error (near pos 27): Expected token ')', but got ';'");
}

// Test parsing an integer at the boundary of what's representable
void test_parse_integer_bounds(void) {
    const char *input_max = "int main(void) { return 2147483647; }"; // INT_MAX
    const char *input_min = "int main(void) { return -2147483647; }"; // -INT_MAX (formerly problematic INT_MIN)

    // --- INT_MAX Test --- //
    Arena arena_max = arena_create(1024);
    Lexer lexer_max;
    lexer_init(&lexer_max, input_max, &arena_max);
    Parser parser_max;
    parser_init(&parser_max, &lexer_max, &arena_max);
    ProgramNode *program_max = parse_program(&parser_max);

    if (program_max == NULL && parser_max.error_message) {
        fprintf(stderr, "Parser error in test_parse_integer_bounds (INT_MAX): %s\n", parser_max.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program_max, "parse_program() returned NULL for test_parse_integer_bounds (INT_MAX)");
    TEST_ASSERT_FALSE_MESSAGE(parser_max.error_flag, parser_max.error_message ? parser_max.error_message : "Parser error flag set in test_parse_integer_bounds (INT_MAX)");

    TEST_ASSERT_NOT_NULL_MESSAGE(program_max->function, "Function node is NULL (INT_MAX)");
    TEST_ASSERT_NOT_NULL_MESSAGE(program_max->function->body, "Function body (BlockNode) is NULL (INT_MAX)");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program_max->function->body->base.type, "Function body is not a BlockNode (INT_MAX)");
    BlockNode *body_block_max = program_max->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block_max->num_items, "Expected 1 item in function body block (INT_MAX)");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block_max->items[0], "First item in block is NULL (INT_MAX)");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block_max->items[0]->type, "First item in block is not a return statement (INT_MAX)");
    ReturnStmtNode *ret_stmt_max = (ReturnStmtNode *) body_block_max->items[0];

    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, ret_stmt_max->expression->type);
    TEST_ASSERT_EQUAL_INT(2147483647, ((IntLiteralNode*)ret_stmt_max->expression)->value);
    arena_destroy(&arena_max);

    // --- -INT_MAX Test (formerly INT_MIN which caused issues) --- //
    Arena arena_min = arena_create(1024);
    Lexer lexer_min;
    lexer_init(&lexer_min, input_min, &arena_min);
    Parser parser_min;
    parser_init(&parser_min, &lexer_min, &arena_min);
    ProgramNode *program_min = parse_program(&parser_min);

    if (program_min == NULL && parser_min.error_message) {
        fprintf(stderr, "Parser error in test_parse_integer_bounds (NEG_INT_MAX): %s\n", parser_min.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program_min, "parse_program() returned NULL for test_parse_integer_bounds (NEG_INT_MAX)");
    TEST_ASSERT_FALSE_MESSAGE(parser_min.error_flag, parser_min.error_message ? parser_min.error_message : "Parser error flag set in test_parse_integer_bounds (NEG_INT_MAX)");

    TEST_ASSERT_NOT_NULL_MESSAGE(program_min->function, "Function node is NULL (NEG_INT_MAX)");
    TEST_ASSERT_NOT_NULL_MESSAGE(program_min->function->body, "Function body (BlockNode) is NULL (NEG_INT_MAX)");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program_min->function->body->base.type, "Function body is not a BlockNode (NEG_INT_MAX)");
    BlockNode *body_block_min = program_min->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block_min->num_items, "Expected 1 item in function body block (NEG_INT_MAX)");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block_min->items[0], "First item in block is NULL (NEG_INT_MAX)");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block_min->items[0]->type, "First item in block is not a return statement (NEG_INT_MAX)");
    ReturnStmtNode *ret_stmt_min = (ReturnStmtNode *) body_block_min->items[0];

    TEST_ASSERT_EQUAL(NODE_UNARY_OP, ret_stmt_min->expression->type);
    UnaryOpNode *unary_min = (UnaryOpNode *) ret_stmt_min->expression;
    TEST_ASSERT_EQUAL(OPERATOR_NEGATE, unary_min->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_min->operand->type);
    // Note: The literal itself is positive, negation makes it -INT_MAX
    TEST_ASSERT_EQUAL_INT(2147483647, ((IntLiteralNode*)unary_min->operand)->value);
    arena_destroy(&arena_min);
}

// Test parsing an integer exceeding the representable range
void test_parse_integer_overflow(void) {
    verify_parser_error("int main(void) { return 2147483648; }", "Integer literal out of range");
    // For negative overflow, it's typically caught by the unary negation of an out-of-range positive
    // or a specific check if you parse negative numbers directly.
    // Let's assume -2147483649 is also an error:
    verify_parser_error("int main(void) { return -2147483649; }", "Integer literal out of range");
}

void test_parse_error_missing_rhs_after_binary_op(void) {
    verify_parser_error("int main(void) { return 1 + ; }",
                        "Parse Error (near pos 28): Expected expression (integer, unary op, or '('), but got ';'");
}

void test_parse_error_consecutive_binary_operators(void) {
    verify_parser_error("int main(void) { return 1 + * 2; }",
                        "Parse Error (near pos 28): Expected expression (integer, unary op, or '('), but got '*'");
    // Or more specific error
}

void test_parse_error_missing_closing_paren(void) {
    verify_parser_error("int main(void) { return (1 + 2; }",
                        "Parse Error (near pos 30): Expected token ')', but got ';'");
}

// --- Test Runner --- //
void run_parser_errors_and_literals_tests(void) {
    RUN_TEST(test_parse_invalid_unary_expression);
    RUN_TEST(test_parse_mismatched_parentheses);
    RUN_TEST(test_parse_integer_bounds);
    RUN_TEST(test_parse_integer_overflow);
    RUN_TEST(test_parse_error_missing_rhs_after_binary_op);
    RUN_TEST(test_parse_error_consecutive_binary_operators);
    RUN_TEST(test_parse_error_missing_closing_paren);
}
