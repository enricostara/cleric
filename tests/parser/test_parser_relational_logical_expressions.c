#include <string.h>

#include "unity.h"
#include "../../src/lexer/lexer.h"
#include "../../src/parser/parser.h"
#include "../../src/parser/ast.h"
#include "../../src/memory/arena.h"

// --- Test Helper Function ---

// Helper function to verify a binary operation node where both operands are int literals
static void verify_binary_op_node(AstNode *node, BinaryOperatorType expected_op, int expected_left_val,
                                  int expected_right_val) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "BinaryOpNode is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, node->type, "Node type is not NODE_BINARY_OP");

    BinaryOpNode *bin_node = (BinaryOpNode *) node;
    TEST_ASSERT_EQUAL_MESSAGE(expected_op, bin_node->op, "Binary operator type mismatch");

    TEST_ASSERT_NOT_NULL_MESSAGE(bin_node->left, "Left operand is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, bin_node->left->type, "Left operand is not NODE_INT_LITERAL");
    IntLiteralNode *left_int_node = (IntLiteralNode *) bin_node->left;
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_left_val, left_int_node->value, "Left operand value mismatch");

    TEST_ASSERT_NOT_NULL_MESSAGE(bin_node->right, "Right operand is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, bin_node->right->type, "Right operand is not NODE_INT_LITERAL");
    IntLiteralNode *right_int_node = (IntLiteralNode *) bin_node->right;
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_right_val, right_int_node->value, "Right operand value mismatch");
}

// --- Test Cases for Relational and Logical Operators --- //

void test_parse_relational_less_than(void) {
    const char *input = "int main(void) { return 1 < 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_LESS, 1, 2);
    arena_destroy(&test_arena);
}

void test_parse_relational_greater_than(void) {
    const char *input = "int main(void) { return 3 > 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_GREATER, 3, 2);
    arena_destroy(&test_arena);
}

void test_parse_relational_less_equal(void) {
    const char *input = "int main(void) { return 1 <= 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_LESS_EQUAL, 1, 2);
    arena_destroy(&test_arena);
}

void test_parse_relational_greater_equal(void) {
    const char *input = "int main(void) { return 3 >= 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_GREATER_EQUAL, 3, 2);
    arena_destroy(&test_arena);
}

void test_parse_relational_equal_equal(void) {
    const char *input = "int main(void) { return 2 == 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_EQUAL_EQUAL, 2, 2);
    arena_destroy(&test_arena);
}

void test_parse_relational_not_equal(void) {
    const char *input = "int main(void) { return 1 != 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_NOT_EQUAL, 1, 2);
    arena_destroy(&test_arena);
}

void test_parse_logical_and(void) {
    const char *input = "int main(void) { return 1 && 0; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_LOGICAL_AND, 1, 0);
    arena_destroy(&test_arena);
}

void test_parse_logical_or(void) {
    const char *input = "int main(void) { return 1 || 0; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_LOGICAL_OR, 1, 0);
    arena_destroy(&test_arena);
}

// --- Test Runner --- //
void run_parser_relational_logical_expressions_tests(void) {
    RUN_TEST(test_parse_relational_less_than);
    RUN_TEST(test_parse_relational_greater_than);
    RUN_TEST(test_parse_relational_less_equal);
    RUN_TEST(test_parse_relational_greater_equal);
    RUN_TEST(test_parse_relational_equal_equal);
    RUN_TEST(test_parse_relational_not_equal);
    RUN_TEST(test_parse_logical_and);
    RUN_TEST(test_parse_logical_or);
}
