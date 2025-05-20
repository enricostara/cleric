#include <string.h>

#include "unity.h"
#include "../../src/lexer/lexer.h"      // Adjusted path
#include "../../src/parser/parser.h"    // Adjusted path
#include "../../src/parser/ast.h"       // Adjusted path
#include "../../src/memory/arena.h"   // Adjusted path

// --- Test Helper Functions ---

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

// Helper function to verify a unary operation node (copied for test_parse_unary_on_parenthesized_expr)
static void verify_unary_op_node(AstNode *node, UnaryOperatorType expected_op, NodeType expected_operand_type) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);
    UnaryOpNode *unary_node = (UnaryOpNode *) node;
    TEST_ASSERT_EQUAL(expected_op, unary_node->op);
    TEST_ASSERT_NOT_NULL(unary_node->operand);
    TEST_ASSERT_EQUAL(expected_operand_type, unary_node->operand->type);
}


// --- Test Cases --- //

void test_parse_simple_addition(void) {
    const char *input = "int main(void) { return 1 + 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_ADD, 1, 2);
    arena_destroy(&test_arena);
}

void test_parse_simple_subtraction(void) {
    const char *input = "int main(void) { return 3 - 1; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_SUBTRACT, 3, 1);
    arena_destroy(&test_arena);
}

void test_parse_simple_multiplication(void) {
    const char *input = "int main(void) { return 2 * 3; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_MULTIPLY, 2, 3);
    arena_destroy(&test_arena);
}

void test_parse_simple_division(void) {
    const char *input = "int main(void) { return 4 / 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_DIVIDE, 4, 2);
    arena_destroy(&test_arena);
}

void test_parse_simple_modulo(void) {
    const char *input = "int main(void) { return 5 % 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_MODULO, 5, 2);
    arena_destroy(&test_arena);
}

void test_parse_precedence_add_mul(void) {
    const char *input = "int main(void) { return 1 + 2 * 3; }"; // Expected: 1 + (2 * 3)
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *add_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_node->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_node->left->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode *)add_node->left)->value);
    verify_binary_op_node(add_node->right, OPERATOR_MULTIPLY, 2, 3);
    arena_destroy(&test_arena);
}

void test_parse_precedence_mul_add(void) {
    const char *input = "int main(void) { return 1 * 2 + 3; }"; // Expected: (1 * 2) + 3
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *add_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_node->op);
    verify_binary_op_node(add_node->left, OPERATOR_MULTIPLY, 1, 2);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_node->right->type);
    TEST_ASSERT_EQUAL_INT(3, ((IntLiteralNode *)add_node->right)->value);
    arena_destroy(&test_arena);
}

void test_parse_associativity_subtract(void) {
    const char *input = "int main(void) { return 5 - 2 - 1; }"; // Expected: (5 - 2) - 1
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *outer_sub_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_SUBTRACT, outer_sub_node->op);
    verify_binary_op_node(outer_sub_node->left, OPERATOR_SUBTRACT, 5, 2);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, outer_sub_node->right->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode *)outer_sub_node->right)->value);
    arena_destroy(&test_arena);
}

void test_parse_associativity_divide(void) {
    const char *input = "int main(void) { return 8 / 4 / 2; }"; // Expected: (8 / 4) / 2
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *outer_div_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_DIVIDE, outer_div_node->op);
    verify_binary_op_node(outer_div_node->left, OPERATOR_DIVIDE, 8, 4);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, outer_div_node->right->type);
    TEST_ASSERT_EQUAL_INT(2, ((IntLiteralNode *)outer_div_node->right)->value);
    arena_destroy(&test_arena);
}

void test_parse_parentheses_simple(void) {
    const char *input = "int main(void) { return (1 + 2) * 3; }"; // Expected: (1 + 2) * 3
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *mul_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_MULTIPLY, mul_node->op);
    verify_binary_op_node(mul_node->left, OPERATOR_ADD, 1, 2);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, mul_node->right->type);
    TEST_ASSERT_EQUAL_INT(3, ((IntLiteralNode *)mul_node->right)->value);
    arena_destroy(&test_arena);
}

