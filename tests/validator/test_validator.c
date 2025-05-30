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

// Test Suite Runner
void run_validator_tests(void) {
    RUN_TEST(test_validate_empty_function);
    RUN_TEST(test_validate_undeclared_variable);
    RUN_TEST(test_validate_redeclared_variable_same_scope);
    RUN_TEST(test_validate_shadowed_variable_inner_scope);
    RUN_TEST(test_validate_valid_variable_usage);
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
    FuncDefNode *func_def = create_func_def_node("main", (BlockNode *) block_node, &test_arena);

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

    IdentifierNode *ident_a = create_identifier_node("a", &test_arena);
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) ident_a, &test_arena);
    BlockNode *block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode *) return_stmt, &test_arena);
    FuncDefNode *func_def = create_func_def_node("main", (BlockNode *) block_node, &test_arena);
    ProgramNode *program_node = create_program_node(func_def, &test_arena);

    // Act
    bool is_valid = validate_program((AstNode *) program_node, &test_arena);

    // Assert: Using undeclared 'a' should be invalid
    TEST_ASSERT_FALSE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_redeclared_variable_same_scope(void) {
    // Arrange: AST for "int main() { int a; int a; return 0; }"
    Arena test_arena = arena_create(1024 * 4);
    Token dummy_token_a = {.type = TOKEN_IDENTIFIER, .lexeme = "a", .position = 0};

    VarDeclNode *var_decl_a1 = create_var_decl_node("int", "a", dummy_token_a, NULL, &test_arena);
    VarDeclNode *var_decl_a2 = create_var_decl_node("int", "a", dummy_token_a, NULL, &test_arena); // Redeclaration
    IntLiteralNode *int_literal_0 = create_int_literal_node(0, &test_arena);
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) int_literal_0, &test_arena);

    BlockNode *block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode *) var_decl_a1, &test_arena);
    block_node_add_item(block_node, (AstNode *) var_decl_a2, &test_arena);
    block_node_add_item(block_node, (AstNode *) return_stmt, &test_arena);

    FuncDefNode *func_def = create_func_def_node("main", (BlockNode *) block_node, &test_arena);
    ProgramNode *program_node = create_program_node(func_def, &test_arena);

    // Act
    bool is_valid = validate_program((AstNode *) program_node, &test_arena);

    // Assert: Redeclaring 'a' in the same scope should be invalid
    TEST_ASSERT_FALSE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_shadowed_variable_inner_scope(void) {
    // Arrange: AST for "int main() { int a; { int a; } return 0; }"
    Arena test_arena = arena_create(1024 * 4);
    Token dummy_token_a_outer = {.type = TOKEN_IDENTIFIER, .lexeme = "a", .position = 0};
    Token dummy_token_a_inner = {.type = TOKEN_IDENTIFIER, .lexeme = "a", .position = 0};
    // Position can be same for distinct tokens if lexeme/type differs or context implies uniqueness

    // Outer scope 'a'
    VarDeclNode *var_decl_a_outer = create_var_decl_node("int", "a", dummy_token_a_outer, NULL, &test_arena);

    // Inner scope
    BlockNode *inner_block_node = create_block_node(&test_arena);
    VarDeclNode *var_decl_a_inner = create_var_decl_node("int", "a", dummy_token_a_inner, NULL, &test_arena);
    // Shadows outer 'a'
    block_node_add_item(inner_block_node, (AstNode *) var_decl_a_inner, &test_arena);

    IntLiteralNode *int_literal_0 = create_int_literal_node(0, &test_arena);
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) int_literal_0, &test_arena);

    // Outer block items
    BlockNode *outer_block_node = create_block_node(&test_arena);
    block_node_add_item(outer_block_node, (AstNode *) var_decl_a_outer, &test_arena);
    block_node_add_item(outer_block_node, (AstNode *) inner_block_node, &test_arena);
    block_node_add_item(outer_block_node, (AstNode *) return_stmt, &test_arena);

    FuncDefNode *func_def = create_func_def_node("main", (BlockNode *) outer_block_node, &test_arena);
    ProgramNode *program_node = create_program_node(func_def, &test_arena);

    // Act
    bool is_valid = validate_program((AstNode *) program_node, &test_arena);

    // Assert: Shadowing 'a' in an inner scope should be valid
    TEST_ASSERT_TRUE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

static void test_validate_valid_variable_usage(void) {
    // Arrange: AST for "int main() { int a; a = 5; return a; }"
    Arena test_arena = arena_create(1024 * 4);
    Token dummy_token_a = {.type = TOKEN_IDENTIFIER, .lexeme = "a", .position = 0};

    VarDeclNode *var_decl_a = create_var_decl_node("int", "a", dummy_token_a, NULL, &test_arena);
    IdentifierNode *ident_a_assign_lhs = create_identifier_node("a", &test_arena);
    IntLiteralNode *val_5 = create_int_literal_node(5, &test_arena);
    BinaryOpNode *assign_stmt = create_binary_op_node(OPERATOR_ASSIGN, (AstNode *) ident_a_assign_lhs,
                                                      (AstNode *) val_5, &test_arena);

    IdentifierNode *ident_a_ret = create_identifier_node("a", &test_arena);
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) ident_a_ret, &test_arena);

    // Block node containing the declaration and return statement
    BlockNode *block_node = create_block_node(&test_arena);
    block_node_add_item(block_node, (AstNode *) var_decl_a, &test_arena);
    block_node_add_item(block_node, (AstNode *) assign_stmt, &test_arena);
    block_node_add_item(block_node, (AstNode *) return_stmt, &test_arena);

    // Function definition: int main() { ... }
    FuncDefNode *func_def = create_func_def_node("main", block_node, &test_arena);

    // Program node
    ProgramNode *program_node = create_program_node(func_def, &test_arena);

    // Act: Validate the program
    bool is_valid = validate_program((AstNode *) program_node, &test_arena);

    // Assert: The program should be valid
    TEST_ASSERT_TRUE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}
