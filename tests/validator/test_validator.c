#include "unity.h"
#include "validator/validator.h"
#include "parser/ast.h"
#include "memory/arena.h"
#include "lexer/lexer.h" // For Token in AST nodes if needed, or for dummy tokens
#include "strings/strings.h" // For arena_strdup if needed for names

#include <string.h>

// Forward declarations for test functions
static void test_validate_empty_function(void);
static void test_validate_undeclared_variable(void);
static void test_validate_redeclared_variable_same_scope(void);
static void test_validate_shadowed_variable_inner_scope(void);
static void test_validate_valid_variable_usage(void);
static void test_validate_invalid_assignment_lvalue(void);

// Test Suite Runner
void run_validator_tests(void) {
    RUN_TEST(test_validate_empty_function);
    RUN_TEST(test_validate_undeclared_variable);
    RUN_TEST(test_validate_redeclared_variable_same_scope);
    RUN_TEST(test_validate_shadowed_variable_inner_scope);
    RUN_TEST(test_validate_valid_variable_usage);
    RUN_TEST(test_validate_invalid_assignment_lvalue);
    // Add more tests here
}

// --- Test Cases --- 

static void test_validate_empty_function(void) {
    // Arrange: Create an arena and a simple AST for "int main() { return 0; }"
    Arena test_arena = arena_create(1024 * 4);

    // ProgramNode -> FuncDefNode -> BlockNode -> ReturnStmtNode -> IntLiteralNode

    // IntLiteralNode for 0
    IntLiteralNode *int_literal = create_int_literal_node(0, &test_arena);

    // ReturnStmtNode
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) int_literal, &test_arena);

    // BlockNode
    BlockNode *block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode *) return_stmt, &test_arena);

    // FuncDefNode for "main"
    FuncDefNode *func_def = create_func_def_node("main", (BlockNode*)block_node, &test_arena);

    // ProgramNode
    ProgramNode *program_node = create_program_node(func_def, &test_arena);

    // Act: Validate the program
    bool is_valid = validate_program((AstNode *) program_node, &test_arena);

    // Assert: The program should be valid
    TEST_ASSERT_TRUE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_undeclared_variable(void) {
    // Arrange: AST for "int main() { return a; }"
    Arena test_arena = arena_create(1024 * 4);

    IdentifierNode* ident_a = create_identifier_node("a", &test_arena);
    ReturnStmtNode* return_stmt = create_return_stmt_node((AstNode*)ident_a, &test_arena);
    BlockNode* block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode*)return_stmt, &test_arena);
    FuncDefNode* func_def = create_func_def_node("main", (BlockNode*)block_node, &test_arena);
    ProgramNode* program_node = create_program_node(func_def, &test_arena);

    // Act
    bool is_valid = validate_program((AstNode*)program_node, &test_arena);

    // Assert: Using undeclared 'a' should be invalid
    TEST_ASSERT_FALSE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_redeclared_variable_same_scope(void) {
    // Arrange: AST for "int main() { int a; int a; return 0; }"
    Arena test_arena = arena_create(1024 * 4);

    VarDeclNode* var_decl_a1 = create_var_decl_node("int", "a", NULL, &test_arena);
    VarDeclNode* var_decl_a2 = create_var_decl_node("int", "a", NULL, &test_arena); // Redeclaration
    IntLiteralNode* int_literal_0 = create_int_literal_node(0, &test_arena);
    ReturnStmtNode* return_stmt = create_return_stmt_node((AstNode*)int_literal_0, &test_arena);

    BlockNode* block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode*)var_decl_a1, &test_arena);
    block_node_add_item(block_node, (AstNode*)var_decl_a2, &test_arena);
    block_node_add_item(block_node, (AstNode*)return_stmt, &test_arena);

    FuncDefNode* func_def = create_func_def_node("main", (BlockNode*)block_node, &test_arena);
    ProgramNode* program_node = create_program_node(func_def, &test_arena);

    // Act
    bool is_valid = validate_program((AstNode*)program_node, &test_arena);

    // Assert: Redeclaring 'a' in the same scope should be invalid
    TEST_ASSERT_FALSE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_shadowed_variable_inner_scope(void) {
    // Arrange: AST for "int main() { int a; { int a; } return 0; }"
    Arena test_arena = arena_create(1024 * 4);

    // Outer scope
    VarDeclNode* var_decl_a_outer = create_var_decl_node("int", "a", NULL, &test_arena);

    // Inner scope
    BlockNode* inner_block_node = create_block_node(&test_arena);
    VarDeclNode* var_decl_a_inner = create_var_decl_node("int", "a", NULL, &test_arena); // Shadows outer 'a'
    block_node_add_item(inner_block_node, (AstNode*)var_decl_a_inner, &test_arena);

    IntLiteralNode* int_literal_0 = create_int_literal_node(0, &test_arena);
    ReturnStmtNode* return_stmt = create_return_stmt_node((AstNode*)int_literal_0, &test_arena);

    // Outer block items
    BlockNode* outer_block_node = create_block_node(&test_arena);
    block_node_add_item(outer_block_node, (AstNode*)var_decl_a_outer, &test_arena);
    block_node_add_item(outer_block_node, (AstNode*)inner_block_node, &test_arena);
    block_node_add_item(outer_block_node, (AstNode*)return_stmt, &test_arena);

    FuncDefNode* func_def = create_func_def_node("main", (BlockNode*)outer_block_node, &test_arena);
    ProgramNode* program_node = create_program_node(func_def, &test_arena);

    // Act
    bool is_valid = validate_program((AstNode*)program_node, &test_arena);

    // Assert: Shadowing 'a' in an inner scope should be valid
    TEST_ASSERT_TRUE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_valid_variable_usage(void) {
    // Arrange: AST for "int main() { int a; return a; }"
    Arena test_arena = arena_create(1024 * 4);

    // Variable declaration: int a;
    VarDeclNode* var_decl_a = create_var_decl_node("int", "a", NULL, &test_arena);

    // Identifier for 'a' in return statement
    IdentifierNode* ident_a = create_identifier_node("a", &test_arena);

    // Return statement: return a;
    ReturnStmtNode* return_stmt = create_return_stmt_node((AstNode*)ident_a, &test_arena);

    // Block node containing the declaration and return statement
    BlockNode* block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode*)var_decl_a, &test_arena);
    block_node_add_item(block_node, (AstNode*)return_stmt, &test_arena);

    // Function definition: int main() { ... }
    FuncDefNode* func_def = create_func_def_node("main", block_node, &test_arena);

    // Program node
    ProgramNode* program_node = create_program_node(func_def, &test_arena);

    // Act: Validate the program
    bool is_valid = validate_program((AstNode*)program_node, &test_arena);

    // Assert: The program should be valid
    TEST_ASSERT_TRUE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_invalid_assignment_lvalue(void) {
    // Arrange: AST for "int main() { int a = 2; a + 3 = 4; return a; }"
    Arena test_arena = arena_create(1024 * 4);

    // int a = 2;
    IntLiteralNode* val_2 = create_int_literal_node(2, &test_arena);
    VarDeclNode* var_decl_a = create_var_decl_node("int", "a", (AstNode*)val_2, &test_arena);

    // Expression: a + 3
    IdentifierNode* ident_a_lhs_expr = create_identifier_node("a", &test_arena);
    IntLiteralNode* val_3 = create_int_literal_node(3, &test_arena);
    // Assuming OP_ADD is defined in BinaryOperatorType
    BinaryOpNode* add_expr = create_binary_op_node(OPERATOR_ADD, (AstNode*)ident_a_lhs_expr, (AstNode*)val_3, &test_arena);

    // Assignment statement: (a + 3) = 4
    IntLiteralNode* val_4 = create_int_literal_node(4, &test_arena);
    // Use OPERATOR_ASSIGN
    BinaryOpNode* assign_expr_stmt = create_binary_op_node(OPERATOR_ASSIGN, (AstNode*)add_expr, (AstNode*)val_4, &test_arena);

    // return a;
    IdentifierNode* ident_a_ret = create_identifier_node("a", &test_arena);
    ReturnStmtNode* return_stmt = create_return_stmt_node((AstNode*)ident_a_ret, &test_arena);

    // Block node
    BlockNode* block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode*)var_decl_a, &test_arena);
    // In C, an assignment expression can be a statement. So, add assign_expr_stmt directly.
    block_node_add_item(block_node, (AstNode*)assign_expr_stmt, &test_arena);
    block_node_add_item(block_node, (AstNode*)return_stmt, &test_arena);

    // Function definition
    FuncDefNode* func_def = create_func_def_node("main", block_node, &test_arena);

    // Program node
    ProgramNode* program_node = create_program_node(func_def, &test_arena);

    // Act: Validate the program
    bool is_valid = validate_program((AstNode*)program_node, &test_arena);

    // Assert: The program should be invalid due to incorrect l-value in assignment
    TEST_ASSERT_FALSE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}
