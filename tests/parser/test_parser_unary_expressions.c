#include <string.h>

#include "unity.h"
#include "../../src/lexer/lexer.h"      // Adjusted path
#include "../../src/parser/parser.h"    // Adjusted path
#include "../../src/parser/ast.h"       // Adjusted path
#include "../../src/memory/arena.h"   // Adjusted path

// --- Test Helper Functions ---

// Helper function to verify a unary operation node
static void verify_unary_op_node(AstNode *node, UnaryOperatorType expected_op, int expected_value) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);

    UnaryOpNode *unary_node = (UnaryOpNode *) node;
    TEST_ASSERT_EQUAL(expected_op, unary_node->op);
    TEST_ASSERT_NOT_NULL(unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);

    IntLiteralNode *int_node = (IntLiteralNode *) unary_node->operand;
    TEST_ASSERT_EQUAL_INT(expected_value, int_node->value);
}

// Helper function to verify a nested unary operation
static void verify_nested_unary_op(AstNode *node, UnaryOperatorType outer_op, UnaryOperatorType inner_op, int value) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);

    UnaryOpNode *outer_node = (UnaryOpNode *) node;
    TEST_ASSERT_EQUAL(outer_op, outer_node->op);
    TEST_ASSERT_NOT_NULL(outer_node->operand);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, outer_node->operand->type);

    UnaryOpNode *inner_node = (UnaryOpNode *) outer_node->operand;
    TEST_ASSERT_EQUAL(inner_op, inner_node->op);
    TEST_ASSERT_NOT_NULL(inner_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, inner_node->operand->type);

    IntLiteralNode *int_node = (IntLiteralNode *) inner_node->operand;
    TEST_ASSERT_EQUAL_INT(value, int_node->value);
}

// --- Test Cases --- //

// Test parsing a program with a negation unary operator
void test_parse_negation_operator(void) {
    const char *input = "int main(void) { return -42; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL(func_def->body);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;

    verify_unary_op_node(return_stmt->expression, OPERATOR_NEGATE, 42);
    arena_destroy(&test_arena);
}

// Test parsing a program with a bitwise complement unary operator
void test_parse_complement_operator(void) {
    const char *input = "int main(void) { return ~42; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL(func_def->body);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;

    verify_unary_op_node(return_stmt->expression, OPERATOR_COMPLEMENT, 42);
    arena_destroy(&test_arena);
}

// Test parsing a program with a logical NOT unary operator
void test_parse_logical_not_operator(void) {
    const char *input = "int main(void) { return !42; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL(func_def->body);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;

    verify_unary_op_node(return_stmt->expression, OPERATOR_LOGICAL_NOT, 42);
    arena_destroy(&test_arena);
}

// Test parsing a program with nested unary operators
void test_parse_nested_unary_operators(void) {
    const char *input = "int main(void) { return !-42; }"; // Logical NOT of a negated number
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL(func_def->body);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;

    verify_nested_unary_op(return_stmt->expression, OPERATOR_LOGICAL_NOT, OPERATOR_NEGATE, 42);
    arena_destroy(&test_arena);
}

// Test parsing a program with parenthesized expression
void test_parse_parenthesized_expression(void) {
    const char *input = "int main(void) { return (42); }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL(func_def->body);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, return_stmt->expression->type);
    IntLiteralNode *int_node = (IntLiteralNode *) return_stmt->expression;
    TEST_ASSERT_EQUAL_INT(42, int_node->value);
    arena_destroy(&test_arena);
}

// Test parsing a program with unary operator and parentheses
void test_parse_unary_with_parentheses(void) {
    const char *input = "int main(void) { return -(42); }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL(func_def->body);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;

    verify_unary_op_node(return_stmt->expression, OPERATOR_NEGATE, 42);
    arena_destroy(&test_arena);
}

// --- Test Runner --- //
void run_parser_unary_expressions_tests(void) {
    RUN_TEST(test_parse_negation_operator);
    RUN_TEST(test_parse_complement_operator);
    RUN_TEST(test_parse_logical_not_operator);
    RUN_TEST(test_parse_nested_unary_operators);
    RUN_TEST(test_parse_parenthesized_expression);
    RUN_TEST(test_parse_unary_with_parentheses);
}