void test_parse_parentheses_nested(void) {
    const char *input = "int main(void) { return (1 + (2 * 3)) - 4; }"; // Expected: ((1 + (2 * 3)) - 4)
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *sub_node = (BinaryOpNode *) expr; // Outermost is subtraction
    TEST_ASSERT_EQUAL(OPERATOR_SUBTRACT, sub_node->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, sub_node->right->type);
    TEST_ASSERT_EQUAL_INT(4, ((IntLiteralNode*)sub_node->right)->value);

    AstNode *add_expr = sub_node->left; // Left of subtraction is (1 + (2*3))
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, add_expr->type);
    BinaryOpNode *add_node = (BinaryOpNode *) add_expr;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_node->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_node->left->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode*)add_node->left)->value);

    verify_binary_op_node(add_node->right, OPERATOR_MULTIPLY, 2, 3); // Right of add is (2*3)
    arena_destroy(&test_arena);
}


void test_parse_unary_with_binary_simple(void) {
    const char *input = "int main(void) { return -1 + 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program is NULL");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag set");

    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL_MESSAGE(func_def, "Function definition is NULL");
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(return_stmt, "Return statement is NULL");

    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_NOT_NULL_MESSAGE(expr, "Expression is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, expr->type, "Expression is not a binary operation");

    BinaryOpNode *bin_op_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_ADD, bin_op_node->op, "Binary operator is not ADD");

    // Left operand: Unary negation
    AstNode *left_operand = bin_op_node->left;
    TEST_ASSERT_NOT_NULL_MESSAGE(left_operand, "Left operand of ADD is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_UNARY_OP, left_operand->type, "Left operand of ADD is not UNARY_OP");
    UnaryOpNode *unary_node = (UnaryOpNode *) left_operand;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_NEGATE, unary_node->op, "Unary operator is not NEGATION");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, unary_node->operand->type, "Operand of NEGATION is not INT_LITERAL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ((IntLiteralNode *)unary_node->operand)->value,
                                  "Value of NEGATION operand is not 1");

    // Right operand: Integer literal
    AstNode *right_operand = bin_op_node->right;
    TEST_ASSERT_NOT_NULL_MESSAGE(right_operand, "Right operand of ADD is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, right_operand->type, "Right operand of ADD is not INT_LITERAL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ((IntLiteralNode *)right_operand)->value,
                                  "Value of right operand of ADD is not 2");

    arena_destroy(&test_arena);
}

void test_parse_unary_on_parenthesized_expr(void) {
    const char *input = "int main(void) { return -(1 + 2); }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program is NULL");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag set");

    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(return_stmt, "Return statement is NULL");

    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_NOT_NULL_MESSAGE(expr, "Expression is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_UNARY_OP, expr->type, "Expression is not a unary operation");

    UnaryOpNode *unary_op_node = (UnaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_NEGATE, unary_op_node->op, "Unary operator is not NEGATION");

    // Operand of unary: Binary addition (1 + 2)
    AstNode *operand = unary_op_node->operand;
    TEST_ASSERT_NOT_NULL_MESSAGE(operand, "Operand of NEGATION is NULL");
    verify_binary_op_node(operand, OPERATOR_ADD, 1, 2);

    arena_destroy(&test_arena);
}

// --- Test Runner --- //
void run_parser_binary_expressions_tests(void) {
    RUN_TEST(test_parse_simple_addition);
    RUN_TEST(test_parse_simple_subtraction);
    RUN_TEST(test_parse_simple_multiplication);
    RUN_TEST(test_parse_simple_division);
    RUN_TEST(test_parse_simple_modulo);
    RUN_TEST(test_parse_precedence_add_mul);
    RUN_TEST(test_parse_precedence_mul_add);
    RUN_TEST(test_parse_associativity_subtract);
    RUN_TEST(test_parse_associativity_divide);
    RUN_TEST(test_parse_parentheses_simple);
    RUN_TEST(test_parse_parentheses_nested);
    RUN_TEST(test_parse_unary_with_binary_simple);
    RUN_TEST(test_parse_unary_on_parenthesized_expr);
}
