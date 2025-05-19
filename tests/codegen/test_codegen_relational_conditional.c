#include "../_unity/unity.h"
#include "../../src/ir/tac.h"
#include "../../src/memory/arena.h"


void verify_asm_for_function(const char *test_description, // For TEST_ASSERT_EQUAL_STRING_MESSAGE
                             TacFunction *func, // Caller creates and populates this with an arena
                             Arena *arena, // The arena used by the caller for func and its instructions
                             const char *expected_assembly
);

// --- Test Cases ---

static void test_codegen_relational_equal_consts_true(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_equal_consts_true");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    // Note: create_tac_instruction_* returns a pointer, but add_instruction_to_function might copy it
    // or the TacFunction might own them. Let's stick to the pattern of creating them directly for the function.
    TacInstruction *instr1 = create_tac_instruction_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    // Create TacFunction
    TacFunction *func = create_tac_function("test_eq_true", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    // Expected Assembly (max_temp_id=0 -> (0+1)*8=8 bytes, aligned to 16 is 16, min 32 bytes)
    const char *expected_asm =
            ".globl _test_eq_true\n"
            "_test_eq_true:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $5, %eax\n"
            "    sete %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_equal_consts_true: t0 = (5 == 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_equal_consts_false(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_equal_consts_false");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_equal(t0, const5, const6, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_eq_false", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_eq_false\n"
            "_test_eq_false:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $6, %eax\n"
            "    sete %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_equal_consts_false: t0 = (5 == 6); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}


static void test_codegen_relational_not_equal_consts_true(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_not_equal_consts_true");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_not_equal(t0, const5, const6, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ne_true", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_ne_true\n"
            "_test_ne_true:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $6, %eax\n"
            "    setne %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_not_equal_consts_true: t0 = (5 != 6); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_not_equal_consts_false(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_not_equal_consts_false");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_not_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ne_false", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_ne_false\n"
            "_test_ne_false:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $5, %eax\n"
            "    setne %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_not_equal_consts_false: t0 = (5 != 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_consts_true_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_less_consts_true_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_less(t0, const5, const6, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_lt_true", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_lt_true\n"
            "_test_lt_true:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $6, %eax\n"
            "    setl %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_consts_true_less: t0 = (5 < 6); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_consts_false_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_less_consts_false_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_less(t0, const5_1, const5_2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_lt_false_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_lt_false_eq\n"
            "_test_lt_false_eq:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $5, %eax\n"
            "    setl %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_consts_false_equal: t0 = (5 < 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_equal_consts_true_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_less_equal_consts_true_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_less_equal(t0, const5, const6, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_le_true_lt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_le_true_lt\n"
            "_test_le_true_lt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $6, %eax\n"
            "    setle %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_equal_consts_true_less: t0 = (5 <= 6); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_consts_false_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_less_consts_false_greater");

    // Operands: t0 = (10 < 5) -> t0 = 0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const10 = create_tac_operand_const(10); // src1
    TacOperand const5 = create_tac_operand_const(5); // src2

    // Instructions
    // t0 = const10 < const5
    TacInstruction *instr1 = create_tac_instruction_less_equal(t0, const10, const5, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_less_false_greater", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_less_false_greater\n"
            "_test_less_false_greater:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $10, %eax\n" // src1 (10) into eax
            "    cmpl $5, %eax\n" // compare eax with src2 (5) - This was the pattern in your actual output
            "    setl %al\n" // Should be setl for TAC_INS_LESS
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // Store result (0) into t0
            "    movl -8(%rbp), %eax\n" // Load t0 for return
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_consts_false_greater: t0 = (10 < 5)",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_equal_consts_true_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_less_equal_consts_true_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_less_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_le_true_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_le_true_eq\n"
            "_test_le_true_eq:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $5, %eax\n"
            "    setle %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_equal_consts_true_equal: t0 = (5 <= 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_equal_consts_false_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_less_equal_consts_false_greater");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const6 = create_tac_operand_const(6);
    TacOperand const5 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_less_equal(t0, const6, const5, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_le_false_gt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_le_false_gt\n"
            "_test_le_false_gt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $6, %eax\n"
            "    cmpl $5, %eax\n"
            "    setle %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_equal_consts_false_greater: t0 = (6 <= 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_consts_true_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_greater_consts_true_greater");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const6 = create_tac_operand_const(6);
    TacOperand const5 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_greater(t0, const6, const5, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_gt_true_gt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_gt_true_gt\n"
            "_test_gt_true_gt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $6, %eax\n"
            "    cmpl $5, %eax\n"
            "    setg %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_consts_true_greater: t0 = (6 > 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_consts_false_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_greater_consts_false_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_greater(t0, const5_1, const5_2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_gt_false_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_gt_false_eq\n"
            "_test_gt_false_eq:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $5, %eax\n"
            "    setg %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_consts_false_equal: t0 = (5 > 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_consts_false_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_greater_consts_false_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_greater(t0, const5, const6, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_gt_false_lt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_gt_false_lt\n"
            "_test_gt_false_lt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $6, %eax\n"
            "    setg %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_consts_false_less: t0 = (5 > 6); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_equal_consts_true_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_greater_equal_consts_true_greater")
    ;

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const6 = create_tac_operand_const(6);
    TacOperand const5 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_greater_equal(t0, const6, const5, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ge_true_gt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_ge_true_gt\n"
            "_test_ge_true_gt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $6, %eax\n"
            "    cmpl $5, %eax\n"
            "    setge %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_equal_consts_true_greater: t0 = (6 >= 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_equal_consts_true_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_greater_equal_consts_true_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_greater_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ge_true_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_ge_true_eq\n"
            "_test_ge_true_eq:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $5, %eax\n"
            "    setge %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_equal_consts_true_equal: t0 = (5 >= 5); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_equal_consts_false_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start,
                                 "Failed to create arena for test_codegen_relational_greater_equal_consts_false_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction *instr1 = create_tac_instruction_greater_equal(t0, const5, const6, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ge_false_lt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
            ".globl _test_ge_false_lt\n"
            "_test_ge_false_lt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, %eax\n"
            "    cmpl $6, %eax\n"
            "    setge %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_equal_consts_false_less: t0 = (5 >= 6); return t0;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

// --- Test for TAC_INS_IF_FALSE_GOTO ---

static void test_codegen_if_false_goto_jumps(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_false_goto_jumps");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const0 = create_tac_operand_const(0);
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");

    // Instructions
    // t0 = 0
    TacInstruction *instr1 = create_tac_instruction_copy(t0, const0, &test_arena);
    // IF_FALSE t0 GOTO L0
    TacInstruction *instr2 = create_tac_instruction_if_false_goto(t0, label0, &test_arena);
    // t1 = 10 (skipped)
    TacInstruction *instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // L0:
    TacInstruction *instr4 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20
    TacInstruction *instr5 = create_tac_instruction_copy(t1, const20, &test_arena);
    // RETURN t1
    TacInstruction *instr6 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_false_jumps", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);

    const char *expected_asm =
            ".globl _test_if_false_jumps\n"
            "_test_if_false_jumps:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $0, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    testl %eax, %eax\n"
            "    jz _L0\n"
            "    movl $10, -16(%rbp)\n"
            "_L0:\n"
            "    movl $20, -16(%rbp)\n"
            "    movl -16(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_if_false_goto_jumps: t0=0; IF_FALSE t0 GOTO L0; t1=10; L0: t1=20; RETURN t1;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_if_false_goto_no_jump(void) {
    Arena test_arena = arena_create(1024 * 4);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_false_goto_no_jump");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const1 = create_tac_operand_const(1);
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");
    TacOperand label1 = create_tac_operand_label("_L1");

    // Instructions
    // t0 = 1 (true)
    TacInstruction *instr1 = create_tac_instruction_copy(t0, const1, &test_arena);
    // IF_FALSE t0 GOTO _L0 (no jump)
    TacInstruction *instr2 = create_tac_instruction_if_false_goto(t0, label0, &test_arena);
    // t1 = 10 (executed)
    TacInstruction *instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // GOTO _L1
    TacInstruction *instr4 = create_tac_instruction_goto(label1, &test_arena);
    // _L0: (skipped section)
    TacInstruction *instr5 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20 (skipped)
    TacInstruction *instr6 = create_tac_instruction_copy(t1, const20, &test_arena);
    // _L1: (continue execution)
    TacInstruction *instr7 = create_tac_instruction_label(label1, &test_arena);
    // RETURN t1 (should be 10)
    TacInstruction *instr8 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_false_no_jump", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);
    add_instruction_to_function(func, instr7, &test_arena);
    add_instruction_to_function(func, instr8, &test_arena);

    const char *expected_asm =
            ".globl _test_if_false_no_jump\n"
            "_test_if_false_no_jump:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $1, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    testl %eax, %eax\n"
            "    jz _L0\n"
            "    movl $10, -16(%rbp)\n"
            "    jmp _L1\n"
            "_L0:\n"
            "    movl $20, -16(%rbp)\n"
            "_L1:\n"
            "    movl -16(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function(
        "test_codegen_if_false_goto_no_jump: t0=1; IF_FALSE t0 GOTO L0; t1=10; GOTO L1; L0: t1=20; L1: RETURN t1;",
        func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_if_true_goto_jumps(void) {
    Arena test_arena = arena_create(1024 * 2); // Start with 2KB, can increase if needed
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_true_goto_jumps");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const1 = create_tac_operand_const(1); // For t0 = 1 (true condition)
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");

    // Instructions
    // t0 = 1
    TacInstruction *instr1 = create_tac_instruction_copy(t0, const1, &test_arena);
    // IF_TRUE t0 GOTO _L0 (jump occurs because t0 is true)
    TacInstruction *instr2 = create_tac_instruction_if_true_goto(t0, label0, &test_arena);
    // t1 = 10 (skipped)
    TacInstruction *instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // _L0:
    TacInstruction *instr4 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20 (executed)
    TacInstruction *instr5 = create_tac_instruction_copy(t1, const20, &test_arena);
    // RETURN t1 (should be 20)
    TacInstruction *instr6 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_true_jumps", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);

    const char *expected_asm =
            ".globl _test_if_true_jumps\n"
            "_test_if_true_jumps:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $1, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    testl %eax, %eax\n"
            "    jnz _L0\n"
            "    movl $10, -16(%rbp)\n"
            "_L0:\n"
            "    movl $20, -16(%rbp)\n"
            "    movl -16(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_if_true_goto_jumps: t0=1; IF_TRUE t0 GOTO L0; t1=10; L0: t1=20; RETURN t1;",
                            func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_if_true_goto_no_jump(void) {
    Arena test_arena = arena_create(1024 * 4); // Matching increased size from similar test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_true_goto_no_jump");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const0 = create_tac_operand_const(0); // For t0 = 0 (false condition)
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");
    TacOperand label1 = create_tac_operand_label("_L1");

    // Instructions
    // t0 = 0
    TacInstruction *instr1 = create_tac_instruction_copy(t0, const0, &test_arena);
    // IF_TRUE t0 GOTO _L0 (no jump because t0 is false)
    TacInstruction *instr2 = create_tac_instruction_if_true_goto(t0, label0, &test_arena);
    // t1 = 10 (executed)
    TacInstruction *instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // GOTO _L1
    TacInstruction *instr4 = create_tac_instruction_goto(label1, &test_arena);
    // _L0: (skipped section)
    TacInstruction *instr5 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20 (skipped)
    TacInstruction *instr6 = create_tac_instruction_copy(t1, const20, &test_arena);
    // _L1: (continue execution)
    TacInstruction *instr7 = create_tac_instruction_label(label1, &test_arena);
    // RETURN t1 (should be 10)
    TacInstruction *instr8 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_true_no_jump", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);
    add_instruction_to_function(func, instr7, &test_arena);
    add_instruction_to_function(func, instr8, &test_arena);

    const char *expected_asm =
            ".globl _test_if_true_no_jump\n"
            "_test_if_true_no_jump:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $0, -8(%rbp)\n"
            "    movl -8(%rbp), %eax\n"
            "    testl %eax, %eax\n"
            "    jnz _L0\n"
            "    movl $10, -16(%rbp)\n"
            "    jmp _L1\n"
            "_L0:\n"
            "    movl $20, -16(%rbp)\n"
            "_L1:\n"
            "    movl -16(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";

    verify_asm_for_function(
        "test_codegen_if_true_goto_no_jump: t0=0; IF_TRUE t0 GOTO L0; t1=10; GOTO L1; L0: t1=20; L1: RETURN t1;",
        func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}


// --- Test Runner ---

void run_codegen_relational_conditional_tests(void) {
    RUN_TEST(test_codegen_relational_equal_consts_true);
    RUN_TEST(test_codegen_relational_equal_consts_false);
    RUN_TEST(test_codegen_relational_not_equal_consts_true);
    RUN_TEST(test_codegen_relational_not_equal_consts_false);
    RUN_TEST(test_codegen_relational_less_consts_true_less);
    RUN_TEST(test_codegen_relational_less_consts_false_equal);
    RUN_TEST(test_codegen_relational_less_consts_false_greater);
    RUN_TEST(test_codegen_relational_less_equal_consts_true_less);
    RUN_TEST(test_codegen_relational_less_equal_consts_true_equal);
    RUN_TEST(test_codegen_relational_less_equal_consts_false_greater);
    RUN_TEST(test_codegen_relational_greater_consts_true_greater);
    RUN_TEST(test_codegen_relational_greater_consts_false_equal);
    RUN_TEST(test_codegen_relational_greater_consts_false_less);
    RUN_TEST(test_codegen_relational_greater_equal_consts_true_greater);
    RUN_TEST(test_codegen_relational_greater_equal_consts_true_equal);
    RUN_TEST(test_codegen_relational_greater_equal_consts_false_less);
    RUN_TEST(test_codegen_if_false_goto_jumps);
    RUN_TEST(test_codegen_if_false_goto_no_jump);
    RUN_TEST(test_codegen_if_true_goto_jumps);
    RUN_TEST(test_codegen_if_true_goto_no_jump);
}
