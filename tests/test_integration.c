#include "unity.h"
#include "../src/driver/driver.h"
#include "../src/strings/strings.h"
#include "../src/parser/ast.h" // For free_ast
#include <stdlib.h> // For NULL


// --- Test Cases ---

static void test_compile_return_4(void) {
    const char *input_c = "int main(void) { return 4; }";
    // IMPORTANT: Ensure expected_asm EXACTLY matches the compiler output,
    // including newlines and spacing.
    const char *expected_asm =
        ".globl _main\n"
        "_main:\n"
        "    movl    $4, %eax\n"
        "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, 256); // Initialize buffer
    AstNode *ast_root = NULL;

    // Run the core compiler pipeline (no _only flags)
    bool success = run_compiler_core(input_c, false, false, false, &sb, &ast_root);

    // Assert compilation succeeded
    TEST_ASSERT_TRUE_MESSAGE(success, "run_compiler_core failed");

    // Assert the generated assembly matches expected
    const char *actual_asm = string_buffer_get_content(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch");

    // Cleanup
    // free_ast(ast_root); // AST cleanup is handled by arena_destroy within run_compiler_core
    string_buffer_destroy(&sb); // Destroy the buffer
}

// Add more test cases here...
// static void test_compile_something_else(void) { ... }

// --- Test Runner --- 

void run_integration_tests(void) {
    RUN_TEST(test_compile_return_4);
}
