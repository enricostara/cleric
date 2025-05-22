#include <string.h>
#include <stdio.h>

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

// Helper function to verify a unary operation node with an int literal operand
static void verify_unary_op_int_operand_node(AstNode *node, UnaryOperatorType expected_op, int expected_value) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);

    UnaryOpNode *unary_node = (UnaryOpNode *) node;
    TEST_ASSERT_EQUAL(expected_op, unary_node->op);
    TEST_ASSERT_NOT_NULL(unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);

    IntLiteralNode *int_node = (IntLiteralNode *) unary_node->operand;
    TEST_ASSERT_EQUAL_INT(expected_value, int_node->value);
}

// --- Test Cases for Complex Precedence and Associativity --- //

void test_parse_complex_nested_expression(void) {
    const char *input = "int main(void) { return -((1 + 2 * 3) / (!0 && (4 > 2 || 5 != 5))); }";
    // Expected structure: -( (1 + (2*3)) / ( (!0) && ( (4>2) || (5!=5) ) ) )

    Arena test_arena = arena_create(4096);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_parse_complex_nested_expression: %s\n", parser.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL for test_parse_complex_nested_expression");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set in test_parse_complex_nested_expression");

    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "Function node is NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function->body, "Function body (BlockNode) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program->function->body->base.type, "Function body is not a BlockNode");
    BlockNode *body_block = program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block->num_items, "Expected 1 item in function body block");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block->items[0], "First item in block is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block->items[0]->type, "First item in block is not a return statement");
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) body_block->items[0];
    AstNode *expr = return_stmt->expression;

    // Root: Unary Negation: -(...)
    TEST_ASSERT_EQUAL_MESSAGE(NODE_UNARY_OP, expr->type, "Root is not UnaryOp");
    UnaryOpNode *neg_node = (UnaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_NEGATE, neg_node->op, "Root op is not NEGATION");

    // Operand of Negation: Division: (1 + 2 * 3) / (...)
    AstNode *div_expr = neg_node->operand;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, div_expr->type, "Negation operand is not BinaryOp (Division)");
    BinaryOpNode *div_node = (BinaryOpNode *) div_expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_DIVIDE, div_node->op, "Division op is not DIVIDE");

    // Left of Division: Addition: (1 + 2 * 3)
    AstNode *add_expr_node = div_node->left;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, add_expr_node->type, "Div left is not BinaryOp (Addition)");
    BinaryOpNode *add_node = (BinaryOpNode *) add_expr_node;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_ADD, add_node->op, "Add op is not ADD");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, add_node->left->type, "Add left is not IntLiteral");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ((IntLiteralNode *)add_node->left)->value, "Add left value mismatch");
    verify_binary_op_node(add_node->right, OPERATOR_MULTIPLY, 2, 3); // Right of Addition: (2 * 3)

    // Right of Division: Logical AND: (!0 && (...))
    AstNode *and_expr_node = div_node->right;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, and_expr_node->type, "Div right is not BinaryOp (AND)");
    BinaryOpNode *and_node = (BinaryOpNode *) and_expr_node;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_AND, and_node->op, "AND op is not LOGICAL_AND");

    // Left of AND: Logical NOT: !0
    verify_unary_op_int_operand_node(and_node->left, OPERATOR_LOGICAL_NOT, 0);

    // Right of AND: Parenthesized Logical OR: (4 > 2 || 5 != 5)
    AstNode *or_expr_node = and_node->right;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, or_expr_node->type, "AND right is not BinaryOp (OR)");
    BinaryOpNode *or_node = (BinaryOpNode *) or_expr_node;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_OR, or_node->op, "OR op is not LOGICAL_OR");
    verify_binary_op_node(or_node->left, OPERATOR_GREATER, 4, 2); // Left of OR: 4 > 2
    verify_binary_op_node(or_node->right, OPERATOR_NOT_EQUAL, 5, 5); // Right of OR: 5 != 5

    arena_destroy(&test_arena);
}

// Test: 1 || 0 && 1  =>  1 || (0 && 1)
void test_precedence_logical_or_and(void) {
    const char *input = "int main(void) { return 1 || 0 && 1; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_precedence_logical_or_and: %s\n", parser.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL for test_precedence_logical_or_and");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set in test_precedence_logical_or_and");

    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "Function node is NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function->body, "Function body (BlockNode) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program->function->body->base.type, "Function body is not a BlockNode");
    BlockNode *body_block = program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block->num_items, "Expected 1 item in function body block");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block->items[0], "First item in block is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block->items[0]->type, "First item in block is not a return statement");
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) body_block->items[0];
    AstNode *expr = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *or_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_LOGICAL_OR, or_node->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, or_node->left->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode *) or_node->left)->value);
    verify_binary_op_node(or_node->right, OPERATOR_LOGICAL_AND, 0, 1);
    arena_destroy(&test_arena);
}

// Test: 1 < 2 && 3 > 1  =>  (1 < 2) && (3 > 1)
void test_precedence_relational_and_logical(void) {
    const char *input = "int main(void) { return 1 < 2 && 3 > 1; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_precedence_relational_and_logical: %s\n", parser.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL for test_precedence_relational_and_logical");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set in test_precedence_relational_and_logical");

    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "Function node is NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function->body, "Function body (BlockNode) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program->function->body->base.type, "Function body is not a BlockNode");
    BlockNode *body_block = program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block->num_items, "Expected 1 item in function body block");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block->items[0], "First item in block is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block->items[0]->type, "First item in block is not a return statement");
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) body_block->items[0];
    AstNode *expr = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *and_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_LOGICAL_AND, and_node->op);
    verify_binary_op_node(and_node->left, OPERATOR_LESS, 1, 2);
    verify_binary_op_node(and_node->right, OPERATOR_GREATER, 3, 1);
    arena_destroy(&test_arena);
}

