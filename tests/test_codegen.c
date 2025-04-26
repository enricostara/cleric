#include "unity/unity.h"
#include "../src/codegen/codegen.h"
#include "../src/parser/ast.h"
#include "../src/strings/strings.h"

// --- Test Cases ---

// Test generating code for a simple function returning an integer literal
static void test_codegen_simple_return(void) {
    // 1. Create AST: program { func main() { return 42; } }
    IntLiteralNode *return_expr = create_int_literal_node(42);
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) return_expr);
    FuncDefNode *func_def = create_func_def_node("main", (AstNode *) return_stmt);
    ProgramNode *program = create_program_node(func_def);

    // 2. Setup Codegen State (just the buffer)
    StringBuffer sb;
    string_buffer_init(&sb, 256);

    // 3. Generate Code
    bool success = codegen_generate_program(&sb, program);

    // 4. Assertions
    TEST_ASSERT_TRUE(success);

    // Expected assembly for: return 42;
    const char *expected_asm =
            ".section .text\n"
            ".globl _main\n"
            "_main:\n"
            "    movl    $42, %eax\n"
            "    retq\n";

    const char *actual_asm = string_buffer_get_content(&sb);
    TEST_ASSERT_EQUAL_STRING(expected_asm, actual_asm);

    // 5. Cleanup
    string_buffer_destroy(&sb);
    free_ast((AstNode *) program); // Free the AST
}

// --- Test Runner ---

void run_codegen_tests(void) {
    RUN_TEST(test_codegen_simple_return);
    // Add more RUN_TEST calls for other codegen tests here
}
