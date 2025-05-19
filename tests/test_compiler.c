#include "unity.h"
#include "../src/strings/strings.h"
#include "../src/parser/ast.h" // For AstNode type
#include "../src/compiler/compiler.h" // Include the new compiler header
#include "../src/memory/arena.h" // Needed for Arena
#include <stdlib.h> // For NULL


// --- Test Cases ---

static void test_compile_return_4(void) {
    Arena test_arena = arena_create(1024 * 8); // Arena for compilation
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

static void test_compile_return_negated_parenthesized_constant(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create test arena for negated parenthesized constant test");

    const char *input_c = "int main(void) { return -((((10)))); }";
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // t0 -> max_id=0 -> (0+1)*8=8 bytes -> aligned 16 -> min 32 bytes
            "    movl $10, %eax\n" // Load constant 10 into %eax
            "    negl %eax\n" // Negate %eax (eax is now -10)
            "    movl %eax, -8(%rbp)\n" // Store result to t0 (-8(%rbp))
            "    movl -8(%rbp), %eax\n" // return t0 (load t0 into eax for return)
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512); // Initialize buffer, 512 should be ample for this output

    // Run the core compiler pipeline
    // Parameters: input_c, lex_only, parse_only, tac_only, silent_mode, output_buffer, arena
    bool const success = compile(input_c, false, false, false, true, &sb, &test_arena);

    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for negated parenthesized constant");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for negated parenthesized constant test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm,
                                     "Generated assembly mismatch for negated parenthesized constant");

    arena_destroy(&test_arena);
}

static void test_compile_return_double_negation(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for double negation test");

    const char *input_c = "int main(void) { return -(-4); }";
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // t0, t1 -> max_id=1 -> (1+1)*8=16 bytes -> min 32 bytes
            "    movl $4, %eax\n" // Load constant 4 into %eax (for inner -4; src_str for NEGATE becomes $4)
            "    negl %eax\n" // Negate %eax (eax is now -4)
            "    movl %eax, -8(%rbp)\n" // Store result to t0 (-8(%rbp))
            "    movl -8(%rbp), %eax\n" // Load t0 (-4) into %eax (for outer -t0)
            "    negl %eax\n" // Negate %eax (eax is now 4)
            "    movl %eax, -16(%rbp)\n" // Store result to t1 (-16(%rbp))
            "    movl -16(%rbp), %eax\n" // Return t1 (load t1 into eax for return)
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512);

    bool const success = compile(input_c, false, false, false, true, &sb, &test_arena);

    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for double negation");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for double negation test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for double negation");

    arena_destroy(&test_arena);
}

static void test_compile_return_addition(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for addition test");

    const char *input_c = "int main(void) { return 5 + 3; }";
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"       // Stack space for TAC temporary t0
            "    movl $5, %eax\n"        // Load 5
            "    addl $3, %eax\n"        // Add 3 (eax is now 8)
            "    movl %eax, -8(%rbp)\n"  // Store result (8) into t0
            "    movl -8(%rbp), %eax\n"  // Load t0 for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512); // Initialize buffer

    bool const success = compile(input_c, false, false, false, true, &sb, &test_arena);

    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for addition");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for addition test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for addition");

    arena_destroy(&test_arena);
}

static void test_compile_return_less_than_false(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for less_than_false test");

    const char *input_c = "int main(void) { return 10 < 5; }";
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"       // Stack space for TAC temporary t0
            "    movl $10, %eax\n"       // Load 10
            "    cmpl $5, %eax\n"        // Compare with 5
            "    setl %al\n"             // Set AL if 10 < 5 (false, so AL=0)
            "    movzbl %al, %eax\n"     // Zero-extend AL to EAX (EAX=0)
            "    movl %eax, -8(%rbp)\n"  // Store result (0) into t0
            "    movl -8(%rbp), %eax\n"  // Load t0 for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512); // Initialize buffer

    bool const success = compile(input_c, false, false, false, true, &sb, &test_arena);

    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for less_than_false");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for less_than_false test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for less_than_false");

    arena_destroy(&test_arena);
}

// --- Test Runner ---

void run_compiler_tests(void) {
    RUN_TEST(test_compile_return_4);
    RUN_TEST(test_compile_return_negated_parenthesized_constant);
    RUN_TEST(test_compile_return_double_negation);
    RUN_TEST(test_compile_return_addition);
    RUN_TEST(test_compile_return_less_than_false);
}
