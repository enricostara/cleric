#include "unity/unity.h"
#include "../src/parser/ast.h"
#include "../src/memory/arena.h"

// Test case for creating and checking an integer literal node
static void test_create_int_literal(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *node = create_int_literal_node(42, &test_arena);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, node->base.type);
    TEST_ASSERT_EQUAL_INT64(42L, node->value);
    arena_destroy(&test_arena);
}

// Test case for creating a return statement node
static void test_create_return_stmt(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *expr_node = create_int_literal_node(5, &test_arena);
    TEST_ASSERT_NOT_NULL(expr_node);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) expr_node, &test_arena);
    TEST_ASSERT_NOT_NULL(return_node);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, return_node->base.type);
    const ReturnStmtNode *ret_node = (ReturnStmtNode *) return_node;
    TEST_ASSERT_EQUAL_PTR(expr_node, ret_node->expression);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, ret_node->expression->type);
    arena_destroy(&test_arena);
}

// Test case for creating a function definition node
static void test_create_func_def(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *expr = create_int_literal_node(2, &test_arena);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) expr, &test_arena);
    TEST_ASSERT_NOT_NULL(ret_stmt);
    FuncDefNode *func_def_node = create_func_def_node("main", (AstNode *) ret_stmt, &test_arena);
    TEST_ASSERT_NOT_NULL(func_def_node);
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def_node->base.type);
    const FuncDefNode *func_node = (FuncDefNode *) func_def_node;
    TEST_ASSERT_EQUAL_PTR(ret_stmt, func_node->body);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_node->body->type);
    arena_destroy(&test_arena);
}

// Test case for creating a full program node (simplified)
static void test_create_program(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *expr = create_int_literal_node(2, &test_arena);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) expr, &test_arena);
    AstNode *func_body = (AstNode *) ret_stmt;
    FuncDefNode *func_def = create_func_def_node("main", func_body, &test_arena);
    TEST_ASSERT_NOT_NULL(func_def);
    ProgramNode *program_node = create_program_node(func_def, &test_arena);
    TEST_ASSERT_NOT_NULL(program_node);
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program_node->base.type);
    TEST_ASSERT_EQUAL_PTR(func_def, (AstNode*)program_node->function);
    arena_destroy(&test_arena);
}

// Test case for creating a unary negate operation node
static void test_create_unary_op_negate(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *operand_node = create_int_literal_node(5, &test_arena);
    TEST_ASSERT_NOT_NULL(operand_node);
    UnaryOpNode *unary_node = create_unary_op_node(OPERATOR_NEGATE, (AstNode *) operand_node, &test_arena);
    TEST_ASSERT_NOT_NULL(unary_node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, unary_node->base.type);
    TEST_ASSERT_EQUAL(OPERATOR_NEGATE, unary_node->op);
    TEST_ASSERT_EQUAL_PTR(operand_node, unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);
    arena_destroy(&test_arena);
}

// Test case for creating a unary complement operation node
static void test_create_unary_op_complement(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *operand_node = create_int_literal_node(10, &test_arena);
    TEST_ASSERT_NOT_NULL(operand_node);
    UnaryOpNode *unary_node = create_unary_op_node(OPERATOR_COMPLEMENT, (AstNode *) operand_node, &test_arena);
    TEST_ASSERT_NOT_NULL(unary_node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, unary_node->base.type);
    TEST_ASSERT_EQUAL(OPERATOR_COMPLEMENT, unary_node->op);
    TEST_ASSERT_EQUAL_PTR(operand_node, unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);
    arena_destroy(&test_arena);
}

// Test case that calls ast_pretty_print (visual inspection needed)
static void test_ast_pretty_print_output(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // Build AST for: int main(void) { return ~5; } 
    IntLiteralNode *operand = create_int_literal_node(5, &test_arena);
    UnaryOpNode *unary_op = create_unary_op_node(OPERATOR_COMPLEMENT, (AstNode *) operand, &test_arena);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) unary_op, &test_arena);
    AstNode *func_body = (AstNode *) ret_stmt;
    FuncDefNode *func_def = create_func_def_node("main", func_body, &test_arena);
    ProgramNode *program_node = create_program_node(func_def, &test_arena);
    TEST_ASSERT_NOT_NULL(program_node);

    printf("\n--- AST Pretty Print Output Start ---\n");
    ast_pretty_print((AstNode *) program_node, 0);
    printf("--- AST Pretty Print Output End ---\n");
    arena_destroy(&test_arena);
    TEST_PASS(); // Visual inspection required for correctness
}

// Runner function for AST tests
void run_ast_tests(void) {
    RUN_TEST(test_create_int_literal);
    RUN_TEST(test_create_return_stmt);
    RUN_TEST(test_create_unary_op_negate);
    RUN_TEST(test_create_unary_op_complement);
    RUN_TEST(test_create_func_def);
    RUN_TEST(test_create_program);
    RUN_TEST(test_ast_pretty_print_output);
}
