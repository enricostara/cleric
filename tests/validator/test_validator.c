#include "unity.h"
#include "validator/validator.h"
#include "parser/ast.h"
#include "memory/arena.h"
#include "lexer/lexer.h" // For Token in AST nodes if needed, or for dummy tokens
#include "strings/strings.h" // For arena_strdup if needed for names

#include <string.h>

// Forward declarations for test functions
static void test_validate_empty_function(void);

// Test Suite Runner
void run_validator_tests(void) {
    RUN_TEST(test_validate_empty_function);
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
    FuncDefNode *func_def = create_func_def_node("main", block_node, &test_arena);

    // ProgramNode
    ProgramNode *program_node = create_program_node( func_def, &test_arena);

    // Act: Validate the program
    bool is_valid = validate_program((AstNode *) program_node, &test_arena);

    // Assert: The program should be valid
    TEST_ASSERT_TRUE(is_valid);

    // Cleanup
    arena_destroy(&test_arena);
}

// Add more test cases for different scenarios:
// - Undeclared variables
// - Redeclared variables (same scope, different scopes)
// - Valid variable usage
// - Nested blocks with correct scoping
// - Etc.
