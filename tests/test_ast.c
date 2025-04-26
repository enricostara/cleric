#include "unity/unity.h"
#include "../src/parser/ast.h" // Adjust path as needed

// Test case for creating and checking an integer literal node
static void test_create_int_literal(void) {
    IntLiteralNode *node = create_int_literal_node(42);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, node->base.type);

    // Cast to access specific members
    const IntLiteralNode *int_node = node;
    TEST_ASSERT_EQUAL_INT64(42L, int_node->value);

    // Free the single node
    free_ast((AstNode *) node);
}

// Test case for creating a return statement node
static void test_create_return_stmt(void) {
    // Create the expression node first
    IntLiteralNode *expr_node = create_int_literal_node(5);
    TEST_ASSERT_NOT_NULL(expr_node);

    // Create the return statement node, passing ownership of expr_node
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) expr_node);
    TEST_ASSERT_NOT_NULL(return_node);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, return_node->base.type);

    // Cast to access specific members
    const ReturnStmtNode *ret_node = (ReturnStmtNode *) return_node;
    TEST_ASSERT_EQUAL_PTR(expr_node, ret_node->expression); // Check if the pointer is assigned
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, ret_node->expression->type); // Check type of expression

    // Free the return node (which should free the expression node)
    free_ast((AstNode *) return_node);
}

// Test case for creating a function definition node
static void test_create_func_def(void) {
    IntLiteralNode *expr = create_int_literal_node(2);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) expr);
    TEST_ASSERT_NOT_NULL(ret_stmt);

    // Create function definition node, passing ownership of the body
    FuncDefNode *func_def_node = create_func_def_node("main", (AstNode *) ret_stmt);
    TEST_ASSERT_NOT_NULL(func_def_node);
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def_node->base.type);

    const FuncDefNode *func_node = (FuncDefNode *) func_def_node;
    TEST_ASSERT_EQUAL_PTR(ret_stmt, func_node->body);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_node->body->type);

    // Free the function definition (should free the body recursively)
    free_ast((AstNode *) func_def_node);
}

// Test case for creating a full program node (simplified)
static void test_create_program(void) {
    IntLiteralNode *expr = create_int_literal_node(2);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) expr);
    AstNode *func_body = (AstNode *) ret_stmt; // Body is just the return statement
    FuncDefNode *func_def = create_func_def_node("main", func_body);
    TEST_ASSERT_NOT_NULL(func_def);

    // Create the program node, passing ownership of the function definition
    ProgramNode *program_node = create_program_node(func_def);
    TEST_ASSERT_NOT_NULL(program_node);
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program_node->base.type);
    TEST_ASSERT_EQUAL_PTR(func_def, (AstNode*)program_node->function);

    // Free the entire program AST
    free_ast((AstNode *) program_node);
}

// Test case that calls ast_pretty_print (visual inspection needed)
static void test_ast_pretty_print_output(void) {
    // Build the same AST as in test_create_program
    IntLiteralNode *expr = create_int_literal_node(2);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) expr);
    AstNode *func_body = (AstNode *) ret_stmt;
    FuncDefNode *func_def = create_func_def_node("main", func_body);
    ProgramNode *program_node = create_program_node(func_def);
    TEST_ASSERT_NOT_NULL(program_node);

    // Print the AST - user should visually check the output
    printf("\n--- AST Pretty Print Output Start ---\n");
    ast_pretty_print((AstNode *) program_node, 0);
    printf("--- AST Pretty Print Output End ---\n");

    // Free the AST
    free_ast((AstNode *) program_node);

    // This test mainly checks that pretty_print runs without errors.
    // Pass if no crash occurs. Output verification is manual.
    TEST_PASS();
}

// Runner function for AST tests
void run_ast_tests(void) {
    RUN_TEST(test_create_int_literal);
    RUN_TEST(test_create_return_stmt);
    RUN_TEST(test_create_func_def);
    RUN_TEST(test_create_program);
    RUN_TEST(test_ast_pretty_print_output);
    // Note: Testing free_ast thoroughly requires tools like valgrind.
    // These tests implicitly check that free_ast doesn't crash on valid trees.
}
