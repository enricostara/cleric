#include "unity.h"
#include "../src/strings/strings.h"
#include "../src/parser/ast.h" // For AstNode type
#include "../src/compiler/compiler.h" // Include the new compiler header
#include "../src/memory/arena.h" // Needed for Arena
#include <stdlib.h> // For NULL


// --- Test Cases ---

static void test_compile_return_4(void) {
    Arena test_arena = arena_create(1024 * 4); // Arena for compilation
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    const char *input_c = "int main(void) { return 4; }";
    // IMPORTANT: Ensure expected_asm EXACTLY matches the compiler output,
    // including newlines and spacing.
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // Standard prologue with stack allocation
            "    movl $4, %eax\n" // Return value
            "    leave\n" // Standard epilogue
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 256); // Initialize buffer

    // Run the core compiler pipeline (no _only flags)
    bool const success = compile(input_c, false, false, false, true, &sb, &test_arena);

    // Assert compilation succeeded
    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed");

    // Assert the generated assembly matches expected
    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch");

    // Cleanup
    arena_destroy(&test_arena); // Destroy arena (cleans up buffer too)
}

// Add more test cases here...
// static void test_compile_something_else(void) { ... }

// --- Test Runner --- 

void run_compiler_tests(void) {
    RUN_TEST(test_compile_return_4);
}
