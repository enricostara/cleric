#include "unity/unity.h"
#include "../src/parser/ast.h" // Adjust path as needed
#include <stdlib.h> // For NULL

// Test case for creating and checking an integer literal node
static void test_create_int_literal(void) {
    AstNode *node = create_int_literal_node(42);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, node->type);

    // Cast to access specific members
    IntLiteralNode *int_node = (IntLiteralNode *)node;
    TEST_ASSERT_EQUAL_INT64(42L, int_node->value);

    // Free the single node
    free_ast(node);
}

// Test case for creating a return statement node
static void test_create_return_stmt(void) {
    // Create the expression node first
    AstNode *expr_node = create_int_literal_node(5);
    TEST_ASSERT_NOT_NULL(expr_node);

    // Create the return statement node, passing ownership of expr_node
    AstNode *return_node = create_return_stmt_node(expr_node);
    TEST_ASSERT_NOT_NULL(return_node);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, return_node->type);

    // Cast to access specific members
    ReturnStmtNode *ret_node = (ReturnStmtNode *)return_node;
    TEST_ASSERT_EQUAL_PTR(expr_node, ret_node->expression); // Check if the pointer is assigned
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, ret_node->expression->type); // Check type of expression

    // Free the return node (which should free the expression node)
    free_ast(return_node);
}

// Test case for creating a function definition node
static void test_create_func_def(void) {
    AstNode *expr = create_int_literal_node(2);
    AstNode *ret_stmt = create_return_stmt_node(expr);
    TEST_ASSERT_NOT_NULL(ret_stmt);

    // Create function definition node, passing ownership of the body
    AstNode *func_def_node = create_func_def_node(ret_stmt);
    TEST_ASSERT_NOT_NULL(func_def_node);
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def_node->type);

    FuncDefNode *func_node = (FuncDefNode *)func_def_node;
    TEST_ASSERT_EQUAL_PTR(ret_stmt, func_node->body);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_node->body->type);

    // Free the function definition (should free the body recursively)
    free_ast(func_def_node);
}

// Test case for creating a full program node (simplified)
static void test_create_program(void) {
    AstNode *expr = create_int_literal_node(2);
    AstNode *ret_stmt = create_return_stmt_node(expr);
    AstNode *func_body = ret_stmt; // Body is just the return statement
    AstNode *func_def = create_func_def_node(func_body);
    TEST_ASSERT_NOT_NULL(func_def);

    // Create the program node, passing ownership of the function definition
    ProgramNode *program_node = create_program_node((FuncDefNode *)func_def);
    TEST_ASSERT_NOT_NULL(program_node);
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program_node->base.type);
    TEST_ASSERT_EQUAL_PTR(func_def, (AstNode*)program_node->function);

    // Free the entire program AST
    free_ast((AstNode *)program_node);
}

// Runner function for AST tests
void run_ast_tests(void) {
    RUN_TEST(test_create_int_literal);
    RUN_TEST(test_create_return_stmt);
    RUN_TEST(test_create_func_def);
    RUN_TEST(test_create_program);
    // Note: Testing free_ast thoroughly requires tools like valgrind.
    // These tests implicitly check that free_ast doesn't crash on valid trees.
}
