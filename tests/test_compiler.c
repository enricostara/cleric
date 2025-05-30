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
    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);

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
    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);

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

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);

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
            "    subq $32, %rsp\n" // Stack space for TAC temporary t0
            "    movl $5, %eax\n" // Load 5
            "    addl $3, %eax\n" // Add 3 (eax is now 8)
            "    movl %eax, -8(%rbp)\n" // Store result (8) into t0
            "    movl -8(%rbp), %eax\n" // Load t0 for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512); // Initialize buffer

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);

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
            "    subq $32, %rsp\n" // Stack space for TAC temporary t0
            "    movl $10, %eax\n" // Load 10
            "    cmpl $5, %eax\n" // Compare with 5
            "    setl %al\n" // Set AL if 10 < 5 (false, so AL=0)
            "    movzbl %al, %eax\n" // Zero-extend AL to EAX (EAX=0)
            "    movl %eax, -8(%rbp)\n" // Store result (0) into t0
            "    movl -8(%rbp), %eax\n" // Load t0 for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512); // Initialize buffer

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);

    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for less_than_false");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for less_than_false test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for less_than_false");

    arena_destroy(&test_arena);
}

static void test_compile_logical_not_false(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical NOT test");

    const char *input_c = "int main(void) { return !0; }";
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // Stack space for TAC temporary t0
            "    movl $0, %eax\n" // Load 0
            "    cmpl $0, %eax\n" // Compare with 0
            "    sete %al\n" // Set AL if 0 == 0 (AL=1)
            "    movzbl %al, %eax\n" // Zero-extend AL to EAX (EAX=1)
            "    movl %eax, -8(%rbp)\n" // Store result (1) into t0
            "    movl -8(%rbp), %eax\n" // Load t0 for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512);

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);
    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for logical NOT");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for logical NOT test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for logical NOT");

    arena_destroy(&test_arena);
}

static void test_compile_logical_and_true_false(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical AND test");

    const char *input_c = "int main(void) { return 1 && 0; }";
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // For TAC temporary and labels
            "    movl $1, %eax\n" // Load LHS (1)
            "    testl %eax, %eax\n" // Is LHS zero?
            "    jz L0\n" // If LHS is 0, result is 0 (short-circuit)
            "    movl $0, %eax\n" // Load RHS (0)
            "    cmpl $0, %eax\n" // Booleanize RHS: (0 == 0) -> false (0)
            "    setne %al\n" // al = (RHS != 0)
            "    movzbl %al, %eax\n" // eax = booleanized RHS
            "    movl %eax, -8(%rbp)\n" // Store booleanized RHS as result
            "    jmp L1\n" // Jump to end
            "L0:\n" // Short-circuit path: LHS of && was false
            "    movl $0, -8(%rbp)\n" // Store 0 as result (short-circuit path)
            "L1:\n"
            "    movl -8(%rbp), %eax\n" // Load final result for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512);

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);
    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for logical AND");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for logical AND test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for logical AND");

    arena_destroy(&test_arena);
}

static void test_compile_logical_or_false_true(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical OR test");

    const char *input_c = "int main(void) { return 0 || 1; }";
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // For TAC temporary and labels
            "    movl $0, %eax\n" // Load LHS (0)
            "    testl %eax, %eax\n" // Is LHS zero?
            "    jnz L1\n" // If LHS is non-zero (true), result of || is 1 (short-circuit)
            // LHS of || was false (0), evaluate RHS: 1
            "    movl $1, %eax\n" // Load RHS (1)
            "    cmpl $0, %eax\n" // Booleanize RHS: (1 == 0) -> false (0) -> setne sets al to 1
            "    setne %al\n" // al = (RHS != 0)
            "    movzbl %al, %eax\n" // eax = booleanized RHS
            "    movl %eax, -8(%rbp)\n" // Store booleanized RHS as result
            "    jmp L2\n" // Jump to end
            "L1:\n"
            "    movl $1, -8(%rbp)\n" // Store 1 as result (short-circuit path for OR)
            "L2:\n"
            "    movl -8(%rbp), %eax\n" // Load final result for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 512);

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);
    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for logical OR");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for logical OR test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for logical OR");

    arena_destroy(&test_arena);
}

