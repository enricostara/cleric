#include "unity/unity.h"
#include "../src/codegen/codegen.h"
#include "../src/parser/ast.h"
#include "../src/strings/strings.h"
#include "../src/memory/arena.h"
#include "../src/parser/parser.h"

// --- Test Cases ---

// Test generating code for a simple function returning an integer literal
static void test_codegen_simple_return(void) {
    // 1. Create AST: program { func main() { return 42; } }
    Arena test_arena = arena_create(1024 * 4);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    IntLiteralNode *return_expr = create_int_literal_node(42, &test_arena);
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) return_expr, &test_arena);
    FuncDefNode *func_def = create_func_def_node("main", (AstNode *) return_stmt, &test_arena);
    ProgramNode *program = create_program_node(func_def, &test_arena);

    // 2. Setup Codegen State (just the buffer)
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 256);

    // 3. Generate Code
    bool success = codegen_generate_program(&sb, program);

    // 4. Assertions
    TEST_ASSERT_TRUE_MESSAGE(success, "codegen_generate_program failed");

    // Expected assembly for: return 42;
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    movl    $42, %eax\n"
            "    retq\n";

    const char *actual_asm = string_buffer_content_str(&sb); // Use read-only access
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL after content_str");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch");

    // 5. Cleanup
    arena_destroy(&test_arena);
}

// --- Test Runner ---

void run_codegen_tests(void) {
    RUN_TEST(test_codegen_simple_return);
    // Add more RUN_TEST calls for other codegen tests here
}