// Test: 1 + 2 < 4 && 5 > 3 - 1  =>  ((1 + 2) < 4) && (5 > (3 - 1))
void test_precedence_arithmetic_relational_logical(void) {
    const char *input = "int main(void) { return 1 + 2 < 4 && 5 > 3 - 1; }";
    Arena test_arena = arena_create(2048);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_precedence_arithmetic_relational_logical: %s\n", parser.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL for test_precedence_arithmetic_relational_logical");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set in test_precedence_arithmetic_relational_logical");

    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "Function node is NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function->body, "Function body (BlockNode) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program->function->body->base.type, "Function body is not a BlockNode");
    BlockNode *body_block = program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block->num_items, "Expected 1 item in function body block");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block->items[0], "First item in block is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block->items[0]->type, "First item in block is not a return statement");
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) body_block->items[0];
    AstNode *expr = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type); // Outermost is &&
    BinaryOpNode *and_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_LOGICAL_AND, and_node->op);

    // Left of && : (1 + 2) < 4
    AstNode *left_of_and = and_node->left;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, left_of_and->type);
    BinaryOpNode *less_node = (BinaryOpNode *) left_of_and;
    TEST_ASSERT_EQUAL(OPERATOR_LESS, less_node->op);
    verify_binary_op_node(less_node->left, OPERATOR_ADD, 1, 2); // (1+2)
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, less_node->right->type);
    TEST_ASSERT_EQUAL_INT(4, ((IntLiteralNode *) less_node->right)->value); // 4

    // Right of && : 5 > (3 - 1)
    AstNode *right_of_and = and_node->right;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, right_of_and->type);
    BinaryOpNode *greater_node = (BinaryOpNode *) right_of_and;
    TEST_ASSERT_EQUAL(OPERATOR_GREATER, greater_node->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, greater_node->left->type);
    TEST_ASSERT_EQUAL_INT(5, ((IntLiteralNode *) greater_node->left)->value); // 5
    verify_binary_op_node(greater_node->right, OPERATOR_SUBTRACT, 3, 1); // (3-1)

    arena_destroy(&test_arena);
}

// Test: !0 && 1  =>  (!0) && 1
void test_precedence_unary_not_with_logical(void) {
    const char *input = "int main(void) { return !0 && 1; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_precedence_unary_not_with_logical: %s\n", parser.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL for test_precedence_unary_not_with_logical");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set in test_precedence_unary_not_with_logical");

    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "Function node is NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function->body, "Function body (BlockNode) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program->function->body->base.type, "Function body is not a BlockNode");
    BlockNode *body_block = program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block->num_items, "Expected 1 item in function body block");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block->items[0], "First item in block is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block->items[0]->type, "First item in block is not a return statement");
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) body_block->items[0];
    AstNode *expr = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr->type);
    BinaryOpNode *and_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_LOGICAL_AND, and_node->op);
    verify_unary_op_int_operand_node(and_node->left, OPERATOR_LOGICAL_NOT, 0); // !0
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, and_node->right->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode *) and_node->right)->value); // 1
    arena_destroy(&test_arena);
}

// Test: !(1 < 2 && 0)  =>  !((1 < 2) && 0)
void test_precedence_unary_not_on_parenthesized_logical(void) {
    const char *input = "int main(void) { return !(1 < 2 && 0); }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_precedence_unary_not_on_parenthesized_logical: %s\n", parser.error_message);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL for test_precedence_unary_not_on_parenthesized_logical");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set in test_precedence_unary_not_on_parenthesized_logical");

    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "Function node is NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function->body, "Function body (BlockNode) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, program->function->body->base.type, "Function body is not a BlockNode");
    BlockNode *body_block = program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block->num_items, "Expected 1 item in function body block");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block->items[0], "First item in block is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, body_block->items[0]->type, "First item in block is not a return statement");
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) body_block->items[0];
    AstNode *expr = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_UNARY_OP, expr->type); // Outermost is !
    UnaryOpNode *not_node = (UnaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_LOGICAL_NOT, not_node->op);

    // Operand of ! : (1 < 2 && 0)
    AstNode *operand_of_not = not_node->operand;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, operand_of_not->type);
    BinaryOpNode *and_node = (BinaryOpNode *) operand_of_not;
    TEST_ASSERT_EQUAL(OPERATOR_LOGICAL_AND, and_node->op);
    verify_binary_op_node(and_node->left, OPERATOR_LESS, 1, 2); // 1 < 2
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, and_node->right->type);
    TEST_ASSERT_EQUAL_INT(0, ((IntLiteralNode *) and_node->right)->value); // 0
    arena_destroy(&test_arena);
}

// --- Test Runner --- //
void run_parser_precedence_tests(void) {
    RUN_TEST(test_parse_complex_nested_expression);
    RUN_TEST(test_precedence_logical_or_and);
    RUN_TEST(test_precedence_relational_and_logical);
    RUN_TEST(test_precedence_arithmetic_relational_logical);
    RUN_TEST(test_precedence_unary_not_with_logical);
    RUN_TEST(test_precedence_unary_not_on_parenthesized_logical);
}