static void test_compile_complex_logical_or(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for complex logical OR test");

    const char *input_c = "int main(void) { return 3 < 2 || 2 < 3; }";
    // Expected evaluation: (3 < 2) is false (0). (2 < 3) is true (1).
    // false || true is true (1).
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            // Evaluate 3 < 2 (LHS of ||)
            "    movl $3, %eax\n"
            "    cmpl $2, %eax\n"
            "    setl %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -16(%rbp)\n" // t1 = (3 < 2) -> t1 = 0
            // Logical OR: t1 || (2 < 3)
            "    movl -16(%rbp), %eax\n" // Load t1 (LHS of ||)
            "    testl %eax, %eax\n" // Test if t1 is zero
            "    jnz L1\n" // If t1 is non-zero (true), result of || is 1 (short-circuit)
            // LHS of || was false (t1 = 0), evaluate RHS: 2 < 3
            "    movl $2, %eax\n"
            "    cmpl $3, %eax\n"
            "    setl %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -24(%rbp)\n" // t2 = (2 < 3) -> t2 = 1
            // Booleanize RHS result for final OR result
            "    movl -24(%rbp), %eax\n" // Load t2
            "    cmpl $0, %eax\n" // (t2 == 0)
            "    setne %al\n" // al = (t2 != 0)
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = result of OR (booleanized t2)
            "    jmp L2\n" // Jump to end of OR logic
            "L1:\n" // Short-circuit path: LHS of || was true
            "    movl $1, -8(%rbp)\n" // t0 = 1 (result of || operation)
            "L2:\n" // End of OR logic
            "    movl -8(%rbp), %eax\n" // Load final result (t0) for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 1024); // Increased buffer slightly for more complex asm

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);
    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for complex logical OR");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for complex logical OR test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for complex logical OR");

    arena_destroy(&test_arena);
}

static void test_compile_complex_logical_and_short_circuit(void) {
    Arena test_arena = arena_create(1024 * 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for complex logical AND test");

    const char *input_c = "int main(void) { return 3 < 2 && 2 < 3; }";
    // Expected evaluation: (3 < 2) is false (0). Short-circuits.
    // Result of && is false (0).
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            // Evaluate LHS of &&: (3 < 2)
            "    movl $3, %eax\n"
            "    cmpl $2, %eax\n"
            "    setl %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = (3 < 2) -> t0 = 0
            // Logical AND short-circuit check
            "    movl -8(%rbp), %eax\n" // Load t0 (LHS of &&)
            "    testl %eax, %eax\n" // Is t0 zero? (t0 is 0, so ZF=1)
            "    jz L0\n" // If t0 is 0, result of && is 0 (short-circuit). Jumps to L0.
            // ---- This block should NOT be executed due to short-circuit ----
            // Evaluate RHS of &&: (2 < 3)
            "    movl $2, %eax\n"
            "    cmpl $3, %eax\n"
            "    setl %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -24(%rbp)\n" // t2 = (2 < 3) -> t2 = 1 (in non-executed path)
            // Booleanize RHS result (in non-executed path)
            "    movl -24(%rbp), %eax\n"
            "    cmpl $0, %eax\n"
            "    setne %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -16(%rbp)\n" // t1 = booleanized t2 (result of && if LHS was true)
            "    jmp L1\n"
            // ---- End of non-executed block ----
            "L0:\n" // Short-circuit path: LHS of && was false
            "    movl $0, -16(%rbp)\n" // t1 = 0 (result of && operation)
            "L1:\n"
            "    movl -16(%rbp), %eax\n" // Load final result (t1) for return
            "    leave\n"
            "    retq\n";

    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 1024);

    bool const success = compile(input_c, false, false, false, false, true, &sb, &test_arena);
    TEST_ASSERT_TRUE_MESSAGE(success, "compile failed for complex logical AND");

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for complex logical AND test");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch for complex logical AND");

    arena_destroy(&test_arena);
}

static void test_compiler_parse_only_invalid_lvalue(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for parse_only_invalid_lvalue test");

    const char *input_c = "int main(void) { int a = 2; a + 3 = 4; return a; }";
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 128); // Initialize buffer, small size is fine

    // Call compile with parse_only = true (second flag)
    // compile(input, lex_only, parse_only, validate_only, tac_only, silent_mode, output_buffer, arena)
    bool const success = compile(input_c, false, true, false, false, true, &sb, &test_arena);

    // Expect compilation to fail due to parser error (invalid l-value)
    TEST_ASSERT_FALSE_MESSAGE(success, "compile should fail for invalid l-value with parse_only flag");

    // Note: We are not checking the content of sb.error_buffer here, but that could be an extension
    // if the compile function populates it with parser errors in parse_only mode.

    arena_destroy(&test_arena);
}

// --- Test Runner ---

void run_compiler_tests(void) {
    RUN_TEST(test_compile_return_4);
    RUN_TEST(test_compile_return_negated_parenthesized_constant);
    RUN_TEST(test_compile_return_double_negation);
    RUN_TEST(test_compile_return_addition);
    RUN_TEST(test_compile_return_less_than_false);
    RUN_TEST(test_compile_logical_not_false);
    RUN_TEST(test_compile_logical_and_true_false);
    RUN_TEST(test_compile_logical_or_false_true);
    RUN_TEST(test_compile_complex_logical_or);
    RUN_TEST(test_compile_complex_logical_and_short_circuit);
    RUN_TEST(test_compiler_parse_only_invalid_lvalue); // Added new test
}
