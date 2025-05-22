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

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_parse_simple_addition: %s\n", parser.error_message);
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL. Check stderr for parser error message if available.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set but no specific message available.");
    FuncDefNode *func_def_add = program->function;
    TEST_ASSERT_NOT_NULL(func_def_add->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_add->body->base.type);
    BlockNode *body_block_add = func_def_add->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_add->num_items);
    TEST_ASSERT_NOT_NULL(body_block_add->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_add->items[0]->type);
    ReturnStmtNode *return_stmt_add = (ReturnStmtNode *) body_block_add->items[0];
    verify_binary_op_node(return_stmt_add->expression, OPERATOR_ADD, 1, 2);
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
    FuncDefNode *func_def_sub = program->function;
    TEST_ASSERT_NOT_NULL(func_def_sub->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_sub->body->base.type);
    BlockNode *body_block_sub = func_def_sub->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_sub->num_items);
    TEST_ASSERT_NOT_NULL(body_block_sub->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_sub->items[0]->type);
    ReturnStmtNode *return_stmt_sub = (ReturnStmtNode *) body_block_sub->items[0];
    verify_binary_op_node(return_stmt_sub->expression, OPERATOR_SUBTRACT, 3, 1);
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
    FuncDefNode *func_def_mul = program->function;
    TEST_ASSERT_NOT_NULL(func_def_mul->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_mul->body->base.type);
    BlockNode *body_block_mul = func_def_mul->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_mul->num_items);
    TEST_ASSERT_NOT_NULL(body_block_mul->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_mul->items[0]->type);
    ReturnStmtNode *return_stmt_mul = (ReturnStmtNode *) body_block_mul->items[0];
    verify_binary_op_node(return_stmt_mul->expression, OPERATOR_MULTIPLY, 2, 3);
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
    FuncDefNode *func_def_div = program->function;
    TEST_ASSERT_NOT_NULL(func_def_div->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_div->body->base.type);
    BlockNode *body_block_div = func_def_div->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_div->num_items);
    TEST_ASSERT_NOT_NULL(body_block_div->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_div->items[0]->type);
    ReturnStmtNode *return_stmt_div = (ReturnStmtNode *) body_block_div->items[0];
    verify_binary_op_node(return_stmt_div->expression, OPERATOR_DIVIDE, 4, 2);
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
    FuncDefNode *func_def_mod = program->function;
    TEST_ASSERT_NOT_NULL(func_def_mod->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_mod->body->base.type);
    BlockNode *body_block_mod = func_def_mod->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_mod->num_items);
    TEST_ASSERT_NOT_NULL(body_block_mod->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_mod->items[0]->type);
    ReturnStmtNode *return_stmt_mod = (ReturnStmtNode *) body_block_mod->items[0];
    verify_binary_op_node(return_stmt_mod->expression, OPERATOR_MODULO, 5, 2);
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
    FuncDefNode *func_def_prec1 = program->function;
    TEST_ASSERT_NOT_NULL(func_def_prec1->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_prec1->body->base.type);
    BlockNode *body_block_prec1 = func_def_prec1->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_prec1->num_items);
    TEST_ASSERT_NOT_NULL(body_block_prec1->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_prec1->items[0]->type);
    ReturnStmtNode *return_stmt_prec1 = (ReturnStmtNode *) body_block_prec1->items[0];
    AstNode *expr_prec1 = return_stmt_prec1->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_prec1->type);
    BinaryOpNode *add_node_prec1 = (BinaryOpNode *) expr_prec1;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_node_prec1->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_node_prec1->left->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode *)add_node_prec1->left)->value);
    verify_binary_op_node(add_node_prec1->right, OPERATOR_MULTIPLY, 2, 3);
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
    FuncDefNode *func_def_prec2 = program->function;
    TEST_ASSERT_NOT_NULL(func_def_prec2->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_prec2->body->base.type);
    BlockNode *body_block_prec2 = func_def_prec2->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_prec2->num_items);
    TEST_ASSERT_NOT_NULL(body_block_prec2->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_prec2->items[0]->type);
    ReturnStmtNode *return_stmt_prec2 = (ReturnStmtNode *) body_block_prec2->items[0];
    AstNode *expr_prec2 = return_stmt_prec2->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_prec2->type);
    BinaryOpNode *add_node_prec2 = (BinaryOpNode *) expr_prec2;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_node_prec2->op);
    verify_binary_op_node(add_node_prec2->left, OPERATOR_MULTIPLY, 1, 2);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_node_prec2->right->type);
    TEST_ASSERT_EQUAL_INT(3, ((IntLiteralNode *)add_node_prec2->right)->value);
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
    FuncDefNode *func_def_assoc_sub = program->function;
    TEST_ASSERT_NOT_NULL(func_def_assoc_sub->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_assoc_sub->body->base.type);
    BlockNode *body_block_assoc_sub = func_def_assoc_sub->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_assoc_sub->num_items);
    TEST_ASSERT_NOT_NULL(body_block_assoc_sub->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_assoc_sub->items[0]->type);
    ReturnStmtNode *return_stmt_assoc_sub = (ReturnStmtNode *) body_block_assoc_sub->items[0];
    AstNode *expr_assoc_sub = return_stmt_assoc_sub->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_assoc_sub->type);
    BinaryOpNode *outer_sub_node = (BinaryOpNode *) expr_assoc_sub;
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
    FuncDefNode *func_def_assoc_div = program->function;
    TEST_ASSERT_NOT_NULL(func_def_assoc_div->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_assoc_div->body->base.type);
    BlockNode *body_block_assoc_div = func_def_assoc_div->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_assoc_div->num_items);
    TEST_ASSERT_NOT_NULL(body_block_assoc_div->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_assoc_div->items[0]->type);
    ReturnStmtNode *return_stmt_assoc_div = (ReturnStmtNode *) body_block_assoc_div->items[0];
    AstNode *expr_assoc_div = return_stmt_assoc_div->expression;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_assoc_div->type);
    BinaryOpNode *outer_div_node = (BinaryOpNode *) expr_assoc_div;
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
    FuncDefNode *func_def_paren_simple = program->function;
    TEST_ASSERT_NOT_NULL(func_def_paren_simple->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_paren_simple->body->base.type);
    BlockNode *body_block_paren_simple = func_def_paren_simple->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_paren_simple->num_items);
    TEST_ASSERT_NOT_NULL(body_block_paren_simple->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_paren_simple->items[0]->type);
    ReturnStmtNode *return_stmt_paren_simple = (ReturnStmtNode *) body_block_paren_simple->items[0];
    AstNode *expr_paren_simple = return_stmt_paren_simple->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_paren_simple->type);
    BinaryOpNode *mul_node = (BinaryOpNode *) expr_paren_simple;
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
    FuncDefNode *func_def_paren_nested = program->function;
    TEST_ASSERT_NOT_NULL(func_def_paren_nested->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_paren_nested->body->base.type);
    BlockNode *body_block_paren_nested = func_def_paren_nested->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_paren_nested->num_items);
    TEST_ASSERT_NOT_NULL(body_block_paren_nested->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_paren_nested->items[0]->type);
    ReturnStmtNode *return_stmt_paren_nested = (ReturnStmtNode *) body_block_paren_nested->items[0];
    AstNode *expr_paren_nested = return_stmt_paren_nested->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_paren_nested->type);
    BinaryOpNode *sub_node = (BinaryOpNode *) expr_paren_nested; // Outermost is subtraction
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

    FuncDefNode *func_def_unary_bin = program->function;
    TEST_ASSERT_NOT_NULL(func_def_unary_bin->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_unary_bin->body->base.type);
    BlockNode *body_block_unary_bin = func_def_unary_bin->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_unary_bin->num_items);
    TEST_ASSERT_NOT_NULL(body_block_unary_bin->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_unary_bin->items[0]->type);
    ReturnStmtNode *return_stmt_unary_bin = (ReturnStmtNode *) body_block_unary_bin->items[0];
    AstNode *expr_unary_bin = return_stmt_unary_bin->expression;

    TEST_ASSERT_NOT_NULL_MESSAGE(expr_unary_bin, "Expression is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, expr_unary_bin->type, "Expression is not a binary operation");

    BinaryOpNode *bin_op_node = (BinaryOpNode *) expr_unary_bin;
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

    FuncDefNode *func_def_unary_paren = program->function;
    TEST_ASSERT_NOT_NULL(func_def_unary_paren->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_unary_paren->body->base.type);
    BlockNode *body_block_unary_paren = func_def_unary_paren->body;
    TEST_ASSERT_EQUAL_INT(1, body_block_unary_paren->num_items);
    TEST_ASSERT_NOT_NULL(body_block_unary_paren->items[0]);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, body_block_unary_paren->items[0]->type);
    ReturnStmtNode *return_stmt_unary_paren = (ReturnStmtNode *) body_block_unary_paren->items[0];
    AstNode *expr_unary_paren = return_stmt_unary_paren->expression;

    TEST_ASSERT_NOT_NULL_MESSAGE(expr_unary_paren, "Expression is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_UNARY_OP, expr_unary_paren->type, "Expression is not a unary operation");

    UnaryOpNode *unary_op_node = (UnaryOpNode *) expr_unary_paren;
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
